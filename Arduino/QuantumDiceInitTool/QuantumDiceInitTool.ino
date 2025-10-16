/*
  ESP32-S3 Unified Sensor Setup Tool
  Combines MAC Address retrieval, ATECC508a configuration, and BNO055 calibration
  
  Hardware Requirements:
  - ESP32-S3 board
  - SparkFun ATECC508a Cryptographic Co-processor (optional)
  - Adafruit BNO055 IMU sensor (optional)
  - I2C/Qwiic connections
  
  Usage:
  1. Upload sketch to ESP32-S3
  2. Open Serial Monitor at 115200 baud
  3. Follow menu prompts to perform initialization tasks
*/

#include <Wire.h>
#include <WiFi.h>
#include <EEPROM.h>

// Conditional includes - comment out if sensors not present
#include <SparkFun_ATECCX08a_Arduino_Library.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <utility/imumaths.h>

// ==================== GLOBAL OBJECTS ====================
ATECCX08A atecc;
Adafruit_BNO055 bno = Adafruit_BNO055(55, 0x28);

// BNO055 sample rate
#define BNO055_SAMPLERATE_DELAY_MS (100)

// Sensor event variables
sensors_event_t orientationData, angVelocityData, linearAccelData;
sensors_event_t magnetometerData, accelerometerData, gravityData;

// ==================== FUNCTION PROTOTYPES ====================
void displayMainMenu();
void getMacAddress();
void configureATECC508a();
void calibrateBNO055();
void testBNO055();
void clearEEPROM();
void parseMacAddress(String macStr, uint8_t* macList);

// ATECC508a functions
void printATECCInfo();

// BNO055 functions
void displaySensorDetails();
void displaySensorStatus();
void displayCalStatus();
void displaySensorOffsets(const adafruit_bno055_offsets_t& calibData);
void performCalibration();
void printEvent(sensors_event_t* event);

// ==================== SETUP ====================
void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("\n\n");
  Serial.println("========================================");
  Serial.println("  ESP32-S3 Unified Sensor Setup Tool");
  Serial.println("========================================");
  Serial.println();
  
  // Initialize I2C
  Wire.begin();
  
  // Initialize EEPROM for BNO055 calibration storage
  EEPROM.begin(512);
  
  displayMainMenu();
}

// ==================== MAIN LOOP ====================
void loop() {
  if (Serial.available() > 0) {
    char input = Serial.read();
    
    // Clear any remaining characters in buffer
    delay(10);
    while (Serial.available() > 0) {
      Serial.read();
    }
    
    // Convert to uppercase
    if (input >= 'a' && input <= 'z') {
      input = input - 32;
    }
    
    Serial.println();
    
    switch (input) {
      case '1':
        getMacAddress();
        break;
        
      case '2':
        configureATECC508a();
        break;
        
      case '3':
        calibrateBNO055();
        break;
        
      case '4':
        testBNO055();
        break;
        
      case '5':
        clearEEPROM();
        break;
        
      case 'M':
        displayMainMenu();
        break;
        
      default:
        Serial.println("Invalid option. Please try again.");
        Serial.println("Type 'M' to show the menu.");
        break;
    }
    
    Serial.println();
  }
}

// ==================== MENU DISPLAY ====================
void displayMainMenu() {
  Serial.println("\n========================================");
  Serial.println("           MAIN MENU");
  Serial.println("========================================");
  Serial.println("1. Get MAC Address");
  Serial.println("2. Configure ATECC508a (PERMANENT)");
  Serial.println("3. Calibrate BNO055 Sensor");
  Serial.println("4. Test BNO055 Sensor (Live Data)");
  Serial.println("----------------------------------------");
  Serial.println("5. Clear EEPROM (Erase calibration)");
  Serial.println("========================================");
  Serial.println("M. Show this menu");
  Serial.println("========================================");
  Serial.println("Enter your choice (1-5 or M):");
}

// ==================== MAC ADDRESS ====================
void getMacAddress() {
  Serial.println("\n--- Getting MAC Address ---");
  
  // Initialize WiFi to get MAC address
  WiFi.mode(WIFI_STA);
  delay(1000);
  String macStr = WiFi.macAddress();
  
  // Print the original MAC address
  Serial.print("Original MAC Address: ");
  Serial.println(macStr);
  
  // Convert to hexadecimal array
  uint8_t macList[6];
  parseMacAddress(macStr, macList);
  
  // Print formatted for code
  Serial.println();
  Serial.print("inline uint8_t deviceX_mac[6] = { ");
  for (int i = 0; i < 6; i++) {
    Serial.print("0x");
    if (macList[i] < 16) Serial.print("0");
    Serial.print(macList[i], HEX);
    if (i < 5) Serial.print(", ");
  }
  Serial.println(" };");
  Serial.println();
  Serial.println("Copy the line above into diceConfig.h");
  Serial.println("Replace X with A or B as needed");
  
  // Turn off WiFi to save power
  WiFi.mode(WIFI_OFF);
  
  Serial.println("\nPress M for menu");
}

void parseMacAddress(String macStr, uint8_t* macList) {
  int j = 0;
  for (int i = 0; i < macStr.length(); i += 3) {
    macList[j] = strtol(macStr.substring(i, i + 2).c_str(), NULL, 16);
    j++;
  }
}

// ==================== ATECC508a CONFIGURATION ====================
void configureATECC508a() {
  Serial.println("\n--- ATECC508a Configuration ---");
  
  if (atecc.begin() == true) {
    Serial.println("✓ ATECC508a detected - I2C connection good");
  } else {
    Serial.println("✗ ATECC508a not found!");
    Serial.println("Check wiring and I2C address.");
    Serial.println("\nPress M for menu");
    return;
  }
  
  Serial.println();
  printATECCInfo();
  
  // Check if already fully configured
  if (atecc.configLockStatus && atecc.dataOTPLockStatus && atecc.slot0LockStatus) {
    Serial.println("\n✓ ATECC508a is already fully configured and locked!");
    Serial.println("All zones are locked - no further configuration needed.");
    Serial.println("The device is ready to use.");
    Serial.println("\nPress M for menu");
    return;
  }
  
  Serial.println("\n*** WARNING ***");
  Serial.println("Configuration settings are PERMANENT and cannot be changed!");
  Serial.println();
  Serial.println("Would you like to configure with SparkFun Standard settings?");
  Serial.println("Type 'Y' to proceed or any other key to cancel:");
  
  // Wait for user input
  while (Serial.available() == 0) {
    delay(10);
  }
  
  char response = Serial.read();
  while (Serial.available() > 0) Serial.read(); // Clear buffer
  
  if (response == 'Y' || response == 'y') {
    Serial.println("\n>>> Starting configuration... <<<\n");
    
    if (!atecc.configLockStatus) {
      Serial.print("Write Config:   ");
      if (atecc.writeConfigSparkFun() == true) {
        Serial.println("✓ Success");
      } else {
        Serial.println("✗ Failure");
      }
      
      Serial.print("Lock Config:    ");
      if (atecc.lockConfig() == true) {
        Serial.println("✓ Success");
      } else {
        Serial.println("✗ Failure");
      }
    } else {
      Serial.println("Config Zone:    Already locked ✓");
    }
    
    if (!atecc.dataOTPLockStatus) {
      Serial.print("Key Creation:   ");
      if (atecc.createNewKeyPair() == true) {
        Serial.println("✓ Success");
      } else {
        Serial.println("✗ Failure");
      }
      
      Serial.print("Lock Data-OTP:  ");
      if (atecc.lockDataAndOTP() == true) {
        Serial.println("✓ Success");
      } else {
        Serial.println("✗ Failure");
      }
    } else {
      Serial.println("Data-OTP Zone:  Already locked ✓");
    }
    
    if (!atecc.slot0LockStatus) {
      Serial.print("Lock Slot 0:    ");
      if (atecc.lockDataSlot0() == true) {
        Serial.println("✓ Success");
      } else {
        Serial.println("✗ Failure");
      }
    } else {
      Serial.println("Slot 0:         Already locked ✓");
    }
    
    Serial.println("\n>>> Configuration complete! <<<");
    Serial.println();
    printATECCInfo();
  } else {
    Serial.println("\nConfiguration cancelled.");
    Serial.println("Note: ATECC508a features require configuration.");
  }
  
  Serial.println("\nPress M for menu");
}

void printATECCInfo() {
  atecc.readConfigZone(false);
  
  Serial.print("Serial Number:  ");
  for (int i = 0; i < 9; i++) {
    if ((atecc.serialNumber[i] >> 4) == 0) Serial.print("0");
    Serial.print(atecc.serialNumber[i], HEX);
  }
  Serial.println();
  
  Serial.print("Rev Number:     ");
  for (int i = 0; i < 4; i++) {
    if ((atecc.revisionNumber[i] >> 4) == 0) Serial.print("0");
    Serial.print(atecc.revisionNumber[i], HEX);
  }
  Serial.println();
  
  Serial.print("Config Zone:    ");
  Serial.println(atecc.configLockStatus ? "Locked" : "NOT Locked");
  
  Serial.print("Data/OTP Zone:  ");
  Serial.println(atecc.dataOTPLockStatus ? "Locked" : "NOT Locked");
  
  Serial.print("Data Slot 0:    ");
  Serial.println(atecc.slot0LockStatus ? "Locked" : "NOT Locked");
  
  // If fully configured, show public key
  if (atecc.configLockStatus && atecc.dataOTPLockStatus && atecc.slot0LockStatus) {
    Serial.println();
    if (atecc.generatePublicKey() == false) {
      Serial.println("✗ Failed to generate public key");
    }
  }
}

// ==================== BNO055 CALIBRATION ====================
void calibrateBNO055() {
  Serial.println("\n--- BNO055 Calibration ---");
  
  if (!bno.begin()) {
    Serial.println("✗ BNO055 not detected!");
    Serial.println("Check wiring and I2C address (0x28 or 0x29).");
    Serial.println("\nPress M for menu");
    return;
  }
  
  Serial.println("✓ BNO055 detected");
  
  displaySensorDetails();
  displaySensorStatus();
  
  bno.setExtCrystalUse(true);
  delay(1000);
  
  // Check if calibration data already exists in EEPROM
  int eeAddress = 0;
  long bnoID;
  bool hasCalibration = false;
  
  EEPROM.get(eeAddress, bnoID);
  
  sensor_t sensor;
  bno.getSensor(&sensor);
  
  if (bnoID == sensor.sensor_id) {
    Serial.println("\n✓ Existing calibration data found in EEPROM!");
    
    // Load and display the existing calibration
    adafruit_bno055_offsets_t existingCalib;
    eeAddress += sizeof(long);
    EEPROM.get(eeAddress, existingCalib);
    
    Serial.println("\nStored calibration offsets:");
    displaySensorOffsets(existingCalib);
    
    Serial.println("\n\nOptions:");
    Serial.println("Y - Perform new calibration (overwrites existing)");
    Serial.println("N - Keep existing calibration and return to menu");
    Serial.print("\nYour choice: ");
    
    // Wait for user input
    while (Serial.available() == 0) {
      delay(10);
    }
    
    char response = Serial.read();
    while (Serial.available() > 0) Serial.read(); // Clear buffer
    
    Serial.println(response);
    
    if (response == 'Y' || response == 'y') {
      Serial.println("\nProceeding with recalibration...");
      performCalibration();
    } else {
      Serial.println("\nKeeping existing calibration data.");
      Serial.println("Use option 4 (Test Mode) to verify sensor readings.");
    }
  } else {
    Serial.println("\n⚠ No calibration data found in EEPROM");
    Serial.println("Starting calibration process...");
    performCalibration();
  }
  
  Serial.println("\nPress M for menu");
}

void performCalibration() {
  Serial.println("\n=== Starting Calibration Process ===");
  Serial.println("Please calibrate by:");
  Serial.println("• Moving sensor in figure-8 pattern (magnetometer)");
  Serial.println("• Rotating slowly around all axes (gyroscope)");
  Serial.println("• Placing in different orientations (accelerometer)");
  Serial.println();
  Serial.println("Goal: Minimize linear acceleration offset (should be near 0.0 m/s²)");
  Serial.println(">>> Type 'Q' at any time to quit calibration <<<");
  Serial.println("------------------------------------------------------------");
  
  bool calibrationAborted = false;
  
  while (!bno.isFullyCalibrated()) {
    // Check for user interrupt
    if (Serial.available() > 0) {
      char input = Serial.read();
      // Clear buffer
      while (Serial.available() > 0) Serial.read();
      
      // Convert to uppercase
      if (input >= 'a' && input <= 'z') {
        input = input - 32;
      }
      
      if (input == 'Q') {
        Serial.println("\n\n*** Calibration aborted by user ***");
        calibrationAborted = true;
        break;
      }
    }
    
    bno.getEvent(&linearAccelData, Adafruit_BNO055::VECTOR_LINEARACCEL);
    
    // Calculate magnitude
    double x = linearAccelData.acceleration.x;
    double y = linearAccelData.acceleration.y;
    double z = linearAccelData.acceleration.z;
    double mag = sqrt(x * x + y * y + z * z);
    
    // Display calibration status first
    displayCalStatus();
    
    // Display linear acceleration with clear offset warning
    Serial.print(" | Linear: ");
    Serial.print(mag, 3);
    Serial.print(" m/s² ");
    
    // Color-coded threshold indicators
    if (mag > 0.5) {
      Serial.print("[⚠⚠ HIGH OFFSET - Keep calibrating! ]");
    } else if (mag > 0.3) {
      Serial.print("[⚠ Moderate offset - Almost there  ]");
    } else if (mag > 0.15) {
      Serial.print("[✓ Good - Continue for best results]");
    } else {
      Serial.print("[✓✓ Excellent offset!              ]");
    }
    
    Serial.println();
    delay(BNO055_SAMPLERATE_DELAY_MS);
  }
  
  // If calibration was aborted, exit without saving
  if (calibrationAborted) {
    Serial.println("Calibration data NOT saved.");
    Serial.println("Previous calibration (if any) remains in EEPROM.");
    return;
  }
  
  Serial.println("\n✓ Fully calibrated!");
  
  // Verify final offset
  Serial.println("\nVerifying calibration quality...");
  delay(500);
  bno.getEvent(&linearAccelData, Adafruit_BNO055::VECTOR_LINEARACCEL);
  double x = linearAccelData.acceleration.x;
  double y = linearAccelData.acceleration.y;
  double z = linearAccelData.acceleration.z;
  double finalMag = sqrt(x * x + y * y + z * z);
  
  Serial.print("Final linear acceleration offset: ");
  Serial.print(finalMag, 4);
  Serial.print(" m/s² ");
  
  if (finalMag < 0.15) {
    Serial.println("- Excellent! ✓✓");
  } else if (finalMag < 0.3) {
    Serial.println("- Good ✓");
  } else {
    Serial.println("- Consider recalibrating for better accuracy ⚠");
  }
  
  Serial.println("================================");
  
  // Get and display calibration data
  adafruit_bno055_offsets_t newCalib;
  bno.getSensorOffsets(newCalib);
  displaySensorOffsets(newCalib);
  
  // Save to EEPROM
  Serial.println("\n\nSaving calibration to EEPROM...");
  
  int eeAddress = 0;
  sensor_t sensor;
  bno.getSensor(&sensor);
  long bnoID = sensor.sensor_id;
  
  EEPROM.put(eeAddress, bnoID);
  eeAddress += sizeof(long);
  EEPROM.put(eeAddress, newCalib);
  EEPROM.commit();
  
  Serial.println("✓ Calibration saved!");
  Serial.println("================================");
}

// ==================== BNO055 TEST MODE ====================
void testBNO055() {
  Serial.println("\n--- BNO055 Test Mode ---");
  
  if (!bno.begin()) {
    Serial.println("✗ BNO055 not detected!");
    Serial.println("Check wiring and I2C address.");
    Serial.println("\nPress M for menu");
    return;
  }
  
  Serial.println("✓ BNO055 detected");
  
  // Try to load calibration from EEPROM
  int eeAddress = 0;
  long bnoID;
  EEPROM.get(eeAddress, bnoID);
  
  sensor_t sensor;
  bno.getSensor(&sensor);
  
  if (bnoID == sensor.sensor_id) {
    Serial.println("✓ Loading stored calibration...");
    adafruit_bno055_offsets_t calibData;
    eeAddress += sizeof(long);
    EEPROM.get(eeAddress, calibData);
    bno.setSensorOffsets(calibData);
  } else {
    Serial.println("⚠ No calibration found - sensor may be less accurate");
  }
  
  bno.setExtCrystalUse(true);
  delay(500);
  
  Serial.println("\n>>> Live Sensor Data <<<");
  Serial.println("Monitoring linear acceleration offset - should be < 0.15 m/s² when stationary");
  Serial.println("Press any key to return to menu\n");
  Serial.println("--------------------------------------------------------------------");
  
  // Display live data until user presses a key
  while (Serial.available() == 0) {
    bno.getEvent(&angVelocityData, Adafruit_BNO055::VECTOR_GYROSCOPE);
    bno.getEvent(&linearAccelData, Adafruit_BNO055::VECTOR_LINEARACCEL);
    bno.getEvent(&gravityData, Adafruit_BNO055::VECTOR_GRAVITY);
    
    // Calculate linear acceleration magnitude
    double x = linearAccelData.acceleration.x;
    double y = linearAccelData.acceleration.y;
    double z = linearAccelData.acceleration.z;
    double mag = sqrt(x * x + y * y + z * z);
    
    // Display calibration status
    displayCalStatus();
    Serial.print(" | ");
    
    // Display linear acceleration with alert
    Serial.print("Linear: ");
    Serial.print(mag, 3);
    Serial.print(" m/s²");
    
    // Alert if magnitude exceeds threshold
    if (mag > 0.4) {
      Serial.print(" [⚠⚠ HIGH - Recalibrate!]");
    } else if (mag > 0.2) {
      Serial.print(" [⚠ Elevated          ]");
    } else if (mag > 0.15) {
      Serial.print(" [✓ Acceptable        ]");
    } else {
      Serial.print(" [✓✓ Excellent        ]");
    }
    
    Serial.print(" | X:");
    Serial.print(x, 3);
    Serial.print(" Y:");
    Serial.print(y, 3);
    Serial.print(" Z:");
    Serial.print(z, 3);
    
    Serial.println();
    
    delay(BNO055_SAMPLERATE_DELAY_MS);
  }
  
  // Clear input buffer
  while (Serial.available() > 0) Serial.read();
  
  Serial.println("\nExiting test mode...");
  Serial.println("Press M for menu");
}

// ==================== BNO055 HELPER FUNCTIONS ====================
void displaySensorDetails() {
  sensor_t sensor;
  bno.getSensor(&sensor);
  Serial.println("------------------------------------");
  Serial.print("Sensor:       "); Serial.println(sensor.name);
  Serial.print("Driver Ver:   "); Serial.println(sensor.version);
  Serial.print("Unique ID:    "); Serial.println(sensor.sensor_id);
  Serial.print("Max Value:    "); Serial.print(sensor.max_value); Serial.println(" xxx");
  Serial.print("Min Value:    "); Serial.print(sensor.min_value); Serial.println(" xxx");
  Serial.print("Resolution:   "); Serial.print(sensor.resolution); Serial.println(" xxx");
  Serial.println("------------------------------------");
}

void displaySensorStatus() {
  uint8_t system_status, self_test_results, system_error;
  system_status = self_test_results = system_error = 0;
  bno.getSystemStatus(&system_status, &self_test_results, &system_error);
  
  Serial.print("System Status: 0x"); Serial.println(system_status, HEX);
  Serial.print("Self Test:     0x"); Serial.println(self_test_results, HEX);
  Serial.print("System Error:  0x"); Serial.println(system_error, HEX);
  Serial.println();
}

void displayCalStatus() {
  uint8_t system, gyro, accel, mag;
  system = gyro = accel = mag = 0;
  bno.getCalibration(&system, &gyro, &accel, &mag);
  
  if (!system) Serial.print("! ");
  Serial.print("Cal: Sys:");
  Serial.print(system, DEC);
  Serial.print(" G:");
  Serial.print(gyro, DEC);
  Serial.print(" A:");
  Serial.print(accel, DEC);
  Serial.print(" M:");
  Serial.print(mag, DEC);
}

// ==================== CLEAR EEPROM ====================
void clearEEPROM() {
  Serial.println("\n--- Clear EEPROM ---");
  Serial.println();
  Serial.println("*** WARNING ***");
  Serial.println("This will erase ALL calibration data stored in EEPROM!");
  Serial.println("After clearing, you will need to recalibrate the BNO055 sensor.");
  Serial.println();
  Serial.println("Are you sure you want to clear EEPROM?");
  Serial.println("Type 'Y' to confirm or any other key to cancel:");
  
  // Wait for user input
  while (Serial.available() == 0) {
    delay(10);
  }
  
  char response = Serial.read();
  while (Serial.available() > 0) Serial.read(); // Clear buffer
  
  if (response == 'Y' || response == 'y') {
    Serial.println("\nClearing EEPROM...");
    
    // Write zeros to entire EEPROM space
    for (int i = 0; i < 512; i++) {
      EEPROM.write(i, 0);
    }
    EEPROM.commit();
    
    Serial.println("✓ EEPROM cleared successfully!");
    Serial.println();
    Serial.println("All calibration data has been erased.");
    Serial.println("Use option 3 to recalibrate the BNO055 sensor.");
  } else {
    Serial.println("\nOperation cancelled. EEPROM data preserved.");
  }
  
  Serial.println("\nPress M for menu");
}

void displaySensorOffsets(const adafruit_bno055_offsets_t& calibData) {
  Serial.print("Accel: ");
  Serial.print(calibData.accel_offset_x); Serial.print(" ");
  Serial.print(calibData.accel_offset_y); Serial.print(" ");
  Serial.print(calibData.accel_offset_z);
  
  Serial.print(" | Gyro: ");
  Serial.print(calibData.gyro_offset_x); Serial.print(" ");
  Serial.print(calibData.gyro_offset_y); Serial.print(" ");
  Serial.print(calibData.gyro_offset_z);
  
  Serial.print(" | Mag: ");
  Serial.print(calibData.mag_offset_x); Serial.print(" ");
  Serial.print(calibData.mag_offset_y); Serial.print(" ");
  Serial.print(calibData.mag_offset_z);
  
  Serial.print(" | Radius: A:");
  Serial.print(calibData.accel_radius);
  Serial.print(" M:");
  Serial.println(calibData.mag_radius);
}

void printEvent(sensors_event_t* event) {
  double x = -1000000, y = -1000000, z = -1000000, mag = 0;
  
  if (event->type == SENSOR_TYPE_ACCELEROMETER) {
    Serial.print("Accl:");
    x = event->acceleration.x;
    y = event->acceleration.y;
    z = event->acceleration.z;
  } else if (event->type == SENSOR_TYPE_ORIENTATION) {
    Serial.print("Orient:");
    x = event->orientation.x;
    y = event->orientation.y;
    z = event->orientation.z;
  } else if (event->type == SENSOR_TYPE_MAGNETIC_FIELD) {
    Serial.print("Mag:");
    x = event->magnetic.x;
    y = event->magnetic.y;
    z = event->magnetic.z;
  } else if (event->type == SENSOR_TYPE_GYROSCOPE) {
    Serial.print("Gyro:");
    x = event->gyro.x;
    y = event->gyro.y;
    z = event->gyro.z;
  } else if (event->type == SENSOR_TYPE_LINEAR_ACCELERATION) {
    Serial.print("Linear:");
    x = event->acceleration.x;
    y = event->acceleration.y;
    z = event->acceleration.z;
    mag = sqrt(x * x + y * y + z * z);
    Serial.print(mag);
    if (mag > 0.4) Serial.print(" ⚠ALARM");
  } else if (event->type == SENSOR_TYPE_GRAVITY) {
    Serial.print("Gravity:");
    x = event->acceleration.x;
    y = event->acceleration.y;
    z = event->acceleration.z;
  } else {
    Serial.print("Unk:");
  }
  
  Serial.print("\tx=");
  Serial.print(x);
  Serial.print(" |\ty=");
  Serial.print(y);
  Serial.print(" |\tz=");
  Serial.println(z);
}