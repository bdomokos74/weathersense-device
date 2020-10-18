import logging
import json, os
import azure.functions as func
from datetime import datetime
from azure.storage.blob import BlobServiceClient
from azure.identity import ClientSecretCredential, ManagedIdentityCredential, DefaultAzureCredential, CredentialUnavailableError
from azure.core.exceptions import ResourceNotFoundError

def main(event: func.EventHubEvent):
    def createBlobName(prefix="meas", sensorId="1", extension=".txt"):
        datePart = datetime.now().strftime("%Y%m%d")
        return "-".join([prefix, sensorId, datePart])+extension

    def createRecord(msg):
        timestamp = datetime.now().isoformat()
        temp = msg.get("Temperature", "")
        pressure = msg.get("Pressure", "")
        humidity = msg.get("Humidity", "")
        return f"{timestamp},{temp},{pressure},{humidity}\n"

    containerName = "weathersense-data"
    logging.info("called WeatherMeasTrg")
    eventString = event.get_body().decode('utf-8')
    msg = json.loads(eventString)
    # {"messageId":16, "Temperature":24.437500}
    
    logging.info(msg)
    #logging.info("meta:")
    #logging.info(event.metadata)

    sensoreId = "default"
    sensoreId = event.metadata.get("SystemProperties", {}).get("iothub-connection-device-id", None)
    if sensoreId == None:
        raise Error("Missing device id")

    #blobConnStr = os.getenv("BLOB_CONN_STR")

    storageName = "weathersenseblob"
    #storageName = os.getenv("OAUTH_STORAGE_ACCOUNT_NAME")
    oauth_url = "https://{}.blob.core.windows.net".format( storageName )
    
    #ad_application_id = os.getenv("AD_APPLICATION_ID")
    #ad_application_secret = os.getenv("AD_APPLICATION_SECRET")
    #ad_tenant_id = os.getenv("AD_TENANT_ID")
    #token_credential = ClientSecretCredential(
    #        ad_tenant_id,
    #        ad_application_id,
    #        ad_application_secret
    #    )

    cid = os.getenv("MANAGED_ID")
    try:
        token_credential = ManagedIdentityCredential(client_id=cid)
    except CredentialUnavailableError as ex:
        log.info("FAILED CRED---------")
        log.info(ex)

    #token_credential = DefaultAzureCredential()
    bsc = BlobServiceClient(oauth_url, credential=token_credential)

    containerClient = bsc.get_container_client(containerName)
    blobName = createBlobName(prefix="meas", sensorId=sensoreId)
    blobClient = containerClient.get_blob_client(blobName)
    record = createRecord(msg)

    logging.info(f"appending to {blobName}: {record}")
    retryCnt = 0
    succ = False
    while not succ and retryCnt<2:
        try:
            ret = blobClient.append_block(record)
            succ = True
        except ResourceNotFoundError:
            retryCnt += 1
            blobClient.create_append_blob()
            logging.info(f"created {blobName}")
    
    #logging.info('Python EventHub trigger processed an event: %s', event.get_body().decode('utf-8'))
