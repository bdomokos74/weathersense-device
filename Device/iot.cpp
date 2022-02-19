#include "iot.h"
#include "led.h"
#include "sevenseg.h"
#include "storage.h"
#include "log.h"
#include "local_config.h"
#include "AzIoTSasToken.h"
#include "SerialLogger.h"

#include <mqtt_client.h>

#include <az_core.h>
#include <az_iot.h>
#include <azure_ca.h>

/*
#include <az_iot_hub_client.h>
#include <az_result.h>
#include <az_span.h>
*/

#include <esp_system.h>


#define AZURE_SDK_CLIENT_USER_AGENT "c/" AZ_SDK_VERSION_STRING "(ard;esp32)"
#define WEATHERSENSE_CLIENT_VERSION "v2.0"

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

extern int wakeCnt;

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

static az_span const twin_document_topic_request_id = AZ_SPAN_LITERAL_FROM_STR("gettwin");
static az_span const twin_patch_topic_request_id = AZ_SPAN_LITERAL_FROM_STR("reported_prop");
static az_span const directmsg_topic_request_id = AZ_SPAN_LITERAL_FROM_STR("devicebound");
static az_span const methods_topic_request_id = AZ_SPAN_LITERAL_FROM_STR("methods");

static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event);
bool _requestTwinGet(int requestId);
int twinGetId = -1;
int twinStatus = 0;

static AzIoTSasToken sasToken(
    &client,
    AZ_SPAN_FROM_STR(IOT_CONFIG_DEVICE_KEY),
    AZ_SPAN_FROM_BUFFER(sas_signature_buffer),
    AZ_SPAN_FROM_BUFFER(mqtt_password));


static void initializeIoTHubClient()
{
  az_iot_hub_client_options options = az_iot_hub_client_options_default();
  options.user_agent = AZ_SPAN_FROM_STR(AZURE_SDK_CLIENT_USER_AGENT);
  
  if (az_result_failed(az_iot_hub_client_init(
          &client,
          az_span_create((uint8_t*)host, strlen(host)),
          az_span_create((uint8_t*)device_id, strlen(device_id)),
          &options)))
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

az_iot_status _handleMethod(char *methodName, char *response, int response_size);


bool _requestTwinGet(int requestId) {
  Serial.print("req twin get ");
  Serial.println(requestId);
  //Logger.Info("request twin get: "+String(requestId));
  char twin_document_topic_buffer[128];
  
  Serial.println("calling get topic2");
  if (az_result_failed(az_iot_hub_client_twin_document_get_publish_topic(
      &client, twin_document_topic_request_id, twin_document_topic_buffer, sizeof(twin_document_topic_buffer), NULL)))
  {
    Serial.println("_requestTwinGet: Failed az_iot_hub_client_telemetry_get_publish_topic");
    return false;
  }
  Serial.print("_requestTwinGet: topic= ");
  Serial.println(twin_document_topic_buffer);

  const char* str = "";
  
  int msgId = esp_mqtt_client_publish(
          mqtt_client,
          twin_document_topic_buffer,
          0,
          0,
          0,
          NULL);
  if(msgId==0)
  {
    logErr("_requestTwinGet: Failed");
    return false;
  } else {
    logMsg("_requestTwinGet: msgId=", msgId);
  }
  return true;
}

bool _replyTwinGet(az_span *msg) {
  char statusBuf[128];
  char twin_patch_topic_buffer[128];

    statusRequested = deviceState->updateState((char*)az_span_ptr(*msg));
    
    deviceState->getStatusString(statusBuf, sizeof(statusBuf));
    logMsgStr("_replyTwinGet: response: ", statusBuf);

    // publish status
    if (az_result_failed(az_iot_hub_client_twin_patch_get_publish_topic(
        &client,
        twin_patch_topic_request_id,
        twin_patch_topic_buffer,
        sizeof(twin_patch_topic_buffer),
        NULL)))
    {
      logErr("_replyTwinGet: Failed to get the Twin Patch topic.");
      return false;
    }
    logMsgStr("_replyTwinGet, sendtopic: ", twin_patch_topic_buffer);
    // Publish the reported property update.
    int msgId = 
    esp_mqtt_client_publish(
        mqtt_client,
        twin_patch_topic_buffer,
        statusBuf,
        0,
        MQTT_QOS1,
        NULL );
    if(msgId==0) 
    {
      logErr("_replyTwinGet: Failed publishing twin patch");
      return false;
    } else {
      logMsg("_replyTwinGet response sent, msgId=", msgId);
    }
    return true;

}

bool _sendMethodResponse(az_iot_hub_client_method_request *method_request, az_iot_status status, az_span response) {
  char methods_response_topic_buffer[200];
  if( az_result_failed(az_iot_hub_client_methods_response_get_publish_topic(
      &client,
      method_request->request_id,
      (uint16_t)status,
      methods_response_topic_buffer,
      sizeof(methods_response_topic_buffer),
      NULL)))
  {
    logMsg("_sendMethodResponse: Failed to get the Methods Response topic");
    return false;
  }

  // Publish the method response.
  int msgId = esp_mqtt_client_publish(
      mqtt_client,
      methods_response_topic_buffer,
      (const char*)az_span_ptr(response),
      az_span_size(response),
      MQTT_QOS1,
      NULL);
  if(msgId ==0)
  {
    logMsg("_sendMethodResponse: Failed to publish the Methods response: MQTTClient return code ");
  } else {
    logMsg("_sendMethodResponse done, msgId=", msgId);
  }
}

void _debugEvent(char *topic, char *msg) 
{
  Serial.print("topic=");Serial.println(topic);
  Serial.print("data=");Serial.println(msg);
}

bool _is_twinGetReply(az_span *topic) {
  return (az_span_find(*topic, twin_document_topic_request_id)>0);
}

bool _is_twinPatch(az_span *topic) {
  return (az_span_find( *topic, twin_patch_topic_request_id)>=0);
}
bool _is_method(az_span *topic) {
  return (az_span_find( *topic, methods_topic_request_id)>=0);
}
bool _is_c2d(az_span *topic) {
  return (az_span_find( *topic, directmsg_topic_request_id)>=0);
}
static uint8_t msgBuf[300];
static uint8_t topicBuf[200];
static char methodNameBuf[20];
static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event)
{
  int msgId;
  az_span topic = AZ_SPAN_FROM_BUFFER(topicBuf);
  az_span msg = AZ_SPAN_FROM_BUFFER(msgBuf);
  az_span method = AZ_SPAN_FROM_BUFFER(methodNameBuf);
  az_span slice;
  az_iot_hub_client_method_request methodReq;
  char responseBuf[100];
  int responseSize;
  
  logMsg("MQTT event ->", event->msg_id);

  slice = az_span_copy(topic, az_span_create((uint8_t*)event->topic, event->topic_len));
  az_span_ptr(slice)[0] = '\0';
  logMsgStr("\ttopic=", (char*)az_span_ptr(topic));

  slice = az_span_copy(msg, az_span_create((uint8_t*)event->data, event->data_len));
  az_span_ptr(slice)[0] = '\0';
  logMsgStr("\tdata=", (char*)az_span_ptr(msg));

  logMsg("\tdata len=", event->data_len);
  int i,r;
  
  switch (event->event_id)
  {
    case MQTT_EVENT_ERROR:
      logMsg("\tMQTT_EVENT_ERROR");
      logMsg(event->error_handle);
      break;
    case MQTT_EVENT_CONNECTED:
      logMsg("\tMQTT_EVENT_CONNECTED");

      r = esp_mqtt_client_subscribe(mqtt_client, AZ_IOT_HUB_CLIENT_C2D_SUBSCRIBE_TOPIC, 1);
      if (r == -1)
      {
        Logger.Error("Could not subscribe for cloud-to-device messages.");
      }
      else
      {
        Logger.Info("Subscribed for cloud-to-device messages; message id:"  + String(r));
        twinGetId = r;
      }


      r = esp_mqtt_client_subscribe(mqtt_client, AZ_IOT_HUB_CLIENT_TWIN_RESPONSE_SUBSCRIBE_TOPIC, 1);
      if (r == -1)
      {
        Logger.Error("Could not subscribe for twin response messages.");
      }
      else
      {
        Logger.Info("Subscribed for twin response messages; message id:"  + String(r));
        
      }

      r = esp_mqtt_client_subscribe(mqtt_client, AZ_IOT_HUB_CLIENT_METHODS_SUBSCRIBE_TOPIC, 1);
      if (r == -1)
      {
        Logger.Error("Could not subscribe for client-methods messages.");
      }
      else
      {
        Logger.Info("Subscribed for client-methods messages; message id:"  + String(r));
      }

      
      hasIoTHub = true;
      break;
    case MQTT_EVENT_DISCONNECTED:
      logMsg("\tMQTT_EVENT_DISCONNECTED");
      hasIoTHub = false;
      break;
    case MQTT_EVENT_SUBSCRIBED:
      logMsg("\tMQTT_EVENT_SUBSCRIBED");
      if(event->msg_id==twinGetId) {
        twinStatus = 1;
      }
      
      break;
    case MQTT_EVENT_UNSUBSCRIBED:
      logMsg("\tMQTT_EVENT_UNSUBSCRIBED");
      break;
    case MQTT_EVENT_PUBLISHED:
      logMsg("\tMQTT_EVENT_PUBLISHED");
      break;
    case MQTT_EVENT_DATA:
      logMsg("\tMQTT_EVENT_DATA");
      
      //_debugEvent( (char*)az_span_ptr(topic), (char*)az_span_ptr(msg));

      if(_is_twinGetReply(&topic)) {
        logMsg("\ttwin get  topic"); 
        _replyTwinGet( &msg);
      } else if( _is_twinPatch(&topic)) {
        logMsg("\ttwin patch  topic"); 
      } else if( _is_c2d(&topic)) {
        logMsg("\tc2d"); 
      } else if( _is_method(&topic)) {
        az_iot_hub_client_methods_parse_received_topic(&client, topic, &methodReq);
        az_span_to_str((char*)methodNameBuf, sizeof(methodNameBuf), methodReq.name);
        logMsgStr("\tdirectmethod, msg=", methodNameBuf);
        az_iot_status statCode = _handleMethod(methodNameBuf, responseBuf, sizeof(responseBuf));
        logMsgStr("\tdirectmethod, resp=", responseBuf);
        _sendMethodResponse(&methodReq, statCode, az_span_create_from_str(responseBuf));
      } else {
        logMsg("\tunexpected topic");
      }
      break;
    case MQTT_EVENT_BEFORE_CONNECT:
      logMsg("\tMQTT_EVENT_BEFORE_CONNECT");
      break;
    default:
      logMsg("\tUNKNOWN");
      break;
  }
  return ESP_OK;
}

/*
static int twinMsgId;
static bool _subscribeTwin() 
{
  int msgId = esp_mqtt_client_subscribe(mqtt_client, AZ_IOT_HUB_CLIENT_TWIN_RESPONSE_SUBSCRIBE_TOPIC, 1);
  
  if (msgId == -1)
  {
    Serial.println("Could not subscribe device twin topic");
    return false;
  }
  twinMsgId = msgId;
  logMsg("!! _subscribeTwin: ", msgId);
  return true;
}

static bool _subscribeC2DMessage()
{
  int msgId = esp_mqtt_client_subscribe(mqtt_client, AZ_IOT_HUB_CLIENT_C2D_SUBSCRIBE_TOPIC, MQTT_QOS1);
  if (msgId == -1)
  {
    Serial.println("Could not subscribe C2D topic");
    return false;
  }
  logMsg("!! _subscribeC2DMessage: ", msgId);
  return  true;
}

static bool _subscribeMethods()
{
  int msgId = esp_mqtt_client_subscribe(mqtt_client, AZ_IOT_HUB_CLIENT_METHODS_SUBSCRIBE_TOPIC, MQTT_QOS1);
  if (msgId == -1)
  {
    Serial.println("Could not subscribe METHODS topic");
    return false;
  }
  logMsg("!!_subscribeMethods: ", msgId);
  return  true;
}

*/

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

  unsigned long connStart = millis();
  while(!hasIoTHub&&(int)(millis()-connStart)<5000) {
    delay(10);
  }
  
  return hasIoTHub;
}

bool IotConn::isConnected() 
{
  return hasIoTHub;
}

void IotConn::close() 
{
  if(!hasIoTHub) return;
  esp_mqtt_client_disconnect(mqtt_client);
  unsigned long disconnStart = millis();
  while(hasIoTHub&&(int)(millis()-disconnStart)<5000) {
    delay(10);
  }
  (void)esp_mqtt_client_destroy(mqtt_client);

  hasIoTHub =false;

}

/*
bool IotConn::subscribeTwin() {
  return _subscribeTwin();
}
bool IotConn::subscribeC2D() {
  return _subscribeC2DMessage();
}
bool IotConn::subscribeMethods() {
  return _subscribeMethods();
}
*/
bool IotConn::requestTwinGet() {
  bool ret = false;
  if(twinStatus==1) {
    ret = _requestTwinGet(twinGetId);
    twinStatus = 2;
  }
  return ret;
}


bool IotConn::sendData() 
{
  if(!hasIoTHub) return false;

  char *msg = storage->getDataBuf();

  logMsg("sendData: telemetry:");
  logMsg(msg);


  // The topic could be obtained just once during setup,
  // however if properties are used the topic need to be generated again to reflect the
  // current values of the properties.
  if (az_result_failed(az_iot_hub_client_telemetry_get_publish_topic(
          &client, NULL, telemetry_topic, sizeof(telemetry_topic), NULL)))
  {
    logErr("sendData: Failed az_iot_hub_client_telemetry_get_publish_topic");
    return false;
  }
  az_span telemetry = az_span_create_from_str(msg);

  int msgId = esp_mqtt_client_publish(
          mqtt_client,
          telemetry_topic,
          (const char*)az_span_ptr(telemetry),
          az_span_size(telemetry),
          MQTT_QOS1,
          DO_NOT_RETAIN_MSG);
  if(msgId==0)
  {
    logErr("sendData: Failed publishing");
    return false;
  }
  else
  {
    logMsg("sendData: published successfully, msgId=", msgId);
    return true;
  }
}


az_iot_status _handleMethod(char *methodName, char *response, int response_size)
{
  const char *statFmt = "{\"status\":%d,\"payload\":\"%s\"}";
  char payloadBuf[300];
  snprintf(payloadBuf, strlen(payloadBuf), "%s", "OK");
  logMsgStr("_handleMethod: ", (char*)methodName);
  
  az_iot_status iotStatus = AZ_IOT_STATUS_OK;
  int result = 200;
  if (strcmp(methodName, "meas") == 0)
  {
    logMsg("\tsend measurements");
    storage->getMeasurementString(payloadBuf, sizeof(payloadBuf));
  } 
  else if (strcmp(methodName, "led on") == 0)
  {
    logMsg("\tled on");
    led->ledOn();
  }
  else if (strcmp(methodName, "led off") == 0)
  {
    logMsg("\tled off");
    led->ledOff();
  }
  else if (strcmp(methodName, "7seg on") == 0)
  {
    logMsg("\tsevenseg on");
    deviceState->setSevenSegTime(false);
    deviceState->setSevenSegOn(true);
    if(sevenSeg!=NULL && sevenSeg->isConnected()) {
      sevenSeg->printHex(0xBEEF);
    }
  }
  else if (strcmp(methodName, "7seg off") == 0)
  {
    logMsg("\tsevenseg off");
    deviceState->setSevenSegTime(false);
    deviceState->setSevenSegOn(false);
    if(sevenSeg!=NULL && sevenSeg->isConnected()) {
      sevenSeg->clear();
    }
  }else if (strcmp(methodName, "7seg time") == 0)
  {
    logMsg("\tsevenseg time");
    deviceState->setSevenSegTime(true);
    deviceState->setSevenSegOn(true);
    if(sevenSeg!=NULL && sevenSeg->isConnected()) {
      sevenSeg->showTime();
    }
  }else if (strcmp(methodName, "stat") == 0)
  {
    const char *tpl = "wakeNum:%d, freeMem: %d, storedMeas: %d, freeMeasBuf: %d";
    snprintf(payloadBuf, sizeof(payloadBuf), tpl, 
      wakeCnt, (int)esp_get_free_heap_size(), storage->getNumStoredMeasurements(), storage->getRemainingLen()
    
    );
  }
  else
  {
    logMsgStr("_handleMethod: No method found: ", (char*)methodName);
    snprintf(payloadBuf, strlen(payloadBuf), "%s", "FAIL");
    result = 404;
    iotStatus = AZ_IOT_STATUS_BAD_REQUEST;
  }
  
  snprintf(response, response_size, statFmt, result, payloadBuf );

  return iotStatus;
}
