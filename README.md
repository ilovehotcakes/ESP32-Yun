# ESP32 Motorcover
An open-source and DIY-friendly solution to motorize windows/blinds for your smart homes.

ESP32 motorcover is an affordable, reliable, and user-friendly wireless stepper motor controller that works with NEMA stepper motors and other bipolar stepper motors.

Under active development. **WARNING:** requires some soldering and electronic knowledge. Please follow along at your own risk. This project is being actively developed to further lower the barrier-to-entry.


## Design
In [**Home Assistant**](https://www.home-assistant.io/), a [**cover**](https://www.home-assistant.io/integrations/cover/) is a type of entity that could be a blind, shade, shutter, window, garage door, etc. A motorized cover provides the ability to control your covers through your choice of smarthome hub/system (HA, Google Home, Alexa, etc.) through iOS/Android apps, voice control, or automations. Please see the **[[demo video](https://user-images.githubusercontent.com/52260129/211658800-c67d9bb7-6f65-4ab0-a19c-eaa4f9b99e2e.mp4)]** for full demonstration.

### **Features:**
* Works with WiFi, no need for extra hub
* Closed-loop system, making the motor movements extremely precise and reliable
* Automatic stalling detection and stopping, preventing injuries and protecting the motor
* Extremely quiet, won't even wake a baby!
* Few parts, cheap and easy to assemble


## Building your own
There are three main components to consider: electronics, firmware, and motor + mounting hardware.

### Requirements:
* Computer and [USB-to-TTL serial adatper](https://www.amazon.com/dp/B07WX2DSVB) to upload the firmware
* [Soldering iron](https://www.amazon.com/dp/B096X6SG13/)
* Wires and [JST PH connectors](https://www.amazon.com/dp/B0731MZCGF)/[crimper](https://www.amazon.com/dp/B00YGLKBSK)
* Bipolar stepper motor, such as [Nema 11](https://www.amazon.com/dp/B00PNEPK94/)
* Power supply: 3.3-24V, 1.2A max
* (Optional) 3D printer for the motor mount and coupling

### 1. Electronics
Here are two ways to acquire the electronics: **(1)** directly order printed circuit boards from JLCPCB/PCBWay or **(2)** buying parts from Amazon and putting them together on a breadboard.

**[[schematic](https://github.com/ilovehotcakes/ESP32-Motorcover/blob/main/electronics/schematic.png)][[gerber files](https://github.com/ilovehotcakes/ESP32-Motorcover/blob/main/electronics/gerber.zip)][[bom](https://github.com/ilovehotcakes/ESP32-Motorcover/blob/main/electronics/bom.csv)][[pick-and-place file](https://github.com/ilovehotcakes/ESP32-Motorcover/blob/main/electronics/pick_and_place.csv)]**

#### Option 1 - Ordering PCB from JLCPCB:
1. Download and upload the gerber files to [JLCPCB.com](https://jlcpcb.com/). The only setting that needs to be changed is the **"Impedance Control"**. Select **"Yes"** and choose **"JLC0416H-3313"** once the dialog pops up.
2. If you prefer to manually assemble the PCB, please refer to the schematic and bom.
3. If you prefer to have the PCB assembled by JLCPCB(additional cost), download the pick-and-place and bom files, and toggle **"PCB Assembly"**. Click on **"Confirm"** to go the next page.
4. Upload the bom and pick-and-place files. Click **"Process BOM & CPL"** and **"Continue"** when the error pops up. The error is for the missing connectors which will need to be manually solder once the PCBs arrive.
5. Solder connectors J1-J4 and separate the AS5600 break-off board from the main board.
6. Crimp some 4-pin JST PH connectors and connect the stepper motor to the **"motor"** connector, power supply to the **"pwr"** connector, and AS5600 breakoff board to the **"encoder"** connector.
<p align="center">
    <img src="https://github.com/ilovehotcakes/ESP32-Motorcover/blob/main/images/electronics/v1_1/pcb_top.png" width="400"/>
    <img src="https://github.com/ilovehotcakes/ESP32-Motorcover/blob/main/images/electronics/v1_1/pcb_bot.png" width="400">
    </br>
    <img src="https://github.com/ilovehotcakes/ESP32-Motorcover/blob/main/images/electronics/v1_1/pcb_3d_top.png" width="400">
    <img src="https://github.com/ilovehotcakes/ESP32-Motorcover/blob/main/images/electronics/v1_1/pcb_3d_bot.png" width="400">
</p>

#### Option 2 - Breadboarding with breakout modules:
This could be more approachable if you don't solder. You can get breakout board modules and assemble them on a solderless breadboard. Please refer to this version of the **[bom](https://github.com/ilovehotcakes/ESP32-Motorcover/blob/main/electronics/prototype/bom.csv)** and reference the schematic to put the circuit together. An example assembly looks like [this](https://github.com/ilovehotcakes/ESP32-Motorcover/blob/main/images/electronics/prototype/assembled_controller.jpg).

### 2. Firmware
Your choice of IDE to program the firmware, e.g. [Arduino IDE](https://www.arduino.cc/en/software) or [ESP-IDF](https://idf.espressif.com/). I use [VSCode](https://code.visualstudio.com/) + [PlatformIO](https://platformio.org/install/ide?install=vscode).

#### Dependencies:
* espressif32@3.5.0
* [TMCStepper@^0.7.3](https://github.com/teemuatlut/TMCStepper)
* [PubSubClient@^2.8](https://github.com/knolleary/pubsubclient)
* [FastAccelStepper@^0.27.5](https://github.com/gin66/FastAccelStepper)
* [robtillaart/AS5600@^0.4.1](https://github.com/RobTillaart/AS5600)

#### Steps:
1. Install the dependencies.
2. Add WiFi and MQTT credentials to **secrets.h**.
3. Double check and set proper current sense resistor value in **platformio.ini**.
4. Set the motor specs, current, speed, and acceleration in **motor.h**.
5. Set the USB-to-TTL serial adatper's voltage level to **3V3** and plug it in to the computer.
6. Keep holding the button on ESP32 motorcover and start flashing the firmware.
7. Once the IDE start transmitting data, connect the four wires from ESP32 motorcover to the adapter: **RX->TXD**, **TX->RXD**, **+->VCC**, **-->GND**.
8. Let go of the ESP32 motorcover button once the firmware starts uploading. Unplug ESP32 motorcover once the firmware is done uploading.

### 3. Motor and mounting hardware
It's helpful to own a 3D printer beause you can print a lot of parts needed for this project and some of the printed parts don't require screws to secure the parts. You can find the stl and pre-sliced files under the **cad** folder.

#### Parts:
* Nema 11 motor mount: [[**stl**](https://github.com/ilovehotcakes/ESP32-Motorcover/blob/main/cad/nema_11_motor_mount.stl)]
* Nema 11 motor mount(generic): [[**stl**](https://github.com/ilovehotcakes/ESP32-Motorcover/blob/main/cad/prototype/nema11_mount_v3.stl)] [[**mirrored stl**](https://github.com/ilovehotcakes/ESP32-Motorcover/blob/main/cad/prototype/nema11_mount_v3_mirrored.stl)]
* AS5600 rotary encoder mount: [[**stl**](https://github.com/ilovehotcakes/ESP32-Motorcover/blob/main/cad/as5600_mount.stl)]
* AS5600 magnet gluing jig: [[**stl**](https://github.com/ilovehotcakes/ESP32-Motorcover/blob/main/cad/prototype/magnet_gluing_jig_v1.stl.stl)]
* (Optional) motor coupling: [[**stl**](https://github.com/ilovehotcakes/ESP32-Motorcover/blob/main/cad/coupling.stl)]
* (Optional) pcb mount: coming soon

<p align="center">
    <img src="https://github.com/ilovehotcakes/ESP32-Motorcover/blob/main/images/cad/v1_0/nema_11_motor_mount_front.png" width="400"/>
    <img src="https://github.com/ilovehotcakes/ESP32-Motorcover/blob/main/images/cad/v1_0/nema_11_motor_mount_front.png" width="400">
    </br>
    <img src="https://github.com/ilovehotcakes/ESP32-Motorcover/blob/main/images/cad/v1_0/IMG_2195.jpg" width="400">
    <img src="https://github.com/ilovehotcakes/ESP32-Motorcover/blob/main/images/cad/v1_0/IMG_2196.jpg" width="400">
</p>


## Usage
Currently, you will need a MQTT server to send commands to the ESP32 motorcover. You can either run one on a computer or a rasberry pi. An update that doesn't require a MQTT server is coming soon.

### Sending commands via MQTT
* *inTopic* is where the motorcover will receive MQTT commands. For example, I set "/server/shades/1" on the MQTT server to send commands to motorshade #1.
* *outTopic* is where motorcover will send MQTT messages to update its state. For example, I set "/client/shades/1" on the MQTT server to receive messages from motorshade #1.
* Home Assistant provides an integration for [MQTT covers](https://www.home-assistant.io/integrations/cover.mqtt/).

### MQTT commands:
* **0~100: move to position(%);** 0 -> open, 100 -> close
*  **-1  : stop**
*  **-2  : open**
*  **-3  : close**
*  **-4  : set min position**
*  **-5  : set max position**
*  **-98 : reset system to default**
*  **-99 : reboot system**

### (Optional) Tuning StallGuard4
StallGuard4(SG) is a feature of the motor driver, TMC2209, which provides automatic stall detection and stopping. SG requires some trial-and-error as well as some patience to get it working as intended. Please refer to the [TMC2209 datasheet](https://www.analog.com/media/en/technical-documentation/data-sheets/TMC2209_datasheet_rev1.09.pdf), chapter 16, page 70.


## Coming soon
* Remove the need for a MQTT server to send commands
* Ability to change WiFi credentials and motor settings after flashing the firmware
* Write custom drivers to reduce dependency
* BLE support
* Updating PCB so little to no soldering is required
* Replace ESP32 WROOM 32E with ESP32-S3 so USB-to-TTL is not required
