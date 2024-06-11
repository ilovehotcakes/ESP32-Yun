# ESP32 Yun
An all-in-one hardward & software solution for motorizing your smart home.

ESP32 Yun is an affordable, reliable, and user-friendly wireless stepper motor controller that works with two-phase bipolar stepper motors. It comes with a web user-interface for intuitive controls and HTTP restful APIs for easy integrations. Please see [demo video](https://user-images.githubusercontent.com/52260129/211658800-c67d9bb7-6f65-4ab0-a19c-eaa4f9b99e2e.mp4) for full demonstration.

Under active development.

[![PlatformIO CI](https://github.com/ilovehotcakes/ESP32-Yun/actions/workflows/build.yml/badge.svg)](https://github.com/ilovehotcakes/ESP32-Yun/actions/workflows/build.yml)


## Features
* WiFi-based: no need for extra hub
* Closed-loop system: motor can be manually moved without losing track of its position
* Automatic stalling detection and stopping: preventing injuries and protecting the motor
* Custom hardware: few parts, cheap and relatively easy to assemble
* Web UI: no need to download app to control motor or change settings, works on desktop/mobile devices
* HTTP restful APIs: easy to create integrations for smart home platforms
* Extremely quiet, won't even wake a baby!
* Can go really fast!


## Building your own
There are three components: electronics, firmware, and mounting hardware.

### Requirements:
* [USB-to-TTL serial adatper](https://www.amazon.com/dp/B07WX2DSVB) to upload the firmware
* Soldering iron, wires, and [JST PH connectors](https://www.amazon.com/dp/B0731MZCGF)/[crimper](https://www.amazon.com/dp/B00YGLKBSK)
* Two-phase bipolar stepper motor, such as a [Nema 11](https://www.amazon.com/dp/B00PNEPK94/)
* Magnet for the rotary encoder
* Power supply: 3.3-9V, 1.2A max
* (Optional) 3D printer for the motor mount

### 1. Electronics
Please refer to the [*electronics*](https://github.com/ilovehotcakes/ESP32-Yun/electronics) folder.

### 2. Firmware
Flashing the firmware via [VSCode](https://code.visualstudio.com/) + [PlatformIO](https://platformio.org/install/ide?install=vscode).

#### Dependencies:
* espressif32@3.5.0
* [TMCStepper@^0.7.3](https://github.com/teemuatlut/TMCStepper)
* [FastAccelStepper@^0.27.5](https://github.com/gin66/FastAccelStepper)
* [robtillaart/AS5600@^0.4.1](https://github.com/RobTillaart/AS5600)
* [ESPAsyncWebServer](https://github.com/me-no-dev/ESPAsyncWebServer)
* [ArduinoJson@^7.0.4](https://github.com/bblanchon/ArduinoJson/tree/v7.0.4)
* [LittleFS_esp32@^1.0.6](https://github.com/lorol/LITTLEFS/tree/1.0.6)

#### Steps:
1. Clone this repository.
2. Double check and set proper current sense resistor value in **platformio.ini**.
3. Set the USB-to-TTL serial adatper's logic level to **3V3** and plug it in to the computer.
4. Keep holding the button on ESP32 Yun and start flashing the firmware.
5. Once the IDE begins data transmition, connect the four wires from ESP32 Yun to the adapter: **RX to TXD**, **TX to RXD**, **+ to VCC**, and **− to GND**.
6. Let go of the button once the firmware starts uploading. Unplug ESP32 motorcover after the firmware has finished uploading.

### 3. Mounting hardware
You can find the stl and pre-sliced files under the [*cad*](cad/) folder. To mount the magnet for the rotary encoder, it is recommended to use the manget gluing jig to make sure that the magnet is centered on the axis-of-rotation; otherwise, it could affect the accuracy of the rotary encoder.

#### Parts:
* Nema 11 motor mount: [[**stl**](https://github.com/ilovehotcakes/ESP32-Motorcover/blob/main/cad/nema_11_motor_mount.stl)]
* AS5600 magnet gluing jig: [[**stl**](https://github.com/ilovehotcakes/ESP32-Motorcover/blob/main/cad/prototype/magnet_gluing_jig_v1.stl)]
* AS5600 rotary encoder mount: [[**stl**](https://github.com/ilovehotcakes/ESP32-Motorcover/blob/main/cad/as5600_mount.stl)]

<p align="center">
    <img src="https://github.com/ilovehotcakes/ESP32-Motorcover/blob/main/images/cad/v1_0/nema_11_motor_mount_front.png" width="400"/>
    <img src="https://github.com/ilovehotcakes/ESP32-Motorcover/blob/main/images/cad/v1_0/nema_11_motor_mount_back.png" width="400">
    </br>
    <img src="https://github.com/ilovehotcakes/ESP32-Motorcover/blob/main/images/cad/v1_0/IMG_2195.jpg" width="400">
    <img src="https://github.com/ilovehotcakes/ESP32-Motorcover/blob/main/images/cad/v1_0/IMG_2196.jpg" width="400">
</p>


## Usage
During the first time booting up, ESP32 Yun is put into setup mode and it functions as a WiFi access point. Connect to it with your device like you would connect to a WiFi network. After connection is established, open a web browser and go to the IP address **[192.168.4.1]()** to access the web UI. There you can enter your WiFi network credentials and change other settings.

### HTTP Restful API
All RestAPIs are implemented using HTTP GET requests. To control the motor or change any settings, use [http://&lt;ESP32-YUN-IP-ADDRESS&gt;/&lt;URI&gt;?&lt;PARAM&gt;=&lt;VALUE&gt;](). For example: [http://192.168.4.1/motor?percent=100]() There are three URIs: motor, system, and wireless.

### Motor parameters:
* stop: stop the motor
* percent: move the motor to the specified percentage
* step: move the motor to the specified step
* forward: run the motor foward continuously
* backward: run the motor backward continuously
* set-min: set the beginning endpoint
* set-max: set the ending endpoint
* zero: set the current motor position to zero
* standby: put the motor driver into standby to reduce power consumption
* sync-settings: sync open/closing settings for current, velocity, and acceleration 
* velocity: set both opening and closing velocity
* opening-velocity: set opening velocity
* closing-velocity: set closing velocity
* acceleration: set opening and closing acceleration
* opening-acceleration: set opening acceleration
* closing-acceleration: set closing acceleration
* current: set opening and closing current
* opening-current: set opening current
* closing-current: set closing current
* direction: change the direction of the motor
* full-steps: set the number of full steps per turn
* microsteps: set the number of subvisions per full step
* stallguard: enable/disable stallguard
* coolstep-threshold: set threshold to enable stallguard and coolstep
* stallguard-threshold: set threshold to trigger stallguard
* fastmode: set to exclusively use fastmode, i.e. SpreadCycle
* fastmode-threshold: set threshold to automatically switch over to fastmode

### System parameters:
* sleep: put ESP32 Yun into standby
* restart: restart the system
* reset: factory reset system
* name: rename the system

### Wireless parameters:
* setup: setup mode
* ssid: ssid of your WiFi network
* password: passowrd of your WiFi network

### Json
Use HTTP GET request [http://&lt;ESP32-YUN-IP-ADDRESS&gt;/json]() to get all setting in a Json object.

### Button
* Toggle setup mode: press and hold down for 5 seconds till the led turns on
* Factory reset: press and hold down for 15 seconds till the led turns off after it turns on at 5 seconds

### (Optional) Tuning StallGuard4
StallGuard4(SG) is a feature of the motor driver, TMC2209, which provides automatic stall detection and stopping. SG requires some trial-and-error as well as some patience to get it working as intended. Please refer to the [TMC2209 datasheet](https://www.analog.com/media/en/technical-documentation/data-sheets/TMC2209_datasheet_rev1.09.pdf), chapter 16, page 70.


## In the Works
* Write custom drivers to reduce dependency
* Updating PCB so little to no soldering is required
* Replace ESP32 WROOM 32E so USB-to-TTL is not required to flash the firmware
* BLE support
