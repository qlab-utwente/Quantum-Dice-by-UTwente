# Programming the QuantumDice by UTwente

---

## 1. Technical Description

The Quantum Dice is built around an [ESP32-S3 N16R8 module](https://www.espressif.com/sites/default/files/documentation/esp32-s3-wroom-1_wroom-1u_datasheet_en.pdf). The device features six round TFT displays that interface via SPI. Each display is controlled through its CS-pin, connected to a digital port. 

A [BNO055 IMU sensor](https://nl.mouser.com/datasheet/3/1046/1/bst-bno055-ds000.pdf) measures the rotation and position of the dice. A [Microchip ATECC508A cryptographic chip](https://cdn.sparkfun.com/assets/learn_tutorials/1/0/0/3/Microchip_ATECC508A_Datasheet.pdf) serves as a random number generator. Both devices use I2C as their communication interface. Additionally, a push button is incorporated, and the battery voltage is continuously monitored.

The ESP-NOW protocol enables peer-to-peer communication to exchange data between the two dice. Both dices have the macAdresses of its peer dice to enable peer-to-peer communication.

Configuration and calibration data is stored in EEPROM. Before the Quantum Dice software can be used, the dice settings must be configured. A separate sketch is available for this purpose

> **⚠️ IMPORTANT:** Always disconnect the 4-wire power cable before connecting the USB cable to prevent damage to the board! To have better acces to the boot and reset button and to avoid stress on the FPC cables of the upper and lower screen, disconnect both displays from the ProcessorBoard.


---
## 2. Configure Arduino IDE and first time upload

### 2.1 Installation Arduino IDE

Download and install the latest version of [Arduino IDE 2.x](https://docs.arduino.cc/software/ide/#ide-v2).

### 2.2 Install ESP32 Board Support

1. Open Arduino IDE
2. Go to **File > Preferences**
3. In "Additional Boards Manager URLs" field, add:
https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json

4. Click **OK**
5. Go to **Tools > Board > Boards Manager**
6. Search for "ESP32"
7. Install **"esp32 by Espressif Systems"** (install the latest version)
8. Wait for installation to complete

> **⚠️ IMPORTANT:** Use ESP32 version 3.3.2 in the Board Manager.

### 2.3 Configure Board Settings

Select the following settings from the **Tools** menu:

- **Board**: ESP32S3 Dev Module
- **USB CDC On Boot**: Enabled *(critical for native USB)*
- **USB Mode**: USB-OTG (TinyUSB) or Hardware CDC and JTAG
- **Flash Size**: 16MB (128Mb)
- **PSRAM**: OPI PSRAM
- **Partition Scheme**: 16M Flash (3MB....)


### 2.4 First Upload - Blink Example

> **⚠️ IMPORTANT:** Always disconnect the 4-wire power cable before connecting the USB cable to prevent damage to the board!

When the ESP32-S3 has never been used before a specific procedure must be followed to prepare the board for normal upload use. The Blink sketch is used for this.

#### 1. Load the Blink Sketch

1. Go to **File > Examples > 01.Basics > Blink**
2. The Blink sketch will open in a new window
3. Modify the LED pin if needed (default is `LED_BUILTIN`, change to your board's LED pin if different)

#### 2. Enter Download Mode

For the first upload, you must manually put the ESP32-S3 into download mode:

1. **Hold down** the **BOOT** button on your board
2. While holding BOOT, press the **RESET** button once
3. Release the **RESET** button
4. Release the **BOOT** button

Your board is now in download mode.

*Alternative method*: Hold the BOOT button while plugging in the USB cable.

#### 3. Select COM Port

1. Go to **Tools > Port**
2. Select the COM port corresponding to your ESP32-S3
   - Windows: COM# (e.g., COM3)
   - Mac: /dev/cu.usbmodem# or /dev/tty.usbmodem#
   - Linux: /dev/ttyACM# or /dev/ttyUSB#

#### 4. Upload the Sketch

1. Click the **Upload** button (right arrow icon) in Arduino IDE
2. Wait for the compilation and upload process to complete
3. You should see "Connecting..." followed by upload progress

#### 5. Run Your Code

After upload completes:

1. **Press the RESET button** on your board
2. Your Blink sketch should now be running (this can not be observed because the LED is not present


### 2.1 Install Libraries

Install the following libraries via the Library Manager:

- BNO055 by Adafruit
- Adafruit Unified Sensor
- Adafruit GFX
- Adafruit GC9A01A
- SparkFun ATECCX08A
- Button2

The Arduino IDE is now ready to be used.

---

## 3. Configuration of ProcessorBoard

If your ProcessorBoard is unconfigured, you need to complete the following steps:

- Obtain the MAC address of the board
- Lock the Microchip ATECC508A cryptographic chip (this chip must be locked before it can be used)
- Calibrate the BNO055 IMU sensor and store calibration settings in EEPROM
- Configurate the Quantum Dice and store this is EEPROM.

The QuantumDiceInitTool.ino sketch will guide you through the various steps. (see Readme)...


## 4. Load the QuantumDice.ino Sketch

> **⚠️ IMPORTANT:** Always disconnect the 4-wire power cable before connecting the USB cable to prevent damage to the board!

If your board is unconfigured, follow the [Configuration of ProcessorBoard](#4-configuration-of-processorboard) instructions first.

Download the QuantumDice Arduino sketch from Github and store it in your default Arduino folder.

### 4.1 Prepare for Upload

> **⚠️ IMPORTANT:** Always disconnect the 4-wire power cable before connecting the USB cable to prevent damage to the board!

Remove the top and bottom (blue) cups and disconnect the 4-wire power cable. For convenience, you may also disconnect the display FPC cables.

Connect a USB-C cable to the underside of the ProcessorPCB.


### 4.2 Upload the Sketch

Connect the ProcessorPCB board with a USB-C cable and select the communication port from the Tools menu. Click **Upload** to begin compiling and uploading.

Debugging information will appear in the Serial Monitor (use baud rate 115200).

A typical output from the startup sequence:

```text
EEPROM initialized successfully
EEPROM size: 512 bytes
Configuration size: 64 bytes

=== EEPROM Memory Map ===
BNO055 Sensor ID:    0x0000 - 0x0003 (4 bytes)
BNO055 Calibration:  0x0004 - 0x0019 (22 bytes)
Dice Configuration:  0x0020 - 0x005F (64 bytes)
Total used:          96 bytes
Free space:          416 bytes
========================

Loading configuration from EEPROM...
Configuration loaded successfully!

=== Dice Configuration ===
Dice ID: TEST1
Device A MAC: D0:CF:13:36:40:88
Device B1 MAC: D0:CF:13:33:58:5C
Device B2 MAC: DC:DA:0C:21:02:44
Background Colors:
  X: 0x0000
  Y: 0x0000
  Z: 0x0000
Entanglement Colors:
  AB1: 0xFFE0
  AB2: 0x07E0
RSSI Limit: -35 dBm
Hardware: SMD, DEVKIT
Always Seven: No

Timing Constants:
  Random Switch Point: 50
  Tumble Constant: 0.20
  Deep Sleep Timeout: 300000 ms (5.0 minutes)
Checksum: 0x00
==========================

Initializing hardware pins...
Hardware pins initialized successfully!

=== Hardware Pin Configuration ===
Board Type: DEVKIT
Screen Type: SMD

TFT Display Pins:
  CS:  GPIO10
  RST: GPIO48
  DC:  GPIO47

Screen CS Pins:
  Screen 0: GPIO4
  Screen 1: GPIO5
  Screen 2: GPIO6
  Screen 3: GPIO7
  Screen 4: GPIO15
  Screen 5: GPIO16

ADC Pin: GPIO2
==================================


/Users/aernoutvanrossum/Github/Quantum-Dice-by-UTwente/Arduino/QuantumDice/QuantumDice.ino Oct 21 2025 10:36:45
FW: 075 - Dice ID: TEST1
Board type: DEVKIT
Initializing displays...
Displays initialized successfully!
Qlab logo on screen: 14
E (3497) i2c.master: I2C transaction unexpected nack detected
E (3498) i2c.master: s_i2c_synchronous_transaction(945): I2C transaction failed
E (3499) i2c.master: i2c_master_transmit_receive(1248): I2C transaction failed
E (3516) i2c.master: I2C transaction unexpected nack detected
E (3516) i2c.master: s_i2c_synchronous_transaction(945): I2C transaction failed
E (3518) i2c.master: i2c_master_transmit_receive(1248): I2C transaction failed
E (3535) i2c.master: I2C transaction unexpected nack detected
E (3535) i2c.master: s_i2c_synchronous_transaction(945): I2C transaction failed
|
|
E (3769) i2c.master: s_i2c_synchronous_transaction(945): I2C transaction failed
E (3771) i2c.master: i2c_master_transmit_receive(1248): I2C transaction failed
BNO device found!
------------------------------------
Sensor:       BNO055
Driver Ver:   1
Unique ID:    55
Max Value:    0.00 xxx
Min Value:    0.00 xxx
Resolution:   0.01 xxx
------------------------------------
Current sensor ID: 55
EEPROM stored ID: 55
Found calibration data in EEPROM
Loading calibration offsets:
Accel: -23 214 -27  | Gyro: 0 -4 2  | Mag: 123 114 -522  | Radii: A=1000 M=633
Calibration data restored successfully
Calibration Status - Sys:0 G:0 A:0 M:0 [!] System not calibrated - data should be ignored (<-- don't worry. Is ok)
Waiting for valid gravity data...
Attempt 1 - Gravity: (0.00, 0.00, 0.00) Magnitude: 0.00
Attempt 2 - Gravity: (3.09, 2.60, 8.93) Magnitude: 9.80
Reset (-0.32, -0.27, -0.91, )
Up vector initialized successfully
IMU initialization complete
Reset (-0.32, -0.27, -0.91, )
ESP-NOW initialized successfully!
MAC Address is : D0:CF:13:33:58:5C
Self role: ROLE_B1
ESP-NOW initialized successfully!
MAC Address is : D0:CF:13:33:58:5C
StateMachine Begin: Calling onEntry for initial state
------------ enter IDLE state -------------
Curent diceState: : DiceStates::SINGLE
Previous diceState: : DiceStates::SINGLE
La
WELCOME function called
Einstein on screen: 2
GODDICE function called
Qlab logo on screen: 4
QLAB_LOGO function called
UTwente logo on screen: 1
UT_LOGO function called
Einstein on screen: 3
GODDICE function called
Einstein on screen: 5
GODDICE function called
Setup complete!
==================================

stateMachine: CLASSIC_STATE
DiceState: DiceStates::CLASSIC
etc.
```


