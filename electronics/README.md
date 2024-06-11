# Electronics
Here are two ways to acquire the electronics: **(1)** directly order printed circuit boards from JLCPCB/PCBWay or **(2)** buying parts from Amazon and assembling on a breadboard.

#### Option 1 - Ordering PCB from JLCPCB:
1. Download and upload the gerber files to [JLCPCB.com](https://jlcpcb.com/). The only setting that needs to be changed is the **"Impedance Control"**. Select **"Yes"** and choose **"JLC0416H-3313"** once the dialog pops up.
2. If you prefer to manually assemble the PCB, please refer to the schematic and bom.
3. If you prefer to have the PCB assembled by JLCPCB(additional cost), download the pick-and-place and bom files, and toggle **"PCB Assembly"**. Click on **"Confirm"** to go the next page.
4. Upload the bom and pick-and-place files. Click **"Process BOM & CPL"** and **"Continue"** when the error pops up. The error is for the missing connectors which will need to be manually solder once the PCBs arrive.
5. Solder connectors J1-J4 and separate the AS5600 break-off board from the main board.
6. Crimp some 4-pin JST PH connectors and connect the stepper motor to the **"motor"** connector, power supply to the **"pwr"** connector, and AS5600 breakoff board to the **"encoder"** connector.
<p align="center">
    <img src="v1_1/pcb_top.png" width="400"/>
    <img src="v1_1/pcb_bot.png" width="400">
</p>

#### Option 2 - Breadboarding with breakout modules:
This could be more approachable if you don't solder. You can get breakout board modules and assemble them on a solderless breadboard. Please refer to this version of the **[bom](https://github.com/ilovehotcakes/ESP32-Motorcover/blob/main/electronics/prototype/bom.csv)** and reference the schematic to put the circuit together. An example assembly looks like [this](https://github.com/ilovehotcakes/ESP32-Motorcover/blob/main/images/electronics/prototype/assembled_controller.jpg).
