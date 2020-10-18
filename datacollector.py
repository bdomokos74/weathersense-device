from bme280 import BME280
from led import Led
from lcd1602 import LCD
from azure.iot.device.aio import IoTHubDeviceClient
from time import sleep 
from datetime import datetime
import os
import RPi.GPIO as GPIO
import asyncio

def main(bc, bme280, led, lcd):
    sensorId = os.getenv("SENSOR_ID")
    (chip_id, chip_version) = bme280.readID()
    print(f"Started, chip_id={chip_id}, chip_version={chip_version}")
    
    conn_str = os.getenv("DEV_BME_CONN_STR")
    device_client = IoTHubDeviceClient.create_from_connection_string(conn_str)
    await device_client.connect()

    while True:
        temperature,pressure,humidity = bme280.readAll()
        measTime = datetime.now().isoformat()
        measRow = f"{measTime},{temperature:0.1f},{pressure:0.1f},{humidity:0.1f}"

        print("Sending message: {measRow}")
        await device_client.send_message(measRow)
        

        lcdOut = f"T:{temperature:0.1f}C {pressure:0.1f}hPa\nH:{humidity:0.1f}%"
        lcd.clear()
        lcd.setCursor(0,0)
        lcd.message(lcdOut)

        led.blinkLed()
        sleep(2*60)

     #await device_client.disconnect()

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

        
   
