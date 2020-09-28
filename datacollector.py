from bme280 import BME280
from led import Led
from lcd1602 import LCD
from blobclient import BlobClient
from time import sleep 
from datetime import datetime
import os
import RPi.GPIO as GPIO

def main(bc, bme280, led, lcd):
    sensorId = os.getenv("SENSOR_ID")
    (chip_id, chip_version) = bme280.readID()
    print(f"Started, chip_id={chip_id}, chip_version={chip_version}")

    while True:
        temperature,pressure,humidity = bme280.readAll()
        measTime = datetime.now().isoformat()
        measRow = f"{measTime},{sensorId},{temperature:0.1f},{pressure:0.1f},{humidity:0.1f}"
        print(measRow)
        
        blobName = bc.createBlobName()
        print("append to"+blobName)
        bc.appendToBlob(blobName, measRow)
        
        lcdOut = f"T:{temperature:0.1f}C {pressure:0.1f}hPa\nH:{humidity:0.1f}%"
        lcd.clear()
        lcd.setCursor(0,0)
        lcd.message(lcdOut)

        led.blinkLed()
        sleep(2*60)

if __name__=="__main__":
   try:
        bc = BlobClient()
        bme280 = BME280()
        led = Led()
        lcd = LCD()
        main(bc, bme280, led, lcd)
   except KeyboardInterrupt:
        print("exiting...")
        bc.close()
        led.close()
        lcd.clear()
        
        lcd.setCursor(0,0)
        lcd.destroy()

        
   
