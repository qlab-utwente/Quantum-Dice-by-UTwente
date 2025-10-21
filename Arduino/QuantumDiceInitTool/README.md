# Quantum Dice Init Tool - User Manual

## Overview

This tool provides a comprehensive setup interface for initializing ESP32-S3 based Quantum Dice devices. It handles MAC address retrieval, cryptographic chip configuration, IMU sensor calibration, and device configuration storage.

Once uploaded open Serial monitor 

---

## Menu Items Reference

### Main Menu Options

| Option | Function | Purpose | Permanent? |
|--------|----------|---------|------------|
| **1** | Get MAC Address | Retrieves and formats the device's unique MAC address for configuration | No |
| **2** | Configure ATECC508a | Initializes and locks the cryptographic co-processor with SparkFun settings | **YES - PERMANENT** |
| **3** | Calibrate BNO055 Sensor | Performs IMU sensor calibration and stores offsets in EEPROM | No (can recalibrate) |
| **4** | Test BNO055 Sensor | Displays live sensor data to verify calibration quality | No |
| **5** | Clear EEPROM | Erases all EEPROM data (calibration + configuration) | No (requires confirmation) |
| **6** | Configure EEPROM Settings | Interactive device configuration setup and storage | No (can reconfigure) |
| **M** | Show Menu | Redisplays the main menu | No |

---

## First-Time Setup Workflow

Follow this step-by-step guide to initialize a new device from scratch (empty EEPROM).


---

### Step 1: Get MAC Address

**Purpose:** Retrieve the unique MAC address needed for device identification and configuration.

1. Press **`1`** in the main menu
2. Wait for WiFi initialization (1-2 seconds)
3. You'll see output similar to:
```
   --- Getting MAC Address ---
   Original MAC Address: D0:CF:13:36:40:88
```

4. **Copy** the formatted MAC address line
5. **Save it** for use in Step 4 (Configuration)
6. Press **`M`** to return to menu

**Notes:**
- Each device has a unique MAC address
- You'll need this for Device A, B1, and B2 configurations

---

### Step 2: Configure ATECC508a Cryptographic Chip

**⚠️ WARNING: THIS STEP IS PERMANENT AND CANNOT BE UNDONE!**

**Purpose:** Initialize the hardware security chip for cryptographic operations and secure key storage. This device must be locked first before it can be used.

1. Press **`2`** in the main menu
2. The tool will detect the ATECC508a and display:
```
   ✓ ATECC508a detected - I2C connection good
   Serial Number: 0123456789ABCDEF
   Config Zone: NOT Locked
   Data/OTP Zone: NOT Locked
   Data Slot 0: NOT Locked
```


3. Type **`Y`** to confirm (or any other key to cancel)
4. The configuration process will run:
```
   Write Config:   ✓ Success
   Lock Config:    ✓ Success
   Key Creation:   ✓ Success
   Lock Data-OTP:  ✓ Success
   Lock Slot 0:    ✓ Success
```

5. Verify all steps show **`✓ Success`**
6. Press **`M`** to return to menu

**Notes:**
- The public key will be generated and displayed after locking
- If already configured, the tool will inform you and skip this step

---

### Step 3: Calibrate BNO055 IMU Sensor

**Purpose:** Calibrate the 9-axis IMU sensor for accurate motion detection and orientation tracking.

1. Press **`3`** in the main menu
2. The tool will detect the BNO055 and display sensor details
3. For new devices, it will show:
```
   ⚠ No calibration data found in EEPROM
   Starting calibration process...
```

4. **Calibration Process:**

   See [Bosch calibration instruction on Youtube](https://www.youtube.com/watch?v=Bw0WuAyGsnY)
   and [Adafruit calibration article](https://learn.adafruit.com/adafruit-bno055-absolute-orientation-sensor/device-calibration)
   
   The tool will display real-time calibration status:
```
   Cal: Sys:0 G:0 A:0 M:0 | Linear: 0.450 m/s² [⚠⚠ HIGH OFFSET - Keep calibrating!]
```

   **Calibration values range from 0 (uncalibrated) to 3 (fully calibrated):**
   - **Sys** = System (overall status)
   - **G** = Gyroscope
   - **A** = Accelerometer  
   - **M** = Magnetometer

5. **Perform these movements until all values reach 3:**

   **For Magnetometer (M):**
   - Move the device in a **figure-8 pattern**
   - Cover all orientations in 3D space
   - Continue until M reaches 3

   **For Gyroscope (G):**
   - **Slowly rotate** the device around all three axes
   - X-axis: rotate left/right
   - Y-axis: rotate forward/backward
   - Z-axis: rotate clockwise/counterclockwise
   
   **For Accelerometer (A):**
   - Place device in **6 different orientations**:
     - Face up, face down
     - Left side, right side
     - Top edge, bottom edge
   - Hold each position for 2-3 seconds

6. **Monitor Linear Acceleration Offset:**
```
   Linear: 0.120 m/s² [✓✓ Excellent offset!]
```
   
   **Target: < 0.15 m/s²**
   - **< 0.15**: Excellent ✓✓
   - **0.15-0.30**: Good ✓
   - **0.30-0.50**: Moderate ⚠
   - **> 0.50**: High - keep calibrating ⚠⚠

7. When fully calibrated (Sys:3 G:3 A:3 M:3):
```
   ✓ Fully calibrated!
   Final linear acceleration offset: 0.0823 m/s² - Excellent! ✓✓
   
   Saving calibration to EEPROM...
   ✓ Calibration saved!
```

8. Press **`M`** to return to menu

**Notes:**
- Calibration typically takes 2-5 minutes
- Move **slowly and smoothly** for best results
- Press **`Q`** anytime to quit without saving
- For acceleration testing 
- You can recalibrate anytime from the menu
- Use Option 4 to test sensor accuracy after calibration

---

### Step 4: Configure Device Settings in EEPROM

**Purpose:** Set up device-specific parameters including MAC addresses, display colors, and operational settings.

1. Press **`6`** in the main menu
2. For empty EEPROM, you'll see:
```
   ⚠ No valid configuration found in EEPROM
   Let's configure your device!
   
   Creating new configuration...
```

3. **Interactive Field-by-Field Configuration:**

   The tool will prompt for each setting with a default value shown in brackets:
```
   ----------------------------------------
   Dice ID [TEST1]: 
```

   - maximum of 5 characters
   - **Press ENTER** to accept the default value
   - **Type a new value** and press ENTER to change it
   - Important fields: Dice ID, Device A MAC, Device B MAC. The remaining fields the defaults are ok
   - When using the Quantum Dice for **Teleportation** demonstration, the MAC Address of the 3rd dice is needed at Device B2 MAC. 

4. **Configuration Fields:**

   | Field | Description | Example | Notes |
   |-------|-------------|---------|-------|
   | **Dice ID** | Unique identifier (max 15 chars) | `DICE_A` | Alpha-numeric |
   | **Device A MAC** | Primary device MAC address | `D0:CF:13:36:40:88` | From Step 1 |
   | **Device B1 MAC** | Secondary device 1 MAC | `D0:CF:13:33:58:5C` | From Step 1 |
   | **Device B2 MAC** | Secondary device 2 MAC | `DC:DA:0C:21:02:44` | From Step 1 |
   | **X Background Color** | Display color (hex) | `0x0000` | 16-bit RGB565 |
   | **Y Background Color** | Display color (hex) | `0x0000` | 16-bit RGB565 |
   | **Z Background Color** | Display color (hex) | `0x0000` | 16-bit RGB565 |
   | **Entanglement AB1 Color** | Connection color (hex) | `0xFFE0` | Yellow |
   | **Entanglement AB2 Color** | Connection color (hex) | `0x07E0` | Green |
   | **RSSI Limit** | Signal strength threshold (dBm) | `-35` | -100 to 0 |
   | **Is NANO board?** | Board type | `N` | Y=NANO, N=DEVKIT |
   | **Is SMD screen?** | Screen type | `Y` | Y=SMD, N=HDR |
   | **Always Seven mode?** | Debug mode | `N` | Y/N |
   | **Random Switch Point** | Randomness threshold (0-100) | `50` | 0-100 |
   | **Tumble Constant** | Motion sensitivity | `0.2` | 0.0-10.0 |
   | **Deep Sleep Timeout** | Auto-sleep time (seconds) | `300` | 10-3600 |

5. **MAC Address Entry:**
   
   Enter MAC addresses in the format: `AA:BB:CC:DD:EE:FF`
   
   Example:
```
   Device A MAC [D0:CF:13:36:40:88]
   Enter MAC (format: AA:BB:CC:DD:EE:FF) or press ENTER:
   D0:CF:13:36:40:88
   ✓ MAC set to: D0:CF:13:36:40:88
```

6. **Configuration Summary:**

   After entering all fields, review the complete configuration:
```
   ========================================
     Configuration Summary
   ========================================
   
   Dice ID: DICE_A
   Device A MAC:  D0:CF:13:36:40:88
   Device B1 MAC: D0:CF:13:33:58:5C
   ...
   
   Write this configuration to EEPROM?
   Type 'Y' to confirm or any other key to cancel:
```

7. Type **`Y`** to save to EEPROM
8. Verification will run automatically:
```
   ✓ Configuration written to EEPROM successfully!
   Verifying...
   ✓ Verification successful!
```

9. Press **`M`** to return to menu

**Notes:**
- Configuration can be changed anytime (not permanent)
- Existing values will be shown as defaults on subsequent edits
- All fields are validated before writing to EEPROM
- Invalid entries will show a warning and use the previous value

---

## Verification and Testing

After completing all setup steps, verify everything is working:

### Verify BNO055 Calibration

1. Press **`4`** (Test BNO055 Sensor)
2. Place device on a stable, flat surface
3. Observe the linear acceleration reading:
```
   Cal: Sys:3 G:3 A:3 M:3 | Linear: 0.098 m/s² [✓✓ Excellent] | X:0.012 Y:0.034 Z:0.023
```
4. **When stationary, linear acceleration should be < 0.40 m/s²**
5. Press any key to exit

### Verify Configuration

1. Press **`6`** (Configure EEPROM Settings)
2. Review the displayed configuration
3. Press **`N`** to keep current settings
4. All values should match your intended configuration

---

## Troubleshooting


### Calibration Won't Complete
- Move device more slowly
- Ensure no magnetic interference nearby
- Try recalibrating in a different location
- System value may reach 3 before all individual sensors do

### Configuration Validation Failed
- Check MAC address format (must be 12 hex digits)
- Verify RSSI limit is between -100 and 0
- Ensure Dice ID is not empty and < 16 characters
- Check numeric ranges for all parameters

### Double-Enter Issues
- Use Arduino Serial Monitor with "Newline" line ending
- Wait for prompt before entering next value
- If a field is skipped, restart configuration

---

## Safety Notes

⚠️ **CRITICAL WARNINGS:**

1. **ATECC508a Configuration:**
   - Configuration locks are **PERMANENT**
   - **Cannot be reversed or changed**
   - Once locked, the chip configuration is fixed for life
   - Only proceed when you are certain

2. **EEPROM Operations:**
   - Clear EEPROM erases **ALL** data (calibration + config)
   - Always backup important configurations
   - Clearing requires recalibration and reconfiguration

3. **Power During Operations:**
   - Keep device powered during EEPROM writes
   - Do not disconnect during ATECC508a locking
   - Stable USB power required throughout setup

---

## Quick Reference Card

**First-Time Setup Order:**
```
1 → Get MAC Address          [Copy and save]
2 → Configure ATECC508a      [⚠️ PERMANENT - Type Y]
3 → Calibrate BNO055         [Move device until Sys:3]
6 → Configure EEPROM         [Enter all fields, Type Y]
4 → Test BNO055              [Verify < 0.15 m/s²]
```

**Testing/Maintenance:**
```
4 → Test sensor readings
6 → View/edit configuration
3 → Recalibrate if needed
```

**Emergency:**
```
5 → Clear EEPROM [Erases everything - requires full reconfiguration]
```

---


*Document Version: 1.0*  
*Compatible with: ESP32-S3 Unified Sensor Setup Tool*
