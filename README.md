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

#### Run raspi datacollector
```bash
cd raspi
export DEV_BME_CONN_STR=... # Azure IoT Device connection string
python3 -m venv .venv
. ./.venv/bin/activate
pip install -r requirements.txt
python datacollector.py -
```

#### DOIT1

- BME_ADDR 0x76
- DOIT ESP32 DEVKIT V1

![raspiDevice1.jpg](pic/raspiDevice1.jpg?raw=true "raspiDevice1")

![raspiDevice2.jpg](pic/raspiDevice2.jpg?raw=true "raspiDevice2")

![esp32Device1.jpg](esp32Device1.jpg?raw=true "esp32Device1")

### Notes

ESP32 sleep-wakeup
https://lastminuteengineers.com/esp32-deep-sleep-wakeup-sources/
https://iotassistant.io/esp32/enable-hardware-watchdog-timer-esp32-arduino-ide/

ESP32 flash memory
https://randomnerdtutorials.com/esp32-save-data-permanently-preferences/

https://docs.microsoft.com/en-us/samples/azure-samples/esp32-iot-devkit-get-started/sample/

Issues with VS Code IoT Workbench:
https://cuneyt.aliustaoglu.biz/en/enabling-arduino-intellisense-with-visual-studio-code/
In case .h files are not found, change settings to:
"intelliSenseEngine": "Tag Parser"