# ESP32 Motorcover
A ESP32-based DIY motorcover solution for smarthomes. A [cover](https://www.home-assistant.io/integrations/cover/)
is a rollershutter, blinds, or window in [Home Assistant](https://www.home-assistant.io/). A motorized cover
provides the possibility to control your covers via your choice of smarthome hub/system. Currently, the ESP32
motorcover interfaces with the smarthome via MQTT. Home Assistant provides


## Parts List
Here are exact parts and costs to make 5 motorcovers. I spent ~$92 per motorcovers but if you
have a 3D printer, extra wires and screws around, it will cost you ~$80 per motor covers.

|Name                       |Cost (incl. tax)|Shipping|Total  |Links  |
|---------------------------|---------------:|-------:|------:|:-----:|
|NEMA stepper motor         |         $166.70|  $33.12|$199.82|[[Link]](https://www.omc-stepperonline.com/nema-11-stepper-motor-bipolar-l-45mm-w-gear-ratio-5-1-planetary-gearbox-11hs18-0674s-pg5)|
|ESP32 node mcu             |          $12.10|        | $60.50|[[Link]](https://www.amazon.com/dp/B0718T232Z)|
|Buck convertor             |          $29.75|   $7.99| $37.74|[[Link]](https://www.mouser.com/ProductDetail/485-4739)|
|TMC2209 UART stepper driver|          $34.12|        | $34.12|[[Link]](https://www.amazon.com/gp/product/B07YW7BM68)|
|Coupling                   |          $14.96|        | $14.96|[[Link]](https://www.amazon.com/gp/product/B07MPFJGZW)|
|100uF capacitor            |           $6.82|        |  $6.82|[[Link]](https://www.amazon.com/gp/product/B07Y3F194W)|
|Solderless breadboard      |           $6.59|        |  $6.59|[[Link]](https://www.amazon.com/gp/product/B07LF71ZTS)|
|Power supply               |          $17.51|        | $35.02|[[Link]](https://www.amazon.com/gp/product/B07N18XN84)|
|Wires                      |          $14.86|        | $14.86|[[Link]](https://www.amazon.com/gp/product/B07Z4W6V6R)|
|Mounting bracket           |          $30.21|   $9.99| $40.20|       |
|Screws                     |          $10.00|        |    $10|       |
|Total                      |                |        |$460.63|       |


## Usage
### Connections
![schematic](images/esp32_motorcover.png)

### Flashing firmware
#### Dependencies
You will need to add teemuatlut/TMCStepper@^0.7.3, knolleary/PubSubClient@^2.8,	gin66/FastAccelStepper@^0.27.5 to your project.

### WiFi and MQTT
https://www.home-assistant.io/integrations/cover.mqtt/
MQTT commands
0~100 percentage of open position; 0 -> open, 100 -> close
-1 stop
-2 open
-3 close
-4 set min position
-5 set max position
-99 reboot system

### My use case
#### Spring dampening

## Resources
### TMC2209 Info
TMC2209 datasheet: https://www.trinamic.com/fileadmin/assets/Products/ICs_Documents/TMC2209_Datasheet_V103.pdf
BigTreeTech TMC2209 schematic: https://github.com/bigtreetech/BIGTREETECH-TMC2209-V1.2/blob/master/Schematic/TMC2209-V1.2.pdf
BigTreeTech TMC2209 manual: https://github.com/bigtreetech/BIGTREETECH-TMC2209-V1.2/blob/master/manual/TMC2209-V1.2-manual.pdf
How to connect TMC2209 for UART (stallguard): https://forum.arduino.cc/t/using-a-tmc2209-silent-stepper-motor-driver-with-an-arduino/666992/14
Explaning TMC2209 settings: https://forum.arduino.cc/t/using-a-tmc2209-silent-stepper-motor-driver-with-an-arduino/666992/25

More explanation on Stallguard: https://gist.github.com/metalinspired/dcfe07ed0b9f42870eb54dcf8e29c126

How to connect UART for stallguard: https://forum.arduino.cc/t/tmcstepper-arduino-tmc2209/956036/9

ESP-now scanning devices: https://circuitcellar.com/research-design-hub/design-solutions/using-esp-now-protocol-part-1/
