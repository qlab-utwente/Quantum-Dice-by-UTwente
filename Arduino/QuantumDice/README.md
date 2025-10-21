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

**Display System:**
- Six round TFT displays with SPI interface
- Each display controlled via individual CS-pin connected to a digital port

**Sensors and Components:**
- **BNO055 IMU sensor**: Measures rotation and position of the dice
- **ATECC508A cryptographic chip**: Provides true random number generation
- Both sensors communicate via I2C protocol
- Push button for user input
- Battery voltage monitoring system

**Communication:**
- ESP-NOW protocol enables peer-to-peer communication between paired dice
- Each dice stores the MAC addresses of its paired partners

**Data Storage:**
- Configuration and calibration data stored in EEPROM
- Must be configured before first use using the separate QuantumDiceInitTool sketch

**Reference Documentation:**
- [ESP32-S3 N16R8 Datasheet](https://www.espressif.com/sites/default/files/documentation/esp32-s3-wroom-1_wroom-1u_datasheet_en.pdf)
- [BNO055 IMU Datasheet](https://nl.mouser.com/datasheet/3/1046/1/bst-bno055-ds000.pdf)
- [ATECC508A Datasheet](https://cdn.sparkfun.com/assets/learn_tutorials/1/0/0/3/Microchip_ATECC508A_Datasheet.pdf)

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
| **Partition Scheme** | 16M Flash (3MB....) |

### 3.4 Install Required Libraries

Install the following libraries via **Tools > Manage Libraries**:

- BNO055 by Adafruit
- Adafruit Unified Sensor
- Adafruit GFX
- Adafruit GC9A01A
- SparkFun ATECCX08A
- Button2

Your Arduino IDE is now ready for QuantumDice development.

---

## 4. First Time Setup

### 4.1 Initial Upload - Blink Test

When programming an ESP32-S3 for the first time, a specific initialization procedure is required. We'll use the built-in Blink example for this.

> **⚠️ REMINDER:** Disconnect the 4-wire power cable before connecting USB!

#### Step 1: Load the Blink Sketch

1. Go to **File > Examples > 01.Basics > Blink**
2. The Blink sketch will open in a new window
3. Leave the code as-is (default `LED_BUILTIN` is fine)

#### Step 2: Enter Download Mode

For the first upload, manually put the ESP32-S3 into download mode:

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

The Blink sketch is now running (though the LED may not be visible on this board).

**After this initial upload, future uploads will not require the BOOT button procedure.**

### 4.2 Board Configuration

If your ProcessorBoard has never been configured, you must complete these steps using the **QuantumDiceInitTool.ino** sketch:

- Obtain the board's MAC address
- Lock the ATECC508A cryptographic chip (required before use)
- Calibrate the BNO055 IMU sensor and save calibration to EEPROM
- Configure QuantumDice settings and save to EEPROM

Refer to the QuantumDiceInitTool README for detailed instructions on this configuration process.

---

## 5. Programming the QuantumDice

### 5.1 Prepare for Upload

> **⚠️ REMINDER:** Always disconnect the 4-wire power cable before connecting USB!

**Before programming:**

1. Remove the top and bottom (blue) cups from the dice
2. Disconnect the 4-wire power cable
3. Optionally disconnect the display FPC cables for easier access to buttons
4. Connect a USB-C cable to the underside of the ProcessorPCB

**Important:** If your board is unconfigured, complete the [Board Configuration](#42-board-configuration) steps first.

### 5.2 Upload the Sketch

1. Download the **QuantumDice.ino** sketch from GitHub
2. Save it to your Arduino default folder
3. Open the sketch in Arduino IDE
4. Connect the ProcessorPCB via USB-C cable
5. Select the correct communication port from **Tools > Port**
6. Click **Upload** to compile and upload the sketch
7. Wait for the upload to complete

### 5.3 Verify Operation

After uploading, open the **Serial Monitor** (baud rate: **115200**) to view debugging information.

**Expected Startup Sequence:**

The Serial Monitor will display:
- EEPROM initialization and memory map
- Loaded configuration (Dice ID, MAC addresses, colors, timing constants)
- Hardware pin configuration
- Display initialization
- BNO055 sensor detection and calibration status
- ESP-NOW initialization with MAC address
- State machine initialization
- Welcome sequence with logo displays

**Example Output:**

```text
EEPROM initialized successfully
Configuration loaded successfully!
Dice ID: TEST1
Device A MAC: D0:CF:13:36:40:88
Hardware pins initialized successfully!
Initializing displays...
Displays initialized successfully!
BNO device found!
Calibration data restored successfully
IMU initialization complete
ESP-NOW initialized successfully!
MAC Address is : D0:CF:13:33:58:5C
Setup complete!
```

> **Note:** You may see some I2C error messages during initialization. These are typically harmless if the sensors are detected successfully afterward.

If you see "System not calibrated" warnings, don't worry—this is normal during startup. The system will use the stored calibration data.

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
- Run the QuantumDiceInitTool if calibration data is missing

**Display issues:**
- Ensure FPC cables are properly connected
- Check CS pin assignments in code match your hardware version

---

*For additional support, refer to the QuantumDice GitHub repository or contact the UTwente development team.*