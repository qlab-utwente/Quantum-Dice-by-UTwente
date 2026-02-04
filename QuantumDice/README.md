# Programming the QuantumDice by UTwente

---

## Table of Contents

1. [Introduction](#1-introduction)
2. [Technical Overview](#2-technical-overview)
3. [Arduino IDE Setup](#3-arduino-ide-setup)
   - 3.1 [Install Arduino IDE](#31-install-arduino-ide)
   - 3.2 [Install ESP32 Board Support](#32-install-esp32-board-support)
   - 3.3 [Configure Board Settings](#33-configure-board-settings)
   - 3.4 [Install Required Libraries](#34-install-required-libraries)
4. [First Time Setup](#4-first-time-setup)
   - 4.1 [Initial Upload - Blink Test](#41-initial-upload---blink-test)
   - 4.2 [Board Configuration](#42-board-configuration)
5. [Programming the QuantumDice](#5-programming-the-quantumdice)
   - 5.1 [Prepare for Upload](#51-prepare-for-upload)
   - 5.2 [Upload the Sketch](#52-upload-the-sketch)
   - 5.3 [Verify Operation](#53-verify-operation)

---

## 1. Introduction

This guide provides step-by-step instructions for programming and configuring the QuantumDice hardware. Before beginning, ensure you have a USB-C cable and have installed the Arduino IDE on your computer.

> **⚠️ CRITICAL SAFETY WARNING:** Always disconnect the 4-wire power cable before connecting the USB cable to prevent permanent damage to the board! For easier access to the boot and reset buttons and to avoid stress on the display cables, disconnect both the upper and lower screens from the ProcessorBoard during programming.

---

## 2. Technical Overview

The QuantumDice is built on an **ESP32-S3 N16R8 module** and includes several key components:

**Partitions:**
A custom partition setting is used to include a **littlFS** partition. Therefor the file `partitions.csv` must be added to the sketch folder. See [partitions.csv](partitions.csv)

**Display System:**

- Six round TFT displays with SPI interface
- Each display controlled via individual CS-pin connected to a digital port

**Sensors and Components:**

- **BNO055 IMU sensor**: Measures rotation and position of the dice. I2C protocol.
- ESP32 random generator
- Push button for user input
- Battery voltage monitoring system

**Communication:**

- ESP-NOW protocol enables peer-to-peer communication between paired dice
- Automatic neighbor discovery via mesh networking

**Data Storage:**

- Configuration data stored in LittleFS
- Configuration file (called XXXXX_config.txt where XXXXX is the Set Name of the dice)
- The system automatically detects and loads the config file on boot
- [See example configs](/example_configs/)

**Reference Documentation:**

- [ESP32-S3 N16R8 Datasheet](https://www.espressif.com/sites/default/files/documentation/esp32-s3-wroom-1_wroom-1u_datasheet_en.pdf)
- [BNO055 IMU Datasheet](https://nl.mouser.com/datasheet/3/1046/1/bst-bno055-ds000.pdf)

---

## 3. Arduino IDE Setup

### 3.1 Install Arduino IDE

Download and install the latest version of [Arduino IDE 2.x](https://docs.arduino.cc/software/ide/#ide-v2) for your operating system.

### 3.2 Install ESP32 Board Support

1. Open Arduino IDE
2. Navigate to **File > Preferences**
3. In the "Additional Boards Manager URLs" field, add:

   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```

4. Click **OK** to save
5. Go to **Tools > Board > Boards Manager**
6. Search for "ESP32"
7. Install **"esp32 by Espressif Systems"**
8. Wait for installation to complete

> **⚠️ IMPORTANT:** Use ESP32 version 3.3.2 from the Board Manager for compatibility.

### 3.3 Configure Board Settings

Select the following settings from the **Tools** menu:

| Setting | Value |
|---------|-------|
| **Board** | ESP32S3 Dev Module |
| **USB CDC On Boot** | Enabled *(critical for native USB)* |
| **USB Mode** | USB-OTG (TinyUSB) or Hardware CDC and JTAG |
| **Flash Size** | 16MB (128Mb) |
| **PSRAM** | OPI PSRAM |
| **Partition Scheme** | custom |

### 3.4 Install Required Libraries

Install the following libraries via **Tools > Manage Libraries**:

- BNO055 by Adafruit
- Adafruit Unified Sensor
- Adafruit GFX
- Adafruit GC9A01A
- Button2

Your Arduino IDE is now ready for QuantumDice development.

---

## 4. First Time Setup

When connecting a fresh ESP32-S3 module (ESP32-S3N16R8) to Arduino IDE for the first time, you may see continuous reboot messages in the Serial monitor:

```
invalid header: 0xffffffff
invalid header: 0xffffffff
ESP-ROM:esp32s3-20210327
Build:Mar 27 2021
rst:0x7 (TG0WDT_SYS_RST),boot:0x2b (SPI_FAST_FLASH_BOOT)
Saved PC:0x40048839
invalid header: 0xffffffff
...
```

This is **completely normal** for a fresh module from the supplier. Here's what's happening:

- **`invalid header: 0xffffffff`** - The bootloader is searching for valid firmware in flash memory but finding only erased flash (0xFFFFFFFF = all bits set high = empty state)
- **`rst:0x7 (TG0WDT_SYS_RST)`** - Watchdog timer reset triggered because no valid firmware was found to execute
- **Bootloop** - The module continuously resets because there's no program to run

The ESP32-S3 module has never been programmed and needs its first firmware upload.

### 4.1 Initial Upload

When programming an ESP32-S3 for the first time, a specific initialization procedure is required.

> **⚠️ REMINDER:** Disconnect the 4-wire power cable before connecting USB!

#### Step 1: Load the QuantumDice sketch

1. Go to **File > Open** and select the QuantumDice.ino file

#### Step 2: Enter Download Mode

For the first upload, manually put the ESP32-S3 into download mode. See attached image for the locations of the **BOOT** and **RESET** button.
![alt text](../../images/bootAndReset.png)

1. **Hold down** the **BOOT** button
2. While holding BOOT, **press and release** the **RESET** button
3. **Release** the **BOOT** button

Your board is now in download mode and ready to receive code.

*Alternative method:* Hold the BOOT button while plugging in the USB cable.

#### Step 3: Select COM Port

1. Go to **Tools > Port**
2. Select the COM port for your ESP32-S3:
   - **Windows:** COM# (e.g., COM3)
   - **Mac:** /dev/cu.usbmodem# or /dev/tty.usbmodem#
   - **Linux:** /dev/ttyACM# or /dev/ttyUSB#

#### Step 4: Upload the Sketch

1. Click the **Upload** button (right arrow icon)
2. Wait for compilation and upload to complete
3. You should see "Connecting..." followed by upload progress
4. Once complete, **press the RESET button** on your board

The QuantumDice should now be running.

**After this initial upload, future uploads will not require the BOOT button procedure.**

### 4.2 Board Configuration

The board configuration is performed with a XXXXX_config.txt file. You can use the  enclosed [TEST1_config.txt](/example_configs/TEST1_config.txt) as a starter.

Uploading the config file with
[ESPConnect](https://thelastoutpostworkshop.github.io/ESPConnect/) tool (need Google Chrome to run). For complete instructions, see the [Setup Guide](../README.md).

---

## 5. Programming the QuantumDice

### 5.1 Prepare for Upload

> **⚠️ REMINDER:** Always disconnect the 4-wire power cable before connecting USB!

**Before programming:**

1. Remove the top and bottom (blue) cups from the dice
2. Disconnect the 4-wire power cable
3. Optionally disconnect the display FPC cables for easier access to buttons
4. Connect a USB-C cable to the underside of the ProcessorPCB

### 5.2 Upload the Sketch

1. Open the sketch in Arduino IDE
2. Connect the ProcessorPCB via USB-C cable
3. Select the correct communication port from **Tools > Port**
4. Click **Upload** to compile and upload the sketch
5. Wait for the upload to complete

### 5.3 Verify Operation

After uploading, open the **Serial Monitor** (baud rate: **115200**) to view debugging information.

**Expected Startup Sequence:**

The Serial Monitor will display:

- Loaded configuration (Dice ID, colors, timing constants)
- Hardware pin configuration
- Display initialization
- BNO055 sensor detection and calibration status
- ESP-NOW initialization with MAC address
- State machine initialization
- Welcome sequence with logo displays

**Example Output:**

```text
[LOG]	
[LOG]	╔════════════════════════════════════════╗
[LOG]	║      QUANTUM DICE INITIALIZATION       ║
[LOG]	╚════════════════════════════════════════╝

[LOG]	/home/twan/Development/Quantum-Dice-by-UTwente/QuantumDice/QuantumDice.ino Jan 26 2026 21:09:54
[L] FW: 2.0.0
[LOG]	Step 1: Initializing filesystem and configuration...
[LOG]	
[DEBUG]	Mounting LittleFS...
[DEBUG]	LittleFS mounted successfully
[D] Total: 4194304 bytes, Used: 8192 bytes
[D] Found config file: DEFAULT_config.txt
[D] Loading global config from /DEFAULT_config.txt...
[DEBUG]	LittleFS mounted successfully
[DEBUG]	Loading config from /DEFAULT_config.txt
[D] Loaded 4 entanglement colors
[DEBUG]	Config loaded successfully
[D] Loaded global config from: /DEFAULT_config.txt
[LOG]	✓ Filesystem and configuration ready!

[LOG]	=== Global Configuration ===
[L] Dice ID: DEFAULT
[L] X Background: 0x0000 (0)
[L] Y Background: 0x0000 (0)
[L] Z Background: 0x0000 (0)
[L] Entangle Colors (4): ffe0, 7e0, 7ff, f81f
[LOG]	
[L] Color Flash Timeout: 250 ms
[L] RSSI Limit: -35 dBm
[L] Is SMD: true
[L] Is Nano: false
[L] Deep Sleep Timeout: 300000 ms
[L] Checksum: 0x00
[LOG]	============================
[LOG]	
Step 2: Initializing hardware...
[LOG]	
Initializing hardware pins...
Hardware pins initialized successfully!
[LOG]	
=== Hardware Pin Configuration ===
[L] Board Type: DEVKIT
[L] Screen Type: SMD
[LOG]	
TFT Display Pins:
[L]   CS:  GPIO10
[L]   RST: GPIO48
[L]   DC:  GPIO47
[LOG]	
Screen CS Pins:
[L]   Screen 0: GPIO4
[L]   Screen 1: GPIO5
[L]   Screen 2: GPIO6
[L]   Screen 3: GPIO7
[L]   Screen 4: GPIO15
[L]   Screen 5: GPIO16
[L] 
ADC Pin: GPIO2
[LOG]	==================================

[L]  - Dice ID: [LOG]	DEFAULT
[L] Board type: [LOG]	DEVKIT
[L] Connectie type isSMD: [LOG]	SMD
Initializing displays...
Displays initialized successfully!
[D] Qlab logo on screen: [DEBUG]	14
[LOG]	
Step 3: Initializing IMU sensor...
[LOG]	
[L] Initializing BNO055... E (3501) i2c.master: I2C transaction unexpected nack detected
E (3502) i2c.master: s_i2c_synchronous_transaction(945): I2C transaction failed
E (3503) i2c.master: i2c_master_transmit_receive(1248): I2C transaction failed
E (3520) i2c.master: I2C transaction unexpected nack detected
...
E (3773) i2c.master: s_i2c_synchronous_transaction(945): I2C transaction failed
E (3775) i2c.master: i2c_master_transmit_receive(1248): I2C transaction failed
[LOG]	detected.
[L] Applying axis remapping... [LOG]	done.
[L] Waiting for stable readings... [L] OK ([L] 9.46[L]  m/s² after [L] 1[LOG]	 attempts)
[L] Stabilizing baseline... [LOG]	done.
[LOG]	✓ BNO055 initialization complete!
[D] QR code on screen: [DEBUG]	1
[D] Einstein on screen: [DEBUG]	8
[D] UTwente logo on screen: [DEBUG]	7
[LOG]	
Step 4: Completing initialization...
[LOG]	
ESP-NOW initialized successfully!
[D] MAC Address is : D0[D] :CF[D] :13[D] :36[D] :15[D] :90[D] 
[LOG]	ESP-NOW initialized successfully!
[D] MAC Address is : D0[D] :CF[D] :13[D] :36[D] :15[D] :90[D] 
[LOG]	StateMachine Begin: Calling onEntry for initial state
[D] StateMachine: CLASSIC | IDLE | PURE
[DEBUG]	=== Entering CLASSIC MODE ===
[DEBUG]	Entering N2 case
[DEBUG]	Entering N3 case
[DEBUG]	Entering N6 case
[DEBUG]	Entering N5 case
[DEBUG]	Entering N4 case
[DEBUG]	Entering N1 case
[LOG]	
[LOG]	╔════════════════════════════════════════╗
[LOG]	║       SETUP COMPLETE - READY!          ║
[LOG]	╚════════════════════════════════════════╝
```


> **Note:** You may see some I2C error messages during initialization. These are typically harmless if the sensors are detected successfully afterward.

**Your QuantumDice is now programmed and ready to use!**

---

## Troubleshooting

**Upload fails:**

- Ensure BOOT button procedure was followed for first-time uploads
- Check USB cable connection and try a different cable
- Verify correct COM port is selected
- Confirm 4-wire power cable is disconnected

**Sensor errors:**

- Check I2C connections
- Verify sensors are properly seated

**Display issues:**

- Ensure FPC cables are properly connected
- Check CS pin assignments in code match your hardware version

---

*For additional support, refer to the QuantumDice GitHub repository or contact the UTwente development team.*
