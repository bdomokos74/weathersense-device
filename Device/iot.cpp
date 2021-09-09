#include "iot.h"
#include "led.h"
#include "sevenseg.h"
#include "storage.h"
#include "log.h"
#include "local_config.h"
#include "AzIoTSasToken.h"
#include "ca.h"

#include <mqtt_client.h>
#include <az_iot_hub_client.h>
#include <az_result.h>
#include <az_span.h>

/*String containing Hostname, Device Id & Device Key in the format:                         */
/*  "HostName=<host_name>;DeviceId=<device_id>;SharedAccessKey=<device_key>"                */
/*  "HostName=<host_name>;DeviceId=<device_id>;SharedAccessSignature=<device_sas_token>"    */

#define MESSAGE_MAX_LEN 256
#define MESSAGE_ACK_TIMEOUT_MS 10000
#define STATUS_MSG_MAX_LEN 100
#define TWIN_TIMEOUT 5000
#define SAS_TOKEN_DURATION_IN_MINUTES 60

#define MQTT_QOS0 0
#define MQTT_QOS1 1
#define DO_NOT_RETAIN_MSG 0

char* iotConnString = DEV_CONN_STR;
volatile bool hasIoTHub = false;

extern State *deviceState;
extern LedUtil *led;
extern SevenSeg *sevenSeg;
extern Storage *storage;

static bool messageSendingOn = true;
static bool statusRequested = false;

unsigned long IotConn::sendTime = 0;

unsigned long connStartTime = 0;

static bool gotDeviceTwinReq = false;

static esp_mqtt_client_handle_t mqtt_client;
static az_iot_hub_client client;

static const char* host = IOT_CONFIG_IOTHUB_FQDN;
static const char* mqtt_broker_uri = "mqtts://" IOT_CONFIG_IOTHUB_FQDN;
static const char* device_id = IOT_CONFIG_DEVICE_ID;
static const int mqtt_port = 8883;

static char mqtt_client_id[128];
static char mqtt_username[128];
static char mqtt_password[200];
static uint8_t sas_signature_buffer[256];

static char telemetry_topic[128];
static uint8_t telemetry_payload[100];
static uint32_t telemetry_send_count = 0;

static az_span const twin_document_topic_request_id = AZ_SPAN_LITERAL_FROM_STR("get_twin");
static az_span const twin_patch_topic_request_id = AZ_SPAN_LITERAL_FROM_STR("reported_prop");

static AzIoTSasToken sasToken(
    &client,
    AZ_SPAN_FROM_STR(IOT_CONFIG_DEVICE_KEY),
    AZ_SPAN_FROM_BUFFER(sas_signature_buffer),
    AZ_SPAN_FROM_BUFFER(mqtt_password));


void receivedCallback(char* topic, byte* payload, unsigned int length)
{
  Serial.print("Received [");
  Serial.print(topic);
  Serial.print("]: ");
  for (int i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
  }
  Serial.println("");
}

bool _requestTwinGet() {
  char twin_document_topic_buffer[128];
  if (az_result_failed(az_iot_hub_client_twin_document_get_publish_topic(
      &client, twin_document_topic_request_id, twin_document_topic_buffer, sizeof(twin_document_topic_buffer), NULL)))
  {
    logErr("Failed az_iot_hub_client_telemetry_get_publish_topic");
    return false;
  }

  if (esp_mqtt_client_publish(
          mqtt_client,
          twin_document_topic_buffer,
          0,
          NULL,
          MQTT_QOS1,
          NULL) == 0)
  {
    logErr("Failed publishing twin get");
    return false;
  }
  return true;
}

bool _replyTwinGet(az_span *topic, az_span *msg) {
  char statusBuf[128];
  char twin_patch_topic_buffer[128];
  if(az_span_find(*topic, twin_document_topic_request_id)>0) 
  {
    statusRequested = deviceState->updateState((char*)az_span_ptr(*msg));
    deviceState->getStatusString(statusBuf, sizeof(statusBuf));
    Serial.print("response: ");Serial.println(statusBuf);

    // publish status
    if (az_result_failed(az_iot_hub_client_twin_patch_get_publish_topic(
        &client,
        twin_patch_topic_request_id,
        twin_patch_topic_buffer,
        sizeof(twin_patch_topic_buffer),
        NULL)))
    {
      logErr("Failed to get the Twin Patch topic.");
      return false;
    }
    Serial.print("sendtopic:");Serial.println(twin_patch_topic_buffer);
    // Publish the reported property update.
    if(esp_mqtt_client_publish(
        mqtt_client,
        twin_patch_topic_buffer,
        statusBuf,
        0,
        MQTT_QOS1,
        NULL )==-1) 
    {
      logErr("Failed publishing twin patch");
      return false;
    }
    return true;
  } else {
    return false;
  }
}

void _debugEvent(char *topic, char *msg) 
{
  Serial.print("topic=");Serial.println(topic);
  Serial.print("data=");Serial.println(msg);
}

static uint8_t msgBuf[300];
static uint8_t topicBuf[100];
static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event)
{
  int msgId;
  az_span topic = AZ_SPAN_FROM_BUFFER(topicBuf);
  az_span msg = AZ_SPAN_FROM_BUFFER(msgBuf);
  az_span slice;
  switch (event->event_id)
  {
    case MQTT_EVENT_ERROR:
      logMsg("MQTT event MQTT_EVENT_ERROR");
      break;
    case MQTT_EVENT_CONNECTED:
      logMsg("MQTT event MQTT_EVENT_CONNECTED, subscribing twin get");
      msgId = 
        esp_mqtt_client_subscribe(mqtt_client, AZ_IOT_HUB_CLIENT_TWIN_RESPONSE_SUBSCRIBE_TOPIC, MQTT_QOS1);
        logMsg("subscribing:", msgId);
        if (msgId == -1)
        {
          Serial.println("Could not subscribe device twin topic");
        }
        hasIoTHub = true;
      break;
    case MQTT_EVENT_DISCONNECTED:
      logMsg("MQTT event MQTT_EVENT_DISCONNECTED");
      hasIoTHub = false;
      break;
    case MQTT_EVENT_SUBSCRIBED:
      logMsg("MQTT event MQTT_EVENT_SUBSCRIBED, msgid=", event->msg_id);
      
      if(!_requestTwinGet()) return false;
    
      break;
    case MQTT_EVENT_UNSUBSCRIBED:
      logMsg("MQTT event MQTT_EVENT_UNSUBSCRIBED");
      break;
    case MQTT_EVENT_PUBLISHED:
      logMsg("MQTT event MQTT_EVENT_PUBLISHED, msgId=", event->msg_id);
      break;
    case MQTT_EVENT_DATA:
      logMsg("MQTT event MQTT_EVENT_DATA, msgId=", event->msg_id);
      
      slice = az_span_copy(topic, az_span_create((uint8_t*)event->topic, event->topic_len));
      az_span_ptr(slice)[0] = '\0';
      
      slice = az_span_copy(msg, az_span_create((uint8_t*)event->data, event->data_len));
      az_span_ptr(slice)[0] = '\0';

      _debugEvent( (char*)az_span_ptr(topic), (char*)az_span_ptr(msg));

      if(!_replyTwinGet(&topic, &msg))
      {
        if( az_span_find( topic, twin_patch_topic_request_id)>=0) {
          logMsg("do nothing, patch  topic"); 
        } else {
          logMsg("unexpected topic");
        }
      }
      break;
    case MQTT_EVENT_BEFORE_CONNECT:
      logMsg("MQTT event MQTT_EVENT_BEFORE_CONNECT");
      break;
    default:
      logMsg("MQTT event UNKNOWN");
      break;
  }
}

static void initializeIoTHubClient()
{
  if (az_result_failed(az_iot_hub_client_init(
          &client,
          az_span_create((uint8_t*)host, strlen(host)),
          az_span_create((uint8_t*)device_id, strlen(device_id)),
          NULL)))
  {
    logErr("Failed initializing Azure IoT Hub client");
    return;
  }

  size_t client_id_length;
  if (az_result_failed(az_iot_hub_client_get_client_id(
          &client, mqtt_client_id, sizeof(mqtt_client_id) - 1, &client_id_length)))
  {
    logErr("Failed getting client id");
    return;
  }

  // Get the MQTT user name used to connect to IoT Hub
  if (az_result_failed(az_iot_hub_client_get_user_name(
          &client, mqtt_username, sizeofarray(mqtt_username), NULL)))
  {
    logErr("Failed to get MQTT clientId, return code");
    return;
  }

  Serial.print("Client ID: "); Serial.println(mqtt_client_id);
  Serial.print("Username: " ); Serial.println(mqtt_username);
}

static int initializeMqttClient()
{
  if (sasToken.Generate(SAS_TOKEN_DURATION_IN_MINUTES) != 0)
  {
    logErr("Failed generating SAS token");
    return 1;
  } else {
    logMsg("SAS token generated");
    char tmpBuf[200];
    az_span_to_str(tmpBuf, 200, sasToken.Get());
    logMsg(tmpBuf);
  }

  esp_mqtt_client_config_t mqtt_config;
  memset(&mqtt_config, 0, sizeof(mqtt_config));
  mqtt_config.uri = mqtt_broker_uri;
  mqtt_config.port = mqtt_port;
  mqtt_config.client_id = mqtt_client_id;
  mqtt_config.username = mqtt_username;
  mqtt_config.password = (const char*)az_span_ptr(sasToken.Get());
  mqtt_config.keepalive = 30;
  mqtt_config.disable_clean_session = 0;
  mqtt_config.disable_auto_reconnect = false;
  mqtt_config.event_handle = mqtt_event_handler;
  mqtt_config.user_context = NULL;
  mqtt_config.cert_pem = (const char*)ca_pem;

  mqtt_client = esp_mqtt_client_init(&mqtt_config);

  if (mqtt_client == NULL)
  {
    logErr("Failed creating mqtt client");
    return -1;
  }

  esp_err_t start_result = esp_mqtt_client_start(mqtt_client);

  if (start_result != ESP_OK)
  {
    Serial.print("Could not start mqtt client; error code:");Serial.println( start_result);
    return -1;
  }
  
   logMsg("MQTT client started");

  return 0;
  
  
}

IotConn::IotConn(WifiNet *wifiNet) 
{
  this->wifiNet = wifiNet;
}

bool IotConn::connect() 
{
  if(hasIoTHub) return true;

  if(!wifiNet->isConnected()) {
    Serial.println("No WiFi, skip IoT");
    hasIoTHub = false;
    return false;
  }
  initializeIoTHubClient();
  initializeMqttClient();
  return hasIoTHub;
}

bool IotConn::isConnected() 
{
  return hasIoTHub;
}

void IotConn::close() 
{
  //esp_mqtt_client_stop(mqtt_client);
  (void)esp_mqtt_client_destroy(mqtt_client);

  hasIoTHub =false;

}

bool IotConn::requestTwinGet() {
  return _requestTwinGet();
}

bool IotConn::sendData() 
{
  if(!hasIoTHub) return false;

  char *msg = storage->getDataBuf();

  Serial.println("sendData called with msg:");
  Serial.println(msg);

  logMsg("Sending telemetry ...");

  // The topic could be obtained just once during setup,
  // however if properties are used the topic need to be generated again to reflect the
  // current values of the properties.
  if (az_result_failed(az_iot_hub_client_telemetry_get_publish_topic(
          &client, NULL, telemetry_topic, sizeof(telemetry_topic), NULL)))
  {
    logErr("Failed az_iot_hub_client_telemetry_get_publish_topic");
    return false;
  }
  az_span telemetry = az_span_create_from_str(msg);

  if (esp_mqtt_client_publish(
          mqtt_client,
          telemetry_topic,
          (const char*)az_span_ptr(telemetry),
          az_span_size(telemetry),
          MQTT_QOS1,
          DO_NOT_RETAIN_MSG) == 0)
  {
    logErr("Failed publishing");
    return false;
  }
  else
  {
    logMsg("Message published successfully");
    return true;
  }
}


static int _handleDeviceMethod(const char *methodName, const unsigned char *payload, int size, unsigned char **response, int *response_size)
{
  Serial.println("handle device method called");
  logMsgStr("Try to invoke method ", (char*)methodName);

  char payloadBuf[50];
  int result = 200;
  snprintf(payloadBuf, 100, "\"OK\"");

  if (strcmp(methodName, "meas") == 0)
  {
    logMsg("send measurements");
    storage->getMeasurementString(payloadBuf, 50);
  } else if (strcmp(methodName, "start") == 0)
  {
    logMsg("do somethig on start");
    messageSendingOn = true;
  }
  else if (strcmp(methodName, "stop") == 0)
  {
    logMsg("Stop shell");
    messageSendingOn = false;
  }
  else if (strcmp(methodName, "ledon") == 0)
  {
    logMsg("led on");
    led->ledOn();
  }
  else if (strcmp(methodName, "ledoff") == 0)
  {
    logMsg("led off");
    led->ledOff();
  }
  else if (strcmp(methodName, "7segon") == 0)
  {
    logMsg("sevenseg on");
    if(sevenSeg!=NULL && sevenSeg->isConnected()) {
      sevenSeg->printHex(0xBEEF);
    }
  }
  else if (strcmp(methodName, "7segoff") == 0)
  {
    logMsg("sevenseg off");
    if(sevenSeg!=NULL && sevenSeg->isConnected()) {
      sevenSeg->clear();
    }
  }
  else
  {
    logMsgStr("No method %s found", (char*)methodName);
    snprintf(payloadBuf, 50, "\"FAIL\"");
    result = 404;
  }

  *response_size = strlen(payloadBuf) + 1;
  *response = (unsigned char *)strdup(payloadBuf);

  return result;
}
