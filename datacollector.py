import bme280
import blobclient
from time import sleep 
from datetime import datetime
import os
import RPi.GPIO as GPIO

def main(bc, bme280, led, lcd):

    sensor_id = os.getenv("SENSOR_ID")
    (chip_id, chip_version) = bme280.readID()
    print(f"Started, chip_id={chip_id}, chip_version={chip_version}")

    while True:
        temperature,pressure,humidity = bme280.readAll()
        measTime = datetime.now().isotime()
        measRow = ",".join([measTime, sensorId, temperature, pressure, humidity])
        blobName = bc.createBlobName()
        bc.appendToBlob(blobName, measRow)
        
        lcdOut = f"T:{temperature:0.1f}C {pressure:0.1f}hPa\nH:{humidity:0.1f}%"
        lcd.clear()
        lcd.setCursor(0,0)
        lcd.message(lcdOut)

        led.blinkLed()
        sleep(60)

if __name__=="__main__":
   try:
        bc = BlobClient()
        bme280 = BME280()
        led = Led()
        lcd = LCD()
		main(bc, bme280, led, lcd)
	except KeyboardInterrupt:
        bc.close()
        led.close()
        lcd.clear()
        
        lcd.setCursor(0,0)
        lcd.destroy()

		print("exiting...")
