from azure.identity import ClientSecretCredential
import os
from datetime import datetime
from azure.storage.blob import BlobServiceClient

# Writing directly to Azure Blobs

class BlobClient:
    '''
    Usage: AD_APPLICATION_ID=<appid> AD_APPLICATION_SECRET=<appsecret> AD_TENANT_ID=<tenant_id> OAUTH_STORAGE_ACCOUNT_NAME=<storageaccname> python3 blobtest.py
    '''
    
    ad_application_id = os.getenv("AD_APPLICATION_ID")
    ad_application_secret = os.getenv("AD_APPLICATION_SECRET")
    ad_tenant_id = os.getenv("AD_TENANT_ID")
    oauth_url = "https://{}.blob.core.windows.net".format(
            os.getenv("OAUTH_STORAGE_ACCOUNT_NAME")
        )

    def __init__(self, containerName="weathersense-data"):
        self._blobServiceClient = self._createBlobServiceClient()
        self.containerName = containerName
        
        print("blob_service_client created")

    def _createBlobServiceClient(self):
        token_credential = ClientSecretCredential(
            self.ad_tenant_id,
            self.ad_application_id,
            self.ad_application_secret
        )

        # Instantiate a BlobServiceClient using a token credential
        blob_service_client = BlobServiceClient(account_url=self.oauth_url, credential=token_credential)
        return blob_service_client

    @staticmethod
    def createBlobName(prefix="meas", sensorId="1", extension=".txt"):
        datePart = datetime.now().strftime("%Y%m%d")
        return "-".join([prefix, sensorId, datePart])+extension

    def createMeasBlob(self, newBlobName, debug=False):
        #account_info = self._blobServiceClient.get_service_properties()
        #print("acc info:") 
        #print(account_info)
        
        containerClient = self._blobServiceClient.get_container_client(self.containerName)
        blobClient = containerClient.get_blob_client(newBlobName)
        blobProp = blobClient.create_append_blob()
        if debug:
            print(f"create_append_blob [{newBlobName}]")
            print(blobProp)

    def listBlobs(self):
        containerClient = self._blobServiceClient.get_container_client(self.containerName)
        blobs = containerClient.list_blobs()
        idx = 0
        for b in blobs:
            print(f"{idx}: name:[{b.name}] lastmodified:[{b.last_modified.isoformat()}] size:[{b.size}]")

    def readBlob(self, blobName):
        containerClient = self._blobServiceClient.get_container_client(self.containerName)
        blobClient = containerClient.get_blob_client(blobName)
        download_stream = blobClient.download_blob()
        print(f"content of {blobName}:\n{download_stream.readall()}")
    
    def deleteBlob(self, blobName, debug=False):
        containerClient = self._blobServiceClient.get_container_client(self.containerName)
        blobClient = containerClient.get_blob_client(blobName)
        ret = blobClient.delete_blob()
        if debug: print(f"{ret}")

    def appendToBlob(self, blobName, block, debug=False):
        containerClient = self._blobServiceClient.get_container_client(self.containerName)
        blobClient = containerClient.get_blob_client(blobName)
        ret = blobClient.append_block(block+"\n")
        if debug: print(f"{ret}")

    def close(self):
        ret = self._blobServiceClient.close()
        print(f"{ret}")
        
if __name__ == "__main__":
    pass
    #BlobClient().appendToBlob()
