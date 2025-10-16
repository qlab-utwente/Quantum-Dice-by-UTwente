# ESP32-S3 N16R8 First Upload Procedure

This guide explains how to upload your first sketch to a custom ESP32-S3 board with an N16R8 module using Arduino IDE.

## Prerequisites

- Arduino IDE installed (version 2.x recommended)
- USB data cable
- Your custom ESP32-S3 N16R8 board

## Setup Arduino IDE

### 1. Install ESP32 Board Support

1. Open Arduino IDE
2. Go to **File > Preferences**
3. In "Additional Boards Manager URLs" field, add:
https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json

4. Click **OK**
5. Go to **Tools > Board > Boards Manager**
6. Search for "ESP32"
7. Install **"esp32 by Espressif Systems"** (install the latest version)
8. Wait for installation to complete

### 2. Configure Board Settings

Select the following settings from the **Tools** menu:

- **Board**: ESP32S3 Dev Module
- **USB CDC On Boot**: Enabled *(critical for native USB)*
- **USB Mode**: USB-OTG (TinyUSB) or Hardware CDC and JTAG
- **Flash Size**: 16MB (128Mb)
- **PSRAM**: OPI PSRAM
- **Partition Scheme**: 16M Flash (3MB....)
- **Upload Speed**: 921600 (or lower if you experience issues)

## First Upload - Blink Example

### 1. Load the Blink Sketch

1. Go to **File > Examples > 01.Basics > Blink**
2. The Blink sketch will open in a new window
3. Modify the LED pin if needed (default is `LED_BUILTIN`, change to your board's LED pin if different)

### 2. Enter Download Mode

For the first upload, you must manually put the ESP32-S3 into download mode:

1. **Hold down** the **BOOT** button on your board
2. While holding BOOT, press the **RESET** button once
3. Release the **RESET** button
4. Release the **BOOT** button

Your board is now in download mode.

*Alternative method*: Hold the BOOT button while plugging in the USB cable.

### 3. Select COM Port

1. Go to **Tools > Port**
2. Select the COM port corresponding to your ESP32-S3
   - Windows: COM# (e.g., COM3)
   - Mac: /dev/cu.usbmodem# or /dev/tty.usbmodem#
   - Linux: /dev/ttyACM# or /dev/ttyUSB#

### 4. Upload the Sketch

1. Click the **Upload** button (right arrow icon) in Arduino IDE
2. Wait for the compilation and upload process to complete
3. You should see "Connecting..." followed by upload progress

### 5. Run Your Code

After upload completes:

1. **Press the RESET button** on your board
2. Your Blink sketch should now be running
3. Observe the LED blinking on your board

## Subsequent Uploads

For future uploads:

- If your board has auto-reset circuitry, you may not need to press BOOT
- If upload fails, repeat the BOOT + RESET button procedure from step 2
- You will need to manually press RESET after each upload for the code to start running (known ESP32-S3 silicon issue)

## Troubleshooting

### "Failed to connect to ESP32-S3" Error

- Ensure you pressed BOOT before attempting upload
- Try a different USB cable (must be a data cable, not charge-only)
- Lower the upload speed: **Tools > Upload Speed** → 115200
- Check that all board settings are correct

### No Serial Output

- Verify **USB CDC On Boot** is set to **Enabled**
- Press the RESET button after upload
- Check that you selected the correct COM port
- Open Serial Monitor at 115200 baud

### Board Not Detected

- Install USB drivers if needed (usually automatic on modern OS)
- Try different USB ports
- Check Device Manager (Windows) or `ls /dev/tty*` (Linux/Mac) to verify device appears

## Important Notes

- **First upload always requires BOOT button**: Manual download mode entry is necessary for the initial flash
- **Manual reset required**: Due to a known ESP32-S3 silicon bug, you must manually press RESET after each upload
- **USB CDC On Boot**: This setting enables the native USB serial, allowing you to use Serial.print() in your code
- **Pin compatibility**: Verify your custom board's LED pin number matches the sketch (change `LED_BUILTIN` if needed)

## Additional Resources

- [ESP32-S3 Datasheet](https://www.espressif.com/sites/default/files/documentation/esp32-s3_datasheet_en.pdf)
- [Arduino-ESP32 Documentation](https://docs.espressif.com/projects/arduino-esp32/en/latest/)
- [ESP32-S3 Technical Reference Manual](https://www.espressif.com/sites/default/files/documentation/esp32-s3_technical_reference_manual_en.pdf)
