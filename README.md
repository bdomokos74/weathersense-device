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


#### Set up env

https://github.com/Azure/azure-iot-arduino

Under review... -> 
https://github.com/Azure/azure-sdk-for-c/tree/main/sdk/samples/iot/aziot_esp32

With Arduino ESP32 2.0.0 board version, it crashes.. downgrade to 1.6.0

To check the backtrace, use:
https://github.com/me-no-dev/EspExceptionDecoder

Output:

  Decoding stack results
  0x400e0811: panic_abort at /home/runner/work/esp32-arduino-lib-builder/esp32-arduino-lib-builder/esp-idf/components/esp_system/panic.c line 402
  0x4008bec9: esp_system_abort at /home/runner/work/esp32-arduino-lib-builder/esp32-arduino-lib-builder/esp-idf/components/esp_system/esp_system.c line 129
  0x40091551: abort at /home/runner/work/esp32-arduino-lib-builder/esp32-arduino-lib-builder/esp-idf/components/newlib/abort.c line 46
  0x401624ca: _Unwind_Resume at /builds/idf/crosstool-NG/.build/HOST-x86_64-apple-darwin12/xtensa-esp32-elf/src/gcc/libgcc/unwind.inc line 247
  0x400d3411: mqtt_event_handler(esp_mqtt_event_handle_t) at /Users/balintdomokos/VSProjects/azure-sdk-for-c/sdk/samples/iot/aziot_esp32/aziot_esp32.ino line 132
  0x4010c105: esp_mqtt_dispatch_event at /home/runner/work/esp32-arduino-lib-builder/esp32-arduino-lib-builder/esp-idf/components/mqtt/esp-mqtt/mqtt_client.c line 922
  0x4010c145: esp_mqtt_dispatch_event_with_msgid at /home/runner/work/esp32-arduino-lib-builder/esp32-arduino-lib-builder/esp-idf/components/mqtt/esp-mqtt/mqtt_client.c line 913
  0x4010c7ca: esp_mqtt_task at /home/runner/work/esp32-arduino-lib-builder/esp32-arduino-lib-builder/esp-idf/components/mqtt/esp-mqtt/mqtt_client.c line 1371

Zip libs location for Arduino, can be added: Scratch/Libraries/
/Users/<username>/Documents/Arduino/libraries/

#### DOIT1

- BME_ADDR 0x76
- DOIT ESP32 DEVKIT V1

![raspiDevice1.jpg](pic/raspiDevice_1.jpg?raw=true "raspiDevice1")

![raspiDevice2.jpg](pic/raspiDevice_2.jpg?raw=true "raspiDevice2")

![esp32Device1.jpg](pic/esp32Device_1.jpg?raw=true "esp32Device1")

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

Also for monitor etc.:
    "C_Cpp.intelliSenseEngine": "Tag Parser",
    "IoTWorkbench.DevicePath": "Device",
    "IoTWorkbench.BoardId": "esp32",
    "arduino.path": "/Applications",
    "arduino.commandPath": "Contents/MacOS/arduino"

https://github.com/Azure/azure-sdk-for-c/blob/main/sdk/docs/iot/mqtt_state_machine.md

- GM counter
https://github.com/2969773606/GeigerCounter-V1.1
151CPM=1uSv/h for M4011 GM Tube

 connect the P3 Pin 1 2 3 to arduino GND,5V, Digital 2 respectively.