# EdgeTX/OpenTX Bluetooth BLE Gamepad  v1.0

Copyright (C) by mosch   
License GPLv2: http://www.gnu.org/licenses/gpl-2.0.html       
GITHUB: https://github.com/moschotto?tab=repositories 

![Alt text](https://github.com/moschotto/OpenTX_EdgeTX_Bluetooth_GamePad/blob/main/Media/front.png)
![Alt text](https://github.com/moschotto/OpenTX_EdgeTX_Bluetooth_GamePad/blob/main/Media/IMG_0454.jpg)

EdgeTX/OpenTX bluetooth GamePad module based on ESP32 that decodes SBUS frames... 
Some quick testing on PC only. Connection to MACs works as well but not tested in-game

Basically you can use any ESP32 and any radio that outputs SBUS and support the mentioned libraries down below.

The code is very simple because the used libraries are easy to use and there is not much to do. I encountered some problems with the SBUS library but added a workaround (see comments in the code if you are intereseted)

Note: I did not test any latency or input lags because everything depends on the BLE game pad / nimble libraries!

### Wiring:

![Alt text](https://github.com/moschotto/OpenTX_EdgeTX_Bluetooth_GamePad/blob/main/Media/wiring-diagram.png)

- The LED is optional
- According to the getting started guide <a href="https://wiki.seeedstudio.com/XIAO_ESP32C3_Getting_Started/">click here</a>, i added a schottky diode to the 5V input of the ESP32. In case that the module is powered by the radio and you plug in USB-c in parallel, the voltage/current can't flow back to the external voltage regulator
- The SBUS input (RX input / yellow) can be any GPIO pin. Adjust the code aacordingly -> "define RX_PIN 20"

### Install / uploading the code:

#### Arduino IDE:

Add the latest boards if you don't already have it. If you are using an ESP32C3, be aware that you need to put it into "bootloader" mode!

- hold the BOOT BUTTON, connect the board to your PC while holding the BOOT button, and then release it to enter bootloader mode
- The device should appear in the device manager as "USB JTAG/serial debug unit (Interface 0) (COM3)" 
- Driver libwdi / USB\VID_303A&PID_1001

After that you should be able to upload the code.

#### Platform.io

If you have the right settings in platform.io, you don't need to put the ESP32C3 into bootloader mode manually.

Espressif32 version 5.1.1 works for me.

adjust the "uploads section" in the boards file for the ESP32C3 accordingly (usb_reset):

```
C:\Users\<usernmae>\.platformio\platforms\espressif32\boards\seeed_xiao_esp32c3.json
  
  "upload": {
    "flash_size": "4MB",
    "maximum_ram_size": 327680,
    "maximum_size": 4194304,
    "require_upload_port": true,
    "speed": 460800,
	"before_reset": "usb_reset",
	"after_reset": "hard_reset"
```



```
plattformio.ini
[env:seeed_xiao_esp32c3]
platform = https://github.com/platformio/platform-espressif32.git
board = seeed_xiao_esp32c3
framework = arduino
upload_protocol = esptool
upload_speed = 921600
upload_port = COM8
monitor_speed = 115200
monitor_port = COM8
build_flags = 
	-DARDUINO_USB_CDC_ON_BOOT=1
	-DARDUINO_USB_MODE=1
lib_deps = 
	h2zero/NimBLE-Arduino@^2.3.0
	fastled/FastLED@^3.9.19
	https://github.com/lemmingDev/ESP32-BLE-Gamepad


```
### Radio setup:

Input setup (Radiomaster Zorrow). Feel free to try out other radios as well

![Alt text](https://github.com/moschotto/OpenTX_EdgeTX_Bluetooth_GamePad/blob/main/Media/SBUS.png)
![Alt text](https://github.com/moschotto/OpenTX_EdgeTX_Bluetooth_GamePad/blob/main/Media/Mixer1.png)
![Alt text](https://github.com/moschotto/OpenTX_EdgeTX_Bluetooth_GamePad/blob/main/Media/Mixer2.png)


### Demo
<a href="http://www.youtube.com/watch?feature=player_embedded&v=TKDMgXqkkfQ" target="_blank"><img src="http://img.youtube.com/vi/TKDMgXqkkfQ/0.jpg" 
alt="IMAGE ALT TEXT HERE" width="240" height="180" border="10" /></a>

### Libraries:

Check out and download/install these libraries (thanks to the contributors)

```
|-- Bolder Flight Systems SBUS @ 7.0.0
|-- ESP32-BLE-Gamepad @ 0.5.0
|   |-- NimBLE-Arduino @ 1.4.0
|-- FastLED @ 3.5.0
|   |-- EspSoftwareSerial @ 6.13.2
|   |-- SPI @ 2.0.0

```
https://www.arduino.cc/reference/en/libraries/bolder-flight-systems-sbus/

https://github.com/lemmingDev/ESP32-BLE-Gamepad

https://github.com/h2zero/NimBLE-Arduino

https://github.com/FastLED/FastLED

### Parts used:

- 1 x Seeed Studio XIAO ESP32C3
- 1 x Ams1117 5.0V voltage regulator 
- 1 x 8 pin 2.54mm female pin headers
- 1 x schottky-diode or signal-diode, power-diode

### Printable case / housing:
i.e. for RADIOMASTER Zorrow  
https://www.thingiverse.com/thing:5515977

### Changelog

#### 20250601  Eisbaeeer ####
  - Added neeeded libs to lib directory
  - Added needed libs to platformio.ini
  - Added firmware to binary folder
  - Added Bluetooth disconnect button
  Now it´s possible to un-pair / delete the bluetooth connection. You must add a pushbutton on GPIO9.
  - Added OTA update via Wifi
  You can update new firmware via Wifi. Connect to Wifi EdgeTX and open 192.168.4.1/update
