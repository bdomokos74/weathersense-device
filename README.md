## Weather sensor with BME280 + LCD + Azure IoT Hub

Sensor device app to send measurement data to Azure IoT Hub

### Other components
* https://github.com/bdomokos74/weathersense-spa - Single Page Application to show the collected data

* https://github.com/bdomokos74/weathersense-collect - Function app to collect sensor measurements from Azure IoT HUB and save it to Azure Blob. Also generates aggregated data, and sends out notifications.

### TODO
* Change data format, to allow device specific data e.g. battery voltage, retry count,...

* Add battery voltage measurement

* Save battery life by waking up less frequetly, and batch-sending measurements

* OTA update of device software



### Raspberry Pi 4B and LCD1602 connections

|BME280|Raspberry Pi|
|---|---|
|D7|GPIO17|
|D6|GPIO18|
|D5|GPIO27|
|D4|GPIO23|
|E|GPIO22|
|RS|GPIO25|

![wsense1.jpg](wsense1.jpg?raw=true "wsense1")

![wsense2.jpg](wsense2.jpg?raw=true "wsense2")

### Notes

http://wiki.sunfounder.cc/index.php?title=LCD1602_Module

https://www.raspberrypi-spy.co.uk/2016/07/using-bme280-i2c-temperature-pressure-sensor-in-python/
