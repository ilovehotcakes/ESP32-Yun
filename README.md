# motorblinds_esp32

### Parts
name                             cost     shipping    total    link
- Stepper motor                  166.70   33.12       199.82   https://www.omc-stepperonline.com/nema-11-stepper-motor-bipolar-l-45mm-w-gear-ratio-5-1-planetary-gearbox-11hs18-0674s-pg5
- ESP32 node mcu                 12.10                60.50    https://www.amazon.com/dp/B0718T232Z/
- Voltage step down convertor    29.75    7.99        37.74    https://www.mouser.com/ProductDetail/485-4739
- Stepper driver                 34.12                34.12    https://www.amazon.com/gp/product/B07YW7BM68/
- Coupling                       14.96                14.96    https://www.amazon.com/gp/product/B07MPFJGZW/
- 100uF Capacitor                6.82                 6.82     https://www.amazon.com/gp/product/B07Y3F194W/
- Solderless breadboard          6.59                 6.59     https://www.amazon.com/gp/product/B07LF71ZTS/
- Power supply                   17.51                35.02    https://www.amazon.com/gp/product/B07N18XN84/
- Wires                          14.86                14.86    https://www.amazon.com/gp/product/B07Z4W6V6R/
- Mounting bracket               30.21    9.99        40.20
- Screws                         10                   10
- Total                                               460.63/5

### Resources
#### TMC2209 Info
TMC2209 datasheet: https://www.trinamic.com/fileadmin/assets/Products/ICs_Documents/TMC2209_Datasheet_V103.pdf
BigTreeTech TMC2209 schematic: https://github.com/bigtreetech/BIGTREETECH-TMC2209-V1.2/blob/master/Schematic/TMC2209-V1.2.pdf
BigTreeTech TMC2209 manual: https://github.com/bigtreetech/BIGTREETECH-TMC2209-V1.2/blob/master/manual/TMC2209-V1.2-manual.pdf
ESP32 with TMC2209 stallguard example: https://github.com/edwardocano/Esp32-TMC2209/blob/master/stallguard/stallguard.ino
How to connect TMC2209 for UART (stallguard): https://forum.arduino.cc/t/using-a-tmc2209-silent-stepper-motor-driver-with-an-arduino/666992/14
Explaning TMC2209 settings: https://forum.arduino.cc/t/using-a-tmc2209-silent-stepper-motor-driver-with-an-arduino/666992/25
TMCStepper library documentation: https://teemuatlut.github.io/TMCStepper/class_t_m_c2209_stepper.html
FastAccelStepper library documentation: https://github.com/gin66/FastAccelStepper/blob/master/extras/doc/FastAccelStepper_API.md

More explanation on Stallguard: https://gist.github.com/metalinspired/dcfe07ed0b9f42870eb54dcf8e29c126

How to connect UART for stallguard: https://forum.arduino.cc/t/tmcstepper-arduino-tmc2209/956036/9

ESP-now scanning devices: https://circuitcellar.com/research-design-hub/design-solutions/using-esp-now-protocol-part-1/

https://www.home-assistant.io/integrations/cover.mqtt/