## Table of Contents

- [1. Introduction](#1-introduction)
- [2. General Description of the Quantum Dice](#2-general-description-of-the-quantum-dice)
  - [2.1 Main Components](#21-main-components)
  - [2.2 PCB](#22-pcb)
- [3. Parts and Sourcing](#3-parts-and-sourcing)
  - [3.1 3D-Printed Parts](#31-3d-printed-parts)
  - [3.2 PCB](#32-pcb)
  - [3.3 TFT Displays](#33-tft-displays)
  - [3.4 Battery](#34-battery)
  - [3.5 Cables and Other Components](#35-cables-and-other-components)
- [4. 3D Printing](#4-3d-printing)
  - [4.1 Recommended Materials](#41-recommended-materials)
  - [4.2 Printing with TPU](#42-printing-with-tpu)
  - [4.3 3D-Printed Components](#43-3d-printed-components)
  - [4.4 Print Files](#44-print-files)
  - [4.5 Outsourcing 3D Printing](#45-outsourcing-3d-printing)
- [5. Preparing the 3D-Printed Parts](#5-preparing-the-3d-printed-parts)
  - [5.1 Required Tools](#51-required-tools)
  - [5.2 Gluing the PCB Display Backplane](#52-gluing-the-pcb-display-backplane)
- [6. Preparing the Electronics and Displays](#6-preparing-the-electronics-and-displays)
  - [6.1 Required Parts and Tools](#61-required-parts-and-tools)
  - [6.2 Removing the Original Display Connectors](#62-removing-the-original-display-connectors)
  - [6.3 Mounting the Displays into the Display Cups](#63-mounting-the-displays-into-the-display-cups)
  - [6.4 Mounting the USB-C Connector](#64-mounting-the-usb-c-connector)
- [7. Assembling the Quantum Die](#7-assembling-the-quantum-die)
  - [7.1 Connecting FPC Cables](#71-connecting-fpc-cables)
  - [7.2 Connecting the PCB Display Cup](#72-connecting-the-pcb-display-cup)
  - [7.3 Preparing the Battery Display Cup](#73-preparing-the-battery-display-cup)
  - [7.4 Connecting the Remaining FPC Cables](#74-connecting-the-remaining-fpc-cables)
  - [7.5 Mounting the PCB Display Cup into the Main Frame](#75-mounting-the-pcb-display-cup-into-the-main-frame)
  - [7.6 Mounting the Red Display Cups](#76-mounting-the-red-display-cups)
  - [7.7 Initial Display Test](#77-initial-display-test)
  - [7.8 Mounting the Blue Display Cups](#78-mounting-the-blue-display-cups)
- [8. Final Test and Completion](#8-final-test-and-completion)
- [9. Firmware Installation, Configuration and Updates](#9-firmware-installation-configuration-and-updates)
  - [9.1 Initial Firmware Installation and Configuration](#91-initial-firmware-installation-and-configuration)
  - [9.2 Updating the Firmware](#92-updating-the-firmware)
  - [9.3 Changing the Configuration File](#93-changing-the-configuration-file)
- [10. Troubleshooting and Support](#10-troubleshooting-and-support)
  - [10.1 The Browser Does Not Support WebSerial](#101-the-browser-does-not-support-webserial)
  - [10.2 Cannot Connect to the Die](#102-cannot-connect-to-the-die)
  - [10.3 Firmware Installation Fails](#103-firmware-installation-fails)
  - [10.4 Configuration File Does Not Load](#104-configuration-file-does-not-load)
  - [10.5 Errors in the Serial Monitor](#105-errors-in-the-serial-monitor)
  - [10.6 Getting Help](#106-getting-help)
- [11. Congratulations](#11-congratulations)

## 1. Introduction

This construction manual provides all the information required to build your own set of **Quantum Dice (Generation 4)**. It covers component sourcing, 3D printing, preparation of the electronics, firmware installation, and step-by-step assembly.

The Quantum Dice were developed by the **University of Twente**. All designs are freely available under the **CC BY licence**.

For further information, visit [Quantum Dice by University of Twente](https://www.utwente.nl/en/mesaplus/education/quantum-dice/) or read the pre-print of our article on [arXiv](https://arxiv.org/abs/2510.04931).

This manual describes **Generation 4** of the Quantum Dice. Information and design files for previous versions are available in the corresponding archive folders.

> **Note:** The University of Twente can provide ready-to-use PCBs. All other components, including the 3D-printed parts, displays, batteries, and cables, can either be sourced independently or, where available, obtained through the University of Twente.

## 2. General Description of the Quantum Dice

The **Quantum Dice** are designed to make abstract quantum-mechanical concepts such as **quantum superposition**, **entanglement**, and **quantum key distribution** more tangible.

The Quantum Dice are used in pairs and are identical in both hardware and firmware. When switched on together, the dice automatically connect and communicate with each other.

### 2.1 Main Components

Each Quantum Die consists of a **3D-printed Main Frame** with a **TFT display** on each of its six sides. The displays are mounted in coloured **Display Cups**.

The colour of each Display Cup indicates its orientation axis:

- **Front / Back**: x-axis, yellow

- **Left / Right**: y-axis, red

- **Top / Bottom**: z-axis, blue

![alt text](<images/Exploded view Quantum Dice 3.png>)

Inside the die, a rechargeable **LiPo battery** supplies power to the electronics. The displays, communication, orientation sensing, and other functions are controlled by an **ESP32-S3 microcontroller** mounted on the PCB.

### 2.2 PCB

All electronics are integrated on a single double-sided **Printed Circuit Board (PCB)**.

![alt text](images/PCBv42_Top_Bottom_view.png)

The PCB includes:

- the **ESP32-S3 microcontroller**

- connectors for all six TFT displays

- an **Inertial Measurement Unit (IMU)** to detect the orientation and movement of the die

- a **push button**

- power and battery charging management

- a **USB-C connection** for firmware management and battery charging

The PCB controls the six displays, monitors and manages the power supply, detects the orientation and movement of the die, and provides the communication required for the Quantum Dice to operate.

The PCB is mounted on the yellow **Display Cup with flexible button tab**. The battery is mounted on the opposite yellow Display Cup.

More information about obtaining the PCB, either as a ready-to-use board from the **University of Twente** or by ordering and assembling it yourself, is provided in Section 3.2.

## 3. Parts and Sourcing

All parts required to construct the Quantum Dice are listed in the **Bill of Materials (BOM)**.

The BOM can be found here: [Bill of Materials](<3D print files/Bill of Material quantum dice.xlsx>).

In the BOM, you can specify the number of Quantum Dice you wish to build. The required quantities of all components are then calculated automatically.

### 3.1 3D-Printed Parts

The required 3D-printed parts can either be **obtained from the University of Twente** or printed independently using the provided 3D-print files.

The current design is **Generation 4**. Design files for previous versions are available in the corresponding archive folders.

All required STL files and additional information for 3D printing are included with the Quantum Dice design files. Detailed information about materials, print settings, and the individual components is provided in Chapter 4.

### 3.2 PCB

The **PCB** can either be obtained as a ready-to-use board from the **University of Twente** or ordered and assembled independently.

A PCB supplied by the University of Twente comes with the latest firmware installed and has been tested. The required connection cables, screws, and other mounting hardware are also supplied with the board.

[Quantum Dice by University of Twente](https://www.utwente.nl/en/mesaplus/education/quantum-dice/)

If you have experience with PCB fabrication and microcontroller firmware, you can also order and assemble the PCB yourself using the provided production files.

- [PCB fabrication files](<PCB files>)

- [Firmware source files](firmware)

- [Firmware installation and configuration](#9-firmware-installation-configuration-and-updates)

> **Note:** If you order and assemble the PCB yourself, the firmware must be installed and the required configuration completed before the Quantum Die can be tested. The firmware performs IMU calibration automatically.

### 3.3 TFT Displays

Each Quantum Die requires **six TFT displays**. The recommended display type and links to suppliers are included in the **BOM**.

The displays require some preparation before they can be installed in the Display Cups. This is described in Chapter 6.

### 3.4 Battery

Each Quantum Die requires one rechargeable **3.7 V LiPo battery**. Links to suitable batteries are included in the BOM. Availability may vary, so equivalent batteries from other suppliers can also be used.

The battery should meet the following specifications:

- **Type:** 3.7 V LiPo

- **Capacity:** > 1000 mAh

- **Maximum height/width:** 50 mm

- **Maximum thickness:** 18 mm

- **Connector:** JST-PH, 2-pin, 2.0 mm pitch

LiPo battery dimensions are commonly indicated by a six-digit size code. For example, in **xx3048**, `xx` indicates the battery thickness, `30` indicates a width of 30 mm, and `48` indicates a height of 48 mm.

![Battery](images/construction_battery.png)

### 3.5 Cables and Other Components

The Quantum Die uses two main types of cables:

- **6 FPC cables** to connect the six TFT displays to the PCB

- **1 USB-C extension cable** to make the PCB's USB-C connection accessible from outside the Quantum Die for firmware management and battery charging

![Cable set](images/cable_set_version4.jpg)

Additional cables and mounting components, including the battery cable, screws, and other fasteners, are listed in the BOM.

Use the BOM as the reference for the required quantities and specifications before starting the assembly.

## 4. 3D Printing

If you obtain the 3D-printed parts from the **University of Twente**, you can skip this chapter and continue with Chapter 5.

If you print the parts yourself, we recommend using **TPU for the Main Frame** and **PLA or PETG for the Display Cups and Backplanes**.

### 4.1 Recommended Materials

The **Main Frame** is printed in one piece and is designed to be made from flexible **TPU**. The flexibility of the frame helps protect the displays and electronics from impacts when the die is rolled.

A TPU hardness of **95A or 40D** is recommended.

The remaining components, including the **Display Cups** and **Backplanes**, can be printed in **PLA or PETG**.

It is also possible to print the Main Frame in PLA or PETG instead of TPU. However, these materials are rigid and provide less protection against impacts. If you use a rigid Main Frame, we recommend rolling the Quantum Dice on a **soft surface**, such as a yoga mat or foam pad.

### 4.2 Printing with TPU

Printing TPU requires some additional care:

1. **Dry the filament before printing** to reduce stringing and improve print quality.

2. **Use a reduced printing speed**, particularly when using fast printers such as the Prusa Core One or Bambu Lab models.

3. TPU should not be used with filament systems that require the flexible filament to be pushed over a long distance, such as the **Bambu Lab AMS** or **Prusa MMU3**. Feed the TPU directly into the print head instead.

Examples of suitable TPU filaments include:

- [Bambu Lab TPU 95A HF](https://eu.store.bambulab.com/products/tpu-95a-hf)

- [Fiberlogy Fiberflex-40D](https://www.3djake.nl/fiberlogy/fiberflex-40d)

For additional information about printing TPU:

- [Bambu Lab TPU Printing Guide](https://wiki.bambulab.com/en/knowledge-sharing/tpu-printing-guide)

- [How to Improve TPU Print Quality on Bambu Lab X1 Carbon](https://youtu.be/yN3RximKNiE?si=DYWy9xm7ewkI_fWw)

### 4.3 3D-Printed Components

For each Quantum Die, the following parts are required:

- 1 × black **Main Frame**

- 2 × red **Display Cups**

- 2 × blue **Display Cups**

- 1 × yellow **Display Cup with flexible button tab**

- 1 × yellow **Display Cup with USB-C port opening**

- 1 × yellow **PCB Display Backplane**

- 1 × yellow **Battery Display Backplane**

The two yellow Display Cups differ from the red and blue Display Cups. One yellow cup is used to mount the **PCB**, while the other is used to mount the **battery** and provide access to the **USB-C connector**.

Support is required for printing the Display Cups and the Main Frame.

### 4.4 Print Files

All 3D models are supplied in **STL format**. The filenames include the part name, recommended material, colour, and version number.

For additional information about the print files, orientation, and support settings, see the [3D print files README](README.md).

### 4.5 Outsourcing 3D Printing

If printing TPU yourself is not practical, the parts can be produced by a 3D-printing service.

Because the Main Frame is printed in flexible TPU and requires support, outsourcing the Main Frame can be a convenient option.

Several printing methods are available:

- **Fused Deposition Modelling (FDM):** generally lower cost

- **Selective Laser Sintering (SLS):** generally higher cost, with different surface and mechanical properties

A supplier we have used successfully is [JLC3D FDM Printing](https://jlc3dp.com/3d-printing/fused-deposition-modeling).

![alt text](<images/construction_parts.png>)

## 5. Preparing the 3D-Printed Parts

Before installing the electronics, prepare the 3D-printed parts as described below.

### 5.1 Required Tools

You will need:

- 2 mm and 2.5 mm hex keys

- cutting pliers

- **cyanoacrylate (CA) glue**, such as Loctite 406 for plastics

- tape

### 5.2 Gluing the PCB Display Backplane

The **PCB Display Backplane** must be glued to the yellow **Display Cup with flexible button tab** before the PCB can be mounted.

1. Apply a few drops of **CA glue** to the recessed edge of the Display Cup.

2. Carefully position the **PCB Display Backplane** on the Display Cup.

3. Press the Backplane firmly into place.

4. Allow the glue to cure before continuing.

> **Warning:** Use only a small amount of glue. Excess glue may overflow into the Display Cup or onto other surfaces.

![alt text](<images/construction_gluing.png>)

---

## 6. Preparing the Electronics and Displays

Before assembling the Quantum Die, prepare the PCB and the six TFT displays as described below.

### 6.1 Required Parts and Tools

You will need:

- 1 × **PCB**

- 6 × **TFT displays**

- FPC cables

- USB-C data cable (supports both data transfer and charging)

- battery cable

- M3 screws

- cutting pliers

### 6.2 Removing the Original Display Connectors

Before mounting the TFT displays into the Display Cups, the original **grey plastic connectors and pins** must be removed from all six displays.

1. Use **cutting pliers** to make two cuts along the sides of the grey connector housing. This separates the plastic housing from the display board.

2. Grip the connector with pliers and gently rock it back and forth along its short side until the connector and pins detach from the display.

3. Remove the **protective foil stickers** covering the screw holes.

> **Note:** Some force may be required to remove the connectors. Work carefully to avoid damaging the display or other components on the display board.

![alt text](<images/construction_Display_Connector_Removal.png>)

![alt text](<images/display_con.gif>)

### 6.3 Mounting the Displays into the Display Cups

Repeat the following procedure for all six displays:

1. Insert the display into the back of its **Display Cup**. Some pressure may be required.

2. If the display does not fit properly because of a slightly protruding edge, carefully remove a small amount of material from the Display Cup using fine sandpaper.

3. Secure the display using **countersunk M3×6 screws**. In most cases, **two screws per display are sufficient** to hold the display securely in place.

![alt text](<images/construction_displ_mount.png>)

### 6.4 Mounting the USB-C Connector

Insert the **USB-C extension connector** firmly into the opening of the yellow **Display Cup with USB-C port opening**.

The external USB-C port supports both **battery charging** and **direct data communication with the ESP32-S3**, including firmware installation, firmware updates, configuration, and serial monitoring.

A small amount of pressure may be required to click the connector securely into place.

## 7. Assembling the Quantum Die

With all 3D-printed and electronic components prepared, the Quantum Die can now be assembled. **If you assembled your own PCB, install its firmware using Section 9.1 before the display test in Section 7.7.**

### 7.1 Connecting FPC Cables

The displays are connected to the PCB using **FPC cables** (flat ribbon cables) with sliding-latch connectors. These connectors are delicate, so handle them carefully.

![alt text](<images/construction_FPC slide lock.png>)

To connect an FPC cable:

1. **Unlock the connector** by gently pulling the sliding latch into the open position. Do not apply excessive force.

2. **Insert the FPC cable** straight into the connector until it reaches the mechanical stop.

3. **Lock the connector** by pushing the sliding latch back into the closed position while keeping the cable fully inserted.

4. **Check the connection** and make sure the cable is fully inserted and securely locked.

> **Important:** Pay attention to the orientation of the FPC cable. The **blue side of the cable must face away from the indentation of the FPC connector**.

![alt text](<images/construction_cable_mount.png>)

### 7.2 Connecting the PCB Display Cup

The six display connectors on the PCB are labelled **TOP**, **BOTTOM**, **LEFT**, **RIGHT**, **FRONT**, and **BACK**.

Start with the yellow **Display Cup with flexible button tab**. This Display Cup holds the PCB and forms the **BACK** side of the die.

1. Insert the **short 60 mm FPC cable** into the display connector.

2. Connect the other end of the cable to the PCB connector labelled **BACK**.

3. Attach the **PCB** to the **PCB Display Backplane** using M3 hex screws.

![alt text](<images/construction_cable_mount_frame.png>)

### 7.3 Preparing the Battery Display Cup

Next, prepare the yellow **Display Cup with USB-C port opening**. This Display Cup holds the battery and forms the **FRONT** side of the die.

1. Insert the **long FPC cable** into the display connector.

2. Apply **double-sided foam tape** to the side of the **Battery Display Backplane** marked **BATTERY**.

3. Mount the **battery** onto the double-sided foam tape.

4. Attach the **Battery Display Backplane**, with the battery mounted, to the Display Cup using M3 hex screws.

5. Place the completed yellow Display Cup into the **Main Frame**.

>The Main Frame itself does not have a predefined orientation. Use the **arrow on the inside of the yellow Display Cup** to establish the orientation of the die. From this point onwards, the direction indicated by the arrow defines the **TOP** of the die and serves as the reference for the remaining assembly.

![alt text](<images/construction_battery_cup.png>)

### 7.4 Connecting the Remaining FPC Cables

With the FRONT and BACK displays prepared, connect the remaining FPC cables to the PCB.

1. Unlock the remaining five FPC connectors on the PCB.

2. Insert four **medium-length FPC cables** into the connectors labelled **TOP**, **BOTTOM**, **LEFT**, and **RIGHT**.

3. Connect the long FPC cable from the yellow FRONT Display Cup to the connector labelled **FRONT**.

Make sure all FPC cables are fully inserted and securely locked.

![alt text](<images/construction_longFPC_to_PCB.png>)

### 7.5 Mounting the PCB Display Cup into the Main Frame

1. Connect the **battery cable** to the PCB.

2. Connect the **six-wire power cable** to the PCB.

3. You may need to hold the PCB and its Display Cup close to the Main Frame while connecting the cables, as the cables are relatively short.

4. Insert the yellow **PCB Display Cup** into the Main Frame on the side opposite the yellow Battery Display Cup.

5. Make sure the **arrows on the inside of both yellow Display Cups point upwards**.

6. Guide the remaining FPC cables through their corresponding openings in the Main Frame:

   - **LEFT** and **RIGHT** through the openings on the sides

   - **TOP** through the opening at the top

   - **BOTTOM** through the opening at the bottom

> **Warning:** Avoid pulling, sharply bending, or pinching the FPC cables during assembly.

![alt text](<images/construction_PCB_battery_connect.png>)

### 7.6 Mounting the Red Display Cups

Mount the **LEFT** and **RIGHT** red Display Cups.

1. Insert the **LEFT** FPC cable into the display in the corresponding red Display Cup and close the FPC connector latch.

2. Repeat this procedure for the **RIGHT** Display Cup.

3. Make sure the FPC cables are oriented so that they do not need to twist when the Display Cups are inserted.

4. Insert both red Display Cups into the **Main Frame**.

> **Warning:** An FPC cable can become trapped between a Display Cup and the Main Frame when the cup is pressed into place. Gently guide the cables into the central cavity of the die while inserting the Display Cups. Avoid pinching, sharply bending, or placing excessive stress on the cables.

![alt text](<images/construction_red_cups.png>)

### 7.7 Initial Display Test

Before mounting the blue Display Cups, test the four displays that are currently connected.

1. Switch on the Quantum Die by pressing the **push button** through the flexible button tab in the yellow Display Cup.

2. Check that all connected displays show the **startup sequence**.

If one or more displays remain blank, switch off the Quantum Die and check the corresponding FPC cable and connector before continuing.

> **Note:** This test requires the firmware to be installed on the PCB. See Section 9.1.

### 7.8 Mounting the Blue Display Cups

Finally, mount the **BOTTOM** and **TOP** blue Display Cups.

1. Insert the **BOTTOM** FPC cable into the display in one of the blue Display Cups and close the FPC connector latch.

2. Make sure the cable does not need to twist when the Display Cup is inserted.

3. Insert the bottom blue Display Cup into the **Main Frame**.

4. Insert the **TOP** FPC cable into the display in the remaining blue Display Cup and close the FPC connector latch.

5. Make sure the cable does not need to twist when the Display Cup is inserted.

6. Insert the top blue Display Cup into the **Main Frame**.

> **Warning:** The FPC cables can become trapped between the Display Cups and the Main Frame. Gently guide the cables into the central cavity while inserting the Display Cups.

![alt text](<images/construction_blue_cups.png>)

## 8. Final Test and Completion

The Quantum Die is now fully assembled.

1. Switch on the Quantum Die.

2. Check that **all six displays** show the startup sequence.

3. Verify that the **battery voltage** is displayed correctly.

4. Check that none of the Display Cups or FPC cables are loose or under tension.

If one or more displays remain blank, switch off the Quantum Die and check the corresponding FPC cable and connector.

If you are using a PCB supplied by the **University of Twente**, the firmware and configuration are already installed. If you assembled the PCB yourself, make sure that firmware installation and any required configuration have been completed. The firmware handles IMU calibration automatically.

## 9. Firmware Installation, Configuration and Updates

The firmware is installed, updated and configured with the **[Quantum Dice Tool](https://qlab-utwente.github.io/Quantum-Dice-by-UTwente/)**, a web page that connects to the die over USB. Nothing needs to be downloaded or installed: the tool always offers the published firmware versions and checks the die before it writes anything. The step-by-step instructions, including the messages the tool can show, are in the **[Firmware Update Guide](FIRMWARE_UPDATE_GUIDE.md)**. This chapter summarises what is needed during construction.

A PCB supplied by the **University of Twente** already has firmware installed and has been tested. You can skip Section 9.1 and only need Sections 9.2 and 9.3 for later updates or a different configuration. The firmware calibrates the IMU automatically; no separate calibration procedure is required.

**What you need:**

- A computer with **Google Chrome** or **Microsoft Edge**. Safari, Firefox, phones and tablets do not support the USB connection used by the tool.
- A **USB-C data cable**, not a charging-only cable.
- The PCB or the assembled die. On a Generation 4 die the external USB-C connector is used for both charging and firmware installation.

### 9.1 Initial Firmware Installation and Configuration

Use this section only for a PCB you **ordered and assembled yourself**. Install the firmware **before the initial display test in Section 7.7**, so you can check the displays afterwards. Prepare each PCB separately.

1. Connect the PCB to your computer with the USB-C data cable.
2. Open the [Quantum Dice Tool](https://qlab-utwente.github.io/Quantum-Dice-by-UTwente/) and click **Connect**. Select the device, usually called **USB JTAG/serial debug unit**.
3. In **step 2, Install firmware**, select the newest **Firmware version** and choose **Complete install**. A new PCB has no firmware and no memory layout yet; the tool therefore does not allow a normal update and tells you to choose Complete install.
4. Click **Install firmware**, confirm with **Erase and install**, and wait until the tool reports **Done!** (one to two minutes). Do not unplug the PCB during the installation.
5. After this first installation the die is called *DEFAULT*. To give it its own name or colours, continue with Section 9.3.
6. Check the result in **step 4, Serial monitor**: click **Restart dice** and look for `FW:` with the installed version and `Dice ID:` with the name of the die.

> **Note:** Some I2C warnings during startup are harmless if the sensor initialises successfully afterward. See Section 10.5.

Once the firmware is installed, continue with the assembly in Chapter 7. The visual display test is described in Section 7.7 and the completed device is checked in Chapter 8.

### 9.2 Updating the Firmware

1. Switch on the die and connect it with the USB-C data cable.
2. Open the [Quantum Dice Tool](https://qlab-utwente.github.io/Quantum-Dice-by-UTwente/) and click **Connect**.
3. In **step 2, Install firmware**, select the newest version and choose **Update firmware**. This replaces only the program; the configuration file on the die is kept.
4. Check the **Safety checks**, click **Install firmware** and confirm with **Update now**. Wait until the tool reports **Done!**
5. Repeat for the other dice.

If the tool reports that the memory layout is different, or if the die does not work properly after an update, use **Complete install** with **Keep my settings** ticked. The tool then saves the configuration first and puts it back afterwards.

### 9.3 Changing the Configuration File

The firmware reads its settings, such as the name of the die, the background colours and the entanglement animation, from a file called `<name>_config.txt`. Each setting is explained in the [example configuration file](firmware/EXAMPLE_config.txt). Dice that are used together do not need identical configurations.

1. Connect the die to the Quantum Dice Tool as in Section 9.2.
2. In **step 3, Settings file**, click **Read settings from dice** and save the current file to your computer. Keep this copy.
3. Edit the file in a plain-text editor. Set `diceId=` to the name of the die and give the file the same name, for example `diceId=ALICE` in `ALICE_config.txt`.
4. Click **Choose file…** and select the edited file. The tool checks it against the example file of the installed firmware and reports errors and warnings before anything is written.
5. Click **Write to dice** and confirm with **Write settings**. The die restarts with the new settings.

## 10. Troubleshooting and Support

Problems with the Quantum Dice Tool and their solutions are described in more detail in the [troubleshooting section of the Firmware Update Guide](FIRMWARE_UPDATE_GUIDE.md#8-troubleshooting).

### 10.1 The Browser Does Not Support WebSerial

If the Quantum Dice Tool shows **"This browser cannot connect to the Quantum Dice"**, open it in a current version of **Google Chrome** or **Microsoft Edge** on a computer. Safari, Firefox, phones and tablets do not support this connection method.

### 10.2 Cannot Connect to the Die

If the die does not appear in the list after clicking **Connect**, or the tool reports that the die did not answer:

1. Make sure the die is switched on and awake. It goes to sleep after a few minutes without movement; press the button briefly to wake it.
2. Close the Arduino IDE, ESPConnect, serial monitors and other browser tabs that might be using the USB connection.
3. Disconnect and reconnect the USB-C cable.
4. Try another USB port or a different **data-capable** USB-C cable.
5. Restart your computer if necessary.

To see whether your computer detects the device:

- **Windows:** Open Device Manager and look under **Ports (COM & LPT)**.
- **macOS:** Open Terminal and run `ls /dev/tty.*`.
- **Linux:** Open Terminal and run `ls /dev/tty*`.

### 10.3 Firmware Installation Fails

If the tool reports **"Installation did not finish"**, the die has not been damaged:

1. Check the cable and make sure the die is awake, then try again.
2. If the **Safety checks** show a red ✕, follow the instruction in the message. *"Choose Complete install instead"* means the PCB is new or has a different memory layout; see Section 9.1.
3. If an update keeps failing, use **Complete install** with **Keep my settings** ticked.

### 10.4 Configuration File Does Not Load

If the die is called *DEFAULT* or the Serial Monitor reports **"No config file found"**, the firmware did not find a valid configuration file and uses its default settings.

1. In the Quantum Dice Tool, click **Read settings from dice** (step 3) to see which file is on the die and whether it contains errors.
2. Check that the filename ends in `_config.txt` and contains only letters, digits, `-`, `_` and `.`; the tool suggests a correct name if needed.
3. Upload the file again as described in Section 9.3.

### 10.5 Errors in the Serial Monitor

An occasional I2C message such as `I2C transaction unexpected nack detected` may occur during sensor startup. This is not necessarily a problem if the sensor subsequently initialises successfully.

Messages that require attention include:

- **"LittleFS mount failed"**: the storage for the configuration file could not be opened. Use **Complete install** with **Keep my settings** ticked (Section 9.2).
- **"Config load failed"**: check the configuration file as described in Section 10.4.
- **Setup never completes**: review the preceding log messages and check the board and cable connections.

### 10.6 Getting Help

If a problem persists, first check the steps relevant to the failed operation and review the Serial Monitor output. You can also try the procedure with the other die to determine whether the issue is device-specific.

For support, consult the [Quantum Dice GitHub repository](https://github.com/qlab-utwente/Quantum-Dice-by-UTwente) or create a GitHub issue. Include the step where the problem occurs, the complete error message or relevant serial log (**Save to file** in the Quantum Dice Tool), and what you have already tried.

---

## 11. Congratulations

You have successfully built a **Quantum Die**.

When two Quantum Dice are switched on, they automatically connect and communicate with each other. Your Quantum Dice are now ready for use.

For instructions on how to use the Quantum Dice and the available demonstrations, see [Using the Quantum Dice](USER_GUIDE.md).

![alt text](<images/construction_final.png>)
