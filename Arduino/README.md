# Programming the QuantumDice by UTwente

<img src=../images/under_construction.jpg alt="construct" width="500"/>

## Table of Contents

- [Technical Description](#technical-description)
- [Configuration Arduino IDE](#configuration-arduino-ide)
  - [Install Libraries](#install-libraries)
  - [Install the ESP32 Board in Arduino IDE](#install-the-esp32-board-in-arduino-ide)
- [Load the QuantumDice.ino Sketch](#load-the-quantumdiceino-sketch)
  - [Fill in the Config File](#fill-in-the-config-file)
  - [Prepare for Upload](#prepare-for-upload)
    - [Select Board and Board Settings](#select-board-and-board-settings)
    - [Set Serial Port and Upload the Sketch](#set-serial-port-and-upload-the-sketch)
- [Configuration of ProcessorBoard](#configuration-of-processorboard)
  - [Get macAddress](#get-macaddress)
  - [Lock the ATECC508A Chip](#lock-the-atecc508a-chip)
  - [Calibration of BNO055 IMU Sensor](#calibration-of-bno055-imu-sensor)

## Technical Description

The Quantum Dice is built around an [ESP32-S3 N16R8 module](https://www.espressif.com/sites/default/files/documentation/esp32-s3-wroom-1_wroom-1u_datasheet_en.pdf). The six round TFT displays interface via SPI. The displays are controlled via the CS-pin, connected to a digital port. A [BNO055 IMU sensor](https://nl.mouser.com/datasheet/3/1046/1/bst-bno055-ds000.pdf) is used to measure rotations and positions of the dice. A [Microchip ATECC508A cryptographic chip](https://cdn.sparkfun.com/assets/learn_tutorials/1/0/0/3/Microchip_ATECC508A_Datasheet.pdf) is used as a random number generator. Both devices use I2C as their interface. Furthermore, a push button is used and the battery voltage is monitored.
The ESPNOW protocol is used for peer-to-peer communication to exchange data between the two dice.

## Configuration Arduino IDE

Download and install the latest version of the [Arduino IDE 2.x](https://docs.arduino.cc/software/ide/#ide-v2).

### Install Libraries

Install the following libraries via the library manager:

- BNO055 by Adafruit
- Adafruit Unified Sensor
- Adafruit GFX
- Adafruit GC9A01A
- SparkFun ATECCX08A
- Button2

### Install the ESP32 Board in Arduino IDE

Follow this instruction: [Installing the ESP32 Board in Arduino IDE](https://randomnerdtutorials.com/installing-esp32-arduino-ide-2-0/)

> **Important**: Board manager: ESP32 version 3.3.2.

## Load the QuantumDice.ino Sketch

If your board comes unconfigured, follow [this](#configuration-of-processorboard) instruction first.

Download the QuantumDice Arduino sketch from this GitLab. Store it in your default Arduino folder. Open the sketch in your Arduino IDE.

### Fill in the Config File

Under the tab `diceConfig.h`, the configuration of the Quantum Dice is set. In the near future, this will be replaced by a config file upload to the ESP32 using SPIFFS.

The top part contains a list of all (your) sets, identified with a serial number:

```text
// List of all dice sets
#define DICE_SET_S000 0  //TEST1
#define DICE_SET_S001 1  //BART1
#define DICE_SET_S002 2  //BART2
#define DICE_SET_S003 3  //SQD1
#define DICE_SET_S004 4  //non existent
#define DICE_SET_S005 5  //TELEP

//select one of the above
#define SELECTED_DICE_SET DICE_SET_S000
```

The config data per serial number:

```text
//**********************************************//
//ESP32 SMD v3.2 n16r8,
#if SELECTED_DICE_SET == DICE_SET_S000
#define DICE_ID "TEST1" //5 letter id for the set
#define SMD //default SMD. Optional HDR for ancient processor boards
#define DEVKIT //default DEVKIT. Optional NANO for processorPCB based on Arduino ESP32 nano board

// definitions of macAddresses per role:
inline uint8_t deviceA_mac[6] = { 0xD0, 0xCF, 0x13, 0x36, 0x40, 0x88 }; // MAC address of device A
inline uint8_t deviceB1_mac[6] = {0xD0, 0xCF, 0x13, 0x33, 0x58, 0x5C };  // MAC address of device B
inline uint8_t deviceB2_mac[6] = { 0xDC, 0xDA, 0xC, 0x21, 0x2, 0x44 };  // DUMMY. Replace with actual MAC address. Use this with the teleportation experiment
//background color of display. Select  BLACK, BLUE, RED, GREEN, CYAN, MAGENTA, YELLOW, WHITE, ORANGE, GREY, BORDEAUX, DINOGREEN, WHITE
#define X_BACKGROUND GC9A01A_BLACK
#define Y_BACKGROUND GC9A01A_BLACK
#define Z_BACKGROUND GC9A01A_BLACK

// color definition when dices are entangled
#define ENTANG_AB1_COLOR GC9A01A_YELLOW
#define ENTANG_AB2_COLOR GC9A01A_GREEN

#define RSSILIMIT -35       //RSSI value to detect close by for entanglement. Less negative is less sensitive.

//**********************************************//
```

> **⚠️ IMPORTANT: Disconnect 4-wire Power Cable Before Connecting USB Cable**

### Prepare for Upload

Remove the top and bottom (blue) cups and disconnect the 4-wire power cable. For convenience, disconnect the display FPC cables.

Connect the USB-C cable on the lower side of the ProcessorPCB.

#### Select Board and Board Settings

Select the ESP32S3 DevModule from the Arduino Tools menu and change the board settings according to the figure below. The red arrows indicate the deviations from the default settings.
![alt text](<../images/ESP32-S3 n16r8 arduino settings.png>)

#### Set Serial Port and Upload the Sketch

Connect the ProcessorPCB board with a USB-C cable and select the communication port from the Tools menu. Click upload to start compiling and uploading.

In the Serial Monitor, the debugging information shows up (use baudrate 115200).

A typical output from the startup sequence:

```text
version:070 - diceID:TEST1
devkit board
BNO device found!
Current sensor ID: 55
EEPROM stored ID: 55
Found calibration data in EEPROM
Loading calibration offsets:
Accel: -2 -3 -22  | Gyro: -4 -1 -2  | Mag: 146 -15 -350  | Radii: A=1000 M=786
Calibration data restored successfully
Calibration Status - Sys:0 G:0 A:0 M:0 [!] System not calibrated - data should be ignored
Waiting for valid gravity data...
Attempt 1 - Gravity: (0.00, 0.00, 0.00) Magnitude: 0.00
Attempt 2 - Gravity: (-0.41, 2.96, -9.33) Magnitude: 9.80
Reset (0.04, -0.30, 0.95, )
Up vector initialized successfully
IMU initialization complete
Reset (0.04, -0.30, 0.95, )
ESP-NOW initialized successfully!
MAC Address is : D0:CF:13:33:58:5C
Self role: ROLE_B1
ESP-NOW initialized successfully!
MAC Address is : D0:CF:13:33:58:5C
StateMachine Begin: Calling onEntry for initial state
------------ enter IDLE state -------------
Curent diceState: : DiceStates::SINGLE
Previous diceState: : DiceStates::SINGLE
Last Packet Send Status: Delivery Fail
Last PaDelivery Success
WELCOME function called
etcetera

```

## Configuration of ProcessorBoard

If your ProcessorBoard is not configured, you need to do the following:

- Get the macAddress of the board
- Lock the Microchip ATECC508A cryptographic chip (this chip must be locked before it can be used)
- Calibrate the BNO055 IMU sensor and store calibration settings in EEPROM

To upload the sketches, follow [this](#prepare-for-upload) instruction.

In the near future, a single configuration sketch will be available for all three configurations.

### Get macAddress

Download and run `getMacAddress.ino`. The macAddress will be printed in the Serial Monitor. Copy and paste the address in the `diceConfig.h` tab of the QuantumDice sketch.

### Lock the ATECC508A Chip

Download and run `lock_ECCX08.ino`. Open the Serial Monitor and follow the instructions. You can store the key if you like, but it is not needed for the QuantumDice. Rerunning this sketch will provide the key again.

### Calibration of BNO055 IMU Sensor

Before use in the Quantum Dice, the BNO055 sensor must be calibrated. Calibration data is stored in EEPROM.

Download and run the `BNO055_Cali_EEPROM.ino` sketch. Open the Serial Monitor and follow the instructions.

Follow this instruction:
[Adafruit BNO055 Absolute Orientation Sensor - Device Calibration](https://learn.adafruit.com/adafruit-bno055-absolute-orientation-sensor/device-calibration)

The calibration of the accelerometer is the most difficult and can take more time. Put the dice on all 6 sides but also between 2 sides at 45 degrees.
