# Firmware of the Quantum Dice

This readme provides a basic overview of the firmware, explaining what is present in this directory, how to contribute to the firware and how to build and upload the firmware.

## Table of Contents

1. [Directory Contents](#1-directory-contents)
2. [Contributing to the Firmware](#2-contributing-to-the-firmware)
3. [Building the Firmware](#3-building-the-firmware)<br>
	3.1 [Using the arduino-cli](#31-using-the-arduino-cli)<br>
	3.2 [Using the Arduino IDE](#32-using-the-arduino-ide)
4. [Uploading the Firmware](#4-uploading-the-firmware)<br>
	4.1 [Using the arduino-cli](#41-using-the-arduino-cli)<br>
	4.2 [Using the Arduino IDE](#42-using-the-arduino-ide)
5. [Changing the Configuration](#5-changing-the-configuration)

## 1. Directory Contents

```
main/                      - This directory contains all the code of the firmware in the format
 |                           that the arduino-cli and Arduino IDE except the code to be.
 | ImageLibrary/           - This directory contains several header files, each containing an
 |                           image in R5G6B5 format, which can be rendered to the screens.
 | main.ino                - The entry point of the firmware code, defining the setup() and
 |                           loop() functions that Arduino requires.
 | partitions.csv          - The custom partition scheme of the firmware.
 | sketch.yml              - The sketch project file that tells the arduino-cli how to build
 |                           the firmware.
 | *.c/cpp/h/hpp           - The C/C++ source and header files that also contain code of the
 |                           firmware.
.editorconfig              - A simple config file that tells compatible IDEs how to handle files,
                             not all IDEs support editorconfig, some may require an extension.
.gitignore                 - A simple file that tells Git which files and directories it should
                             ignore.
build.sh                   - A bash script that calls the arduino-cli to build the firmware, this
                             script should be invoked in the firmware directory and will output
					         the result to a new build directory.
EXAMPLE_config.txt         - An example config file with explanations for all the settings.
README.md                  - This file.
technical documentation.md - A technical explanation of the design of the firmware.
```

## 2. Contributing to the Firmware

Contributions to the firmware are welcomed and appreciated, but please try to match the Arduino coding style.

For questions about contributing to the firmware, please refer to the Quantum Dice Github or contact the UTwente development team.

## 3. Building the Firmware

For building the firmware, there are two options: the [arduino-cli](https://docs.arduino.cc/arduino-cli/) and the [Arduino IDE](https://www.arduino.cc/en/software/).

The Arduino IDE is recommended for beginners, the arduino-cli is recommended for those who have experience with programming in C/C++.

You can install the arduino-cli using [this guide](https://arduino.github.io/arduino-cli/installation/) or the Arduino IDE using [this guide](https://docs.arduino.cc/software/ide-v2/tutorials/getting-started/ide-v2-downloading-and-installing/).

### 3.1 Using the arduino-cli

In your command line, run the following:

```
.../Quantum-Dice-by-UTwente/firmware> ./build.sh
```

This script will run the arduino-cli with the necessary arguments to compile the firmware, which will be output to a new build/ directory.

You can then upload the compiled output [using the arduino-cli](#41-using-the-arduino-cli).

In the build/ directory, the compiled firmware will be in a file called *main.ino.bin*. There is also a file called *main.ino.merged.bin*, which is a full version of the firmware that includes the bootloader, partitions, etc...

### 3.2 Using the Arduino IDE

Before you can use the Arduino IDE to build the firmware, you must configure the Arduino IDE, use [this step-by-step plan](#configuring-the-arduino-ide) (scroll down if the link does not work) to configure the Arduino IDE.

If your Arduino IDE is configured, then you can open the firmware sketch by navigating to **File > Open**. This will open a window in which you can select a .ino file. In this window, you must navigate to the [main/main.ino] file and select it. This will cause the Arduino IDE to open the firmware sketch.

Then, you can press the Verify button (the checkmark) or you can navigate to **Sketch > Verify/Compile** to build the firmware.

Then, you can upload the compiled output [using the Arduino IDE](#42-using-the-arduino-ide).

#### Configuring the Arduino IDE:

1. In the Arduino IDE, navigate to **File > Preferences**.
2. In the field *"Additional Boards Manager URLS"*, add: ` https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json `
3. Click **Ok** to save.
4. Navigate to **Tools > Board > Boards Manager**.
5. Search for *"ESP32"*.
6. Install version *3.3.2* of *esp32 by Espressif Systems*.
7. After installation is complete, set the following settings in the **Tools** menu:

| **Setting** | **Value** |
| --- | --- |
| **Board** | esp32 |
| **USB CDC On Boot** | Enabled |
| **USB Mode** | Hardware CDC and JTAG |
| **Flash Size** | 16MB |
| **PSRAM** | OPI PSRAM |
| **Partition Scheme** | Custom |

8. Then, navigate to **Tools > Manage Libraries**.
9. Install the following libraries, make sure to install the right version:

| **Name** | **Version** |
| --- | --- |
| Adafruit BNO055 | 1.6.4 |
| Adafruit LSM6DS | 4.7.4 |
| Adafruit Unified Sensor | 1.1.15 |
| Adafruit GFX Library | 1.12.4 |
| Adafruit GC9A01A | 1.1.1 |
| Adafruit BusIO | 1.17.4 |
| Adafruit MAX1704X | 1.0.3 |

## 4. Uploading the Firmware
### 4.1 Using the arduino-cli

If this is the first time that you are uploading code to the ESP32-S3, then you must first enter download mode. To enter download mode, hold down the **BOOT** button on the PCB with the ESP32, then press and release the **RESET** button and then release the **BOOT** button. Your ESP32 should now be in download mode and you can now upload the firmware.

First, run the following in your command line:

```
.../Quantum-Dice-by-UTwente/firmware> arduino-cli board list
```

This will return a list of all the boards connected to your computer (that the arduino-cli recognizes) and their types.

Then, in your command line, run the following to upload to the ESP32:

```
.../Quantum-Dice-by-UTwente/firmware> arduino-cli upload main/ -p <PORT> --build-path build/
```

Where, `<PORT>` is the port that belongs to your ESP32.

Finally, press and release the **RESET** button on the PCB (only if you have the ESP32 in download mode).

### 4.2 Using the Arduino IDE

If this is the first time that you are uploading code to the ESP32-S3, then you must first enter download mode. To enter download mode, hold down the **BOOT** button on the PCB with the ESP32, then press and release the **RESET** button and then release the **BOOT** button. Your ESP32 should now be in download mode and you can now upload the firmware.

First, you must select the port, which you can do by either navigating to **Tools > Port** and selecting the port there, or by selecting one in the drop-down menu next to the verify (checkmark), upload (arrow pointing to the right) and start debugging (play triangle with a bug) buttons.

Then, simply press the upload button (arrow pointing to the right) to upload the compiled firmware to the ESP32.

Finally, press and release the **RESET** button on the PCB (only if you have the ESP32 in download mode).

## 5. Changing the Configuration

When starting the Quantum Dice for the first time, it will have a default configuration that it will load and save to the file system onboard. This configuration matches the settings stored in the [example config](EXAMPLE_config.txt).

For instructions on changing the settings in the config file, see [the Setup Guide](..SETUP.md).

---

*For additional support, refer to the QuantumDice GitHub repository or contact the UTwente development team.*
