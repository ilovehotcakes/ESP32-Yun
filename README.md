# ESP32 Motorcover
An open-source and DIY-friendly solution to motorize windows/blinds for your smart homes.

ESP32 motorcover is an affordable, reliable, and user-friendly closed-loop wireless stepper motor controller that works with NEMA stepper motors and other bipolar stepper motors.

Under active development. **Warning:** although it's a beginner-friendly DIY project, it still requires some soldering and computer skills. Please follow the guide at your own risk. This project is being actively developed to further lower the barrier-to-entry.


## Design
In [**Home Assistant**](https://www.home-assistant.io/), a [**cover**](https://www.home-assistant.io/integrations/cover/) is a type of entity that could be a blind, shade, shutter, window, garage door, etc. A motorized cover provides the ability to control your covers through your choice of smarthome hub/system (HA, Google Home, Alexa, etc.) through iOS/Android apps, voice control, or automations. Please see the [[**Demo video**](https://user-images.githubusercontent.com/52260129/211658800-c67d9bb7-6f65-4ab0-a19c-eaa4f9b99e2e.mp4)] for example.

### **Features:**
* Works with WiFi, no need for extra hub
* Closed-loop system, making the motor movements extremely precise and repeatable
* Automatic stalling detection and stopping, preventing injuries and protecting the motor
* Extremely quiet, won't even wake a baby!
* Few parts, cheap and easy to assemble


## Building your own
There are three main components to consider: electronics, firmware, and motor + mounting hardware.

### Requirements:
* Computer and [USB-to-TTL serial adatper](https://github.com/ilovehotcakes/ESP32-Motorcover/blob/pcb-v1.0/electronics/v1_1/BOM_ESP32-Motorcover_2024-03-27.csv) to upload the firmware
* Soldering iron
* Wires and JST PH connectors/crimper
* (Optional) 3D printer for the motor mount and coupling

### 1. Electronics
There are two ways to build your own: **(1)** directly order printed circuit boards from JLCPCB/PCBWay or **(2)** buying parts from Amazon and putting them together on a breadboard.

#### Option (1) ordering PCB from JLCPCB:
1. Download the [[**gerber files**](https://github.com/ilovehotcakes/ESP32-Motorcover/blob/pcb-v1.0/electronics/v1_1/Gerber_ESP32-Motorcover_PCB_ESP32-Motorcover_2024-03-27.zip)] and [[**bom**](https://github.com/ilovehotcakes/ESP32-Motorcover/blob/pcb-v1.0/electronics/v1_1/BOM_ESP32-Motorcover_2024-03-27.csv)].
2. Go to [JLCPCB.com](https://jlcpcb.com/) and upload the gerber files. The only setting that needs to be changed is the **"Impedance Control"**. Select **"Yes"** and choose **"JLC0416H-3313"** once the dialog pops up.
3. If you prefer to manually assemble the PCB, please refer to the [[**schematic**](https://github.com/ilovehotcakes/ESP32-Motorcover/blob/pcb-v1.0/electronics/v1_1/Schematic_ESP32-Motorcover_2024-03-27.png)] and bom.
4. If you prefer to have the PCB assembled by JLCPCB(additional cost), download the [[**pick-and-place file**](https://github.com/ilovehotcakes/ESP32-Motorcover/blob/pcb-v1.0/electronics/v1_1/PickAndPlace_PCB_ESP32-Motorcover_2024-03-27.csv)] and then toggle **"PCB Assembly"**. Click on **"Confirm"** to go the next page.
5. Upload the bom and pick-and-place files. Click **"Process BOM & CPL"** and **"Continue"** when the error pops up. The error is for the missing connectors which we will manually solder once we receive the PCBs.
<p align="center">
    <img src="https://github.com/ilovehotcakes/ESP32-Motorcover/blob/pcb-v1.0/images/electronics/v1_1/v1_1_top.png" width="400"/>
    <img src="https://github.com/ilovehotcakes/ESP32-Motorcover/blob/pcb-v1.0/images/electronics/v1_1/v1_1_bot.png" width="400">
    </br>
    <img src="https://github.com/ilovehotcakes/ESP32-Motorcover/blob/pcb-v1.0/images/electronics/v1_1/v1_1_top_3d.png" width="400">
    <img src="https://github.com/ilovehotcakes/ESP32-Motorcover/blob/pcb-v1.0/images/electronics/v1_1/v1_1_bot_3d.png" width="400">
</p>


#### Option (2) breadboarding with breakout boards:
This could be more approachable if you don't solder. You can get breakout board modules and assemble them on a solderless breadboard. Please find the bill of materials [here](https://github.com/ilovehotcakes/ESP32-Motorcover/blob/pcb-v1.0/electronics/prototype/bom.csv) and reference the [schematic](https://github.com/ilovehotcakes/ESP32-Motorcover/blob/pcb-v1.0/electronics/v1_1/Schematic_ESP32-Motorcover_2024-03-27.png) to put the circuit together. An example assembly looks like [this](https://github.com/ilovehotcakes/ESP32-Motorcover/blob/pcb-v1.0/images/electronics/prototype/assembled_controller.jpg).


### 2. Firmware
Your choice of IDE to program the firmware, e.g. [Arduino IDE](https://www.arduino.cc/en/software) or [ESP-IDF](https://idf.espressif.com/). I use [VSCode](https://code.visualstudio.com/) + [PlatformIO](https://platformio.org/install/ide?install=vscode).

#### Dependencies:
* espressif32@3.5.0
* [TMCStepper@^0.7.3](https://github.com/teemuatlut/TMCStepper)
* [PubSubClient@^2.8](https://github.com/knolleary/pubsubclient)
* [FastAccelStepper@^0.27.5](https://github.com/gin66/FastAccelStepper)
* [robtillaart/AS5600@^0.4.1](https://github.com/RobTillaart/AS5600)

#### Steps:
1. Install dependencies.
2. Add WiFi and MQTT credentials to secrets.h.
3. Double check shunt resistor value in platformio.ini.
4. Set the motor specs, current, speed, and acceleration in motor.h.
5. Connect computer -> USB-to-TTL serial adatper -> ESP32 motorcover. The four wires to connect are: RX->TX, TX->RX, 3V3->3V3, GND->GND.
6. Flash the firmware.


### Motor and mounting hardware
It's helpful to own a 3D printer beause you can print a lot of parts needed for this project and some of the printed parts don't require screws to secure the connection. You can find the stl and pre-sliced files under the **cad** folder.

#### Parts:
* Nema 11 motor mount: [[**stl**](https://github.com/ilovehotcakes/ESP32-Motorcover/blob/pcb-v1.0/cad/nema_11_motor_mount.stl)]
* Nema 11 motor mount(generic): [[**stl**](https://github.com/ilovehotcakes/ESP32-Motorcover/blob/pcb-v1.0/cad/prototype/nema11_mount_v3.stl)] [[**mirrored stl**](https://github.com/ilovehotcakes/ESP32-Motorcover/blob/pcb-v1.0/cad/prototype/nema11_mount_v3.stl)]
* AS5600 rotary encoder mount: [[**stl**](https://github.com/ilovehotcakes/ESP32-Motorcover/blob/pcb-v1.0/cad/nema_11_coupling.stl)]
* AS5600 magnet gluing jig: [[**stl**](https://github.com/ilovehotcakes/ESP32-Motorcover/blob/pcb-v1.0/cad/prototype/magnet_gluing_jig_v1.stl)]
* (Optional) motor coupling: [[**stl**](https://github.com/ilovehotcakes/ESP32-Motorcover/blob/pcb-v1.0/cad/nema_11_coupling.stl)]
* (Optional) pcb mount: coming soon

<p align="center">
    <img src="https://github.com/ilovehotcakes/ESP32-Motorcover/blob/pcb-v1.0/images/cad/v1_0/mount_v1_front.png" width="400"/>
    <img src="https://github.com/ilovehotcakes/ESP32-Motorcover/blob/pcb-v1.0/images/cad/v1_0/mount_v1_back.png" width="400">
    </br>
    <img src="https://github.com/ilovehotcakes/ESP32-Motorcover/blob/pcb-v1.0/images/cad/v1_0/IMG_2195.jpg" width="400">
    <img src="https://github.com/ilovehotcakes/ESP32-Motorcover/blob/pcb-v1.0/images/cad/v1_0/IMG_2196.jpg" width="400">
</p>


## Usage
Currently, you will need a MQTT server to send commands to the ESP32 motorcover. You can either run one on a computer or a rasberry pi. Working on an update that doesn't require a MQTT server.

### Sending Commands via MQTT
* inTopic is where the motorcover will receive MQTT commands. For example, I set "/server/shades/1" on the MQTT server to send commands to motorshade #1.
* outTopic is where motorcover will send MQTT messages to update its state. For example, I set "/client/shades/1" on the MQTT server to receive messages from motorshade #1.
* Home Assistant provides an integration for [MQTT covers](https://www.home-assistant.io/integrations/cover.mqtt/).

### MQTT Commands:
    * **0~100: move to position(%);** 0 -> open, 100 -> close
    *  **-1  : stop**
    *  **-2  : open**
    *  **-3  : close**
    *  **-4  : set min position**
    *  **-5  : set max position**
    *  **-98 : reset system to default**
    *  **-99 : reboot system**

### (Optional) tuning StallGuard4
StallGuard4(SG) is a feature of the motor driver, TMC2209, which provides automatic stall detection and stopping. SG requires some trial-and-error as well as some patience to get it working as intended. Please refer to the [TMC2209 datasheet](https://www.analog.com/media/en/technical-documentation/data-sheets/TMC2209_datasheet_rev1.09.pdf), chapter 16, page 70.


## TODO:
* write my own drivers to reduce dependency
* no need to add wifi creds when flashing firmware
* reduce soldering wiring
* remove MQTT dependency
