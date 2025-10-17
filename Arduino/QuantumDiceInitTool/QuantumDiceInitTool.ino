/*
  ESP32-S3 Unified Sensor Setup Tool with EEPROM Configuration
  Combines MAC Address retrieval, ATECC508a configuration, BNO055 calibration,
  and EEPROM configuration management
  
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
#include <Adafruit_GC9A01A.h>  // For color constants
#include <utility/imumaths.h>

// ==================== EEPROM MEMORY LAYOUT ====================
#define EEPROM_SIZE 512
#define EEPROM_BNO_SENSOR_ID_ADDR 0
#define EEPROM_BNO_CALIBRATION_ADDR 4
#define EEPROM_CONFIG_ADDRESS 24

// ==================== DICE CONFIGURATION STRUCTURE ====================
struct DiceConfig {
  char diceId[16];
  uint8_t deviceA_mac[6];
  uint8_t deviceB1_mac[6];
  uint8_t deviceB2_mac[6];
  uint16_t x_background;
  uint16_t y_background;
  uint16_t z_background;
  uint16_t entang_ab1_color;
  uint16_t entang_ab2_color;
  int8_t rssiLimit;
  bool isSMD;
  bool isNano;
  bool alwaysSeven;
  uint8_t randomSwitchPoint;
  float tumbleConstant;
  uint32_t deepSleepTimeout;
};

// Default configuration
const DiceConfig defaultConfig = {
  .diceId = "TEST1",
  .deviceA_mac = { 0xD0, 0xCF, 0x13, 0x36, 0x40, 0x88 },
  .deviceB1_mac = { 0xD0, 0xCF, 0x13, 0x33, 0x58, 0x5C },
  .deviceB2_mac = { 0xDC, 0xDA, 0x0C, 0x21, 0x02, 0x44 },
  .x_background = GC9A01A_BLACK,
  .y_background = GC9A01A_BLACK,
  .z_background = GC9A01A_BLACK,
  .entang_ab1_color = GC9A01A_YELLOW,
  .entang_ab2_color = GC9A01A_GREEN,
  .rssiLimit = -35,
  .isSMD = true,
  .isNano = false,
  .alwaysSeven = false,
  .randomSwitchPoint = 50,
  .tumbleConstant = 0.2,
  .deepSleepTimeout = 300000  // 5 minutes
};

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
void configureEEPROMSettings();
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

// EEPROM Configuration functions
bool validateConfig(const DiceConfig& config);
void printConfig(const DiceConfig& config, const char* title);
bool readEEPROMConfig(DiceConfig& config);
void writeEEPROMConfig(const DiceConfig& config);
String readSerialLine();
bool readMacFromSerial(uint8_t* mac);
uint16_t readColorFromSerial();

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
  
  // Initialize EEPROM
  EEPROM.begin(EEPROM_SIZE);
  
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
        
      case '6':
        configureEEPROMSettings();
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
  Serial.println("5. Clear EEPROM (Erase calibration)");
  Serial.println("6. Configure EEPROM Settings");
  Serial.println("========================================");
  Serial.println("M. Show this menu");
  Serial.println("========================================");
  Serial.println("Enter your choice (1-6 or M):");
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
  int eeAddress = EEPROM_BNO_SENSOR_ID_ADDR;
  long bnoID;
  
  EEPROM.get(eeAddress, bnoID);
  
  sensor_t sensor;
  bno.getSensor(&sensor);

  Serial.print("Current sensor ID: ");
  Serial.println(sensor.sensor_id);
  Serial.print("EEPROM stored ID: ");
  Serial.println(bnoID);

  if (bnoID == sensor.sensor_id) {
    Serial.println("\n✓ Existing calibration data found in EEPROM!");
    
    // Load and display the existing calibration
    adafruit_bno055_offsets_t existingCalib;
    eeAddress = EEPROM_BNO_CALIBRATION_ADDR;
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
  
  int eeAddress = EEPROM_BNO_SENSOR_ID_ADDR;
  sensor_t sensor;
  bno.getSensor(&sensor);
  long bnoID = sensor.sensor_id;
  
  EEPROM.put(eeAddress, bnoID);
  eeAddress = EEPROM_BNO_CALIBRATION_ADDR;
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
  int eeAddress = EEPROM_BNO_SENSOR_ID_ADDR;
  long bnoID;
  EEPROM.get(eeAddress, bnoID);
  
  sensor_t sensor;
  bno.getSensor(&sensor);
  
  if (bnoID == sensor.sensor_id) {
    Serial.println("✓ Loading stored calibration...");
    adafruit_bno055_offsets_t calibData;
    eeAddress = EEPROM_BNO_CALIBRATION_ADDR;
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
    if (mag > 0.4) Serial.print(" ⚠ ALARM");
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

// ==================== CLEAR EEPROM ====================
void clearEEPROM() {
  Serial.println("\n--- Clear EEPROM ---");
  Serial.println();
  Serial.println("*** WARNING ***");
  Serial.println("This will erase ALL data stored in EEPROM!");
  Serial.println("Including: BNO055 calibration AND device configuration");
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
    for (int i = 0; i < EEPROM_SIZE; i++) {
      EEPROM.write(i, 0);
    }
    EEPROM.commit();
    
    Serial.println("✓ EEPROM cleared successfully!");
    Serial.println();
    Serial.println("All data has been erased.");
    Serial.println("Use option 3 to recalibrate BNO055");
    Serial.println("Use option 6 to reconfigure device settings");
  } else {
    Serial.println("\nOperation cancelled. EEPROM data preserved.");
  }
  
  Serial.println("\nPress M for menu");
}

// ==================== EEPROM CONFIGURATION ====================
void configureEEPROMSettings() {
  Serial.println("\n========================================");
  Serial.println("    EEPROM Configuration Setup");
  Serial.println("========================================\n");
  
  // Check if configuration already exists
  DiceConfig existingConfig;
  bool hasExisting = readEEPROMConfig(existingConfig);
  
  if (hasExisting) {
    Serial.println("✓ Current configuration found in EEPROM:\n");
    printConfig(existingConfig, "Current Configuration");
    Serial.println("\nOptions:");
    Serial.println("  N - Keep current configuration");
    Serial.println("  Y - Configure new settings");
    Serial.print("\nYour choice: ");
    
    while (Serial.available() == 0) delay(10);
    char response = Serial.read();
    
    // Consume any trailing newlines
    delay(50);
    while (Serial.available() > 0) {
      char c = Serial.peek();
      if (c == '\n' || c == '\r') {
        Serial.read();
      } else {
        break;
      }
    }
    
    Serial.println(response);
    
    if (response != 'Y' && response != 'y') {
      Serial.println("\nKeeping current configuration.");
      Serial.println("Press M for menu");
      return;
    }
  } else {
    Serial.println("⚠ No valid configuration found in EEPROM");
    Serial.println("Let's configure your device!\n");
  }
  
  // Create new configuration starting with appropriate defaults
  DiceConfig newConfig;
  DiceConfig displayDefaults;
  
  if (hasExisting) {
    // Use existing config as the base
    newConfig = existingConfig;
    displayDefaults = existingConfig;
    Serial.println("\nEditing existing configuration...");
  } else {
    // Use hardcoded defaults
    newConfig = defaultConfig;
    displayDefaults = defaultConfig;
    Serial.println("\nCreating new configuration...");
  }
  
  Serial.println("\n========================================");
  Serial.println("  Interactive Configuration");
  Serial.println("========================================");
  Serial.println("Press ENTER to accept default, or type new value\n");
  
  // Dice ID
  Serial.println("----------------------------------------");
  Serial.printf("Dice ID [%s]: ", displayDefaults.diceId);
  String input = readSerialLine();
  if (input.length() > 0) {
    strncpy(newConfig.diceId, input.c_str(), 15);
    newConfig.diceId[15] = '\0';
  }
  
  // Device A MAC
  Serial.println("\n----------------------------------------");
  Serial.print("Device A MAC [");
  for (int i = 0; i < 6; i++) {
    Serial.printf("%02X", displayDefaults.deviceA_mac[i]);
    if (i < 5) Serial.print(":");
  }
  Serial.println("]");
  Serial.println("Enter MAC (format: AA:BB:CC:DD:EE:FF) or press ENTER:");
  if (!readMacFromSerial(newConfig.deviceA_mac)) {
    // Keep current/default
    memcpy(newConfig.deviceA_mac, displayDefaults.deviceA_mac, 6);
  }
  
  // Device B1 MAC
  Serial.println("\n----------------------------------------");
  Serial.print("Device B1 MAC [");
  for (int i = 0; i < 6; i++) {
    Serial.printf("%02X", displayDefaults.deviceB1_mac[i]);
    if (i < 5) Serial.print(":");
  }
  Serial.println("]");
  Serial.println("Enter MAC (format: AA:BB:CC:DD:EE:FF) or press ENTER:");
  if (!readMacFromSerial(newConfig.deviceB1_mac)) {
    memcpy(newConfig.deviceB1_mac, displayDefaults.deviceB1_mac, 6);
  }
  
  // Device B2 MAC
  Serial.println("\n----------------------------------------");
  Serial.print("Device B2 MAC [");
  for (int i = 0; i < 6; i++) {
    Serial.printf("%02X", displayDefaults.deviceB2_mac[i]);
    if (i < 5) Serial.print(":");
  }
  Serial.println("]");
  Serial.println("Enter MAC (format: AA:BB:CC:DD:EE:FF) or press ENTER:");
  if (!readMacFromSerial(newConfig.deviceB2_mac)) {
    memcpy(newConfig.deviceB2_mac, displayDefaults.deviceB2_mac, 6);
  }
  
  // X Background Color
  Serial.println("\n----------------------------------------");
  Serial.printf("X Background Color [0x%04X]: ", displayDefaults.x_background);
  input = readSerialLine();
  if (input.length() > 0) {
    newConfig.x_background = strtoul(input.c_str(), NULL, 16);
  }
  
  // Y Background Color
  Serial.printf("Y Background Color [0x%04X]: ", displayDefaults.y_background);
  input = readSerialLine();
  if (input.length() > 0) {
    newConfig.y_background = strtoul(input.c_str(), NULL, 16);
  }
  
  // Z Background Color
  Serial.printf("Z Background Color [0x%04X]: ", displayDefaults.z_background);
  input = readSerialLine();
  if (input.length() > 0) {
    newConfig.z_background = strtoul(input.c_str(), NULL, 16);
  }
  
  // Entanglement AB1 Color
  Serial.printf("Entanglement AB1 Color [0x%04X]: ", displayDefaults.entang_ab1_color);
  input = readSerialLine();
  if (input.length() > 0) {
    newConfig.entang_ab1_color = strtoul(input.c_str(), NULL, 16);
  }
  
  // Entanglement AB2 Color
  Serial.printf("Entanglement AB2 Color [0x%04X]: ", displayDefaults.entang_ab2_color);
  input = readSerialLine();
  if (input.length() > 0) {
    newConfig.entang_ab2_color = strtoul(input.c_str(), NULL, 16);
  }
  
  // RSSI Limit
  Serial.println("\n----------------------------------------");
  Serial.printf("RSSI Limit in dBm [%d]: ", displayDefaults.rssiLimit);
  input = readSerialLine();
  if (input.length() > 0) {
    newConfig.rssiLimit = input.toInt();
  }
  
  // Board Type
  Serial.println("\n----------------------------------------");
  Serial.printf("Is NANO board? (Y/N) [%s]: ", displayDefaults.isNano ? "Y" : "N");
  input = readSerialLine();
  if (input.length() > 0) {
    newConfig.isNano = (input[0] == 'Y' || input[0] == 'y');
  }
  
  // Screen Type
  Serial.printf("Is SMD screen? (Y/N) [%s]: ", displayDefaults.isSMD ? "Y" : "N");
  input = readSerialLine();
  if (input.length() > 0) {
    newConfig.isSMD = (input[0] == 'Y' || input[0] == 'y');
  }
  
  // Always Seven
  Serial.println("\n----------------------------------------");
  Serial.printf("Always Seven mode? (Y/N) [%s]: ", displayDefaults.alwaysSeven ? "Y" : "N");
  input = readSerialLine();
  if (input.length() > 0) {
    newConfig.alwaysSeven = (input[0] == 'Y' || input[0] == 'y');
  }
  
  // Random Switch Point
  Serial.printf("Random Switch Point (0-100) [%d]: ", displayDefaults.randomSwitchPoint);
  input = readSerialLine();
  if (input.length() > 0) {
    newConfig.randomSwitchPoint = input.toInt();
  }
  
  // Tumble Constant
  Serial.printf("Tumble Constant [%.2f]: ", displayDefaults.tumbleConstant);
  input = readSerialLine();
  if (input.length() > 0) {
    newConfig.tumbleConstant = input.toFloat();
  }
  
  // Deep Sleep Timeout
  Serial.println("\n----------------------------------------");
  Serial.printf("Deep Sleep Timeout in seconds [%d]: ", displayDefaults.deepSleepTimeout / 1000);
  input = readSerialLine();
  if (input.length() > 0) {
    newConfig.deepSleepTimeout = input.toInt() * 1000;
  }
  
  // Show summary and confirm
  Serial.println("\n========================================");
  Serial.println("  Configuration Summary");
  Serial.println("========================================\n");
  printConfig(newConfig, "New Configuration");
  
  Serial.println("\nWrite this configuration to EEPROM?");
  Serial.println("Type 'Y' to confirm or any other key to cancel:");
  
  while (Serial.available() == 0) delay(10);
  char response = Serial.read();
  
  // Consume any trailing newlines
  delay(50);
  while (Serial.available() > 0) {
    char c = Serial.peek();
    if (c == '\n' || c == '\r') {
      Serial.read();
    } else {
      break;
    }
  }
  
  if (response == 'Y' || response == 'y') {
    // Validate before writing
    if (validateConfig(newConfig)) {
      writeEEPROMConfig(newConfig);
      Serial.println("\n✓ Configuration written to EEPROM successfully!");
      
      // Verify by reading back
      Serial.println("\nVerifying...");
      DiceConfig verifyConfig;
      if (readEEPROMConfig(verifyConfig)) {
        Serial.println("✓ Verification successful!");
      } else {
        Serial.println("⚠ Verification failed - please check configuration");
      }
    } else {
      Serial.println("\n✗ Configuration validation failed!");
      Serial.println("Configuration was NOT written to EEPROM");
    }
  } else {
    Serial.println("\nConfiguration cancelled. EEPROM unchanged.");
  }
  
  Serial.println("\nPress M for menu");
}

// ==================== EEPROM HELPER FUNCTIONS ====================

String readSerialLine() {
  String input = "";
  unsigned long startTime = millis();
  bool gotNewline = false;
  
  // Wait for input with 30 second timeout
  while (millis() - startTime < 30000) {
    if (Serial.available() > 0) {
      char c = Serial.read();
      
      if (c == '\n' || c == '\r') {
        if (input.length() > 0) {
          gotNewline = true;
          break;
        } else {
          // Empty input - just newline pressed
          gotNewline = true;
          // Continue reading briefly to consume any paired \r\n or \n\r
          unsigned long newlineTime = millis();
          while (millis() - newlineTime < 50) {
            if (Serial.available() > 0) {
              char nc = Serial.peek();
              if (nc == '\n' || nc == '\r') {
                Serial.read(); // Consume it
              } else {
                break;
              }
            }
            delay(5);
          }
          break;
        }
      } else if (c >= 32 && c <= 126) {  // Printable characters
        input += c;
        Serial.print(c);  // Echo character
      }
    }
    delay(10);
  }
  
  // If we got a newline, consume any remaining newline characters
  if (gotNewline) {
    delay(50);  // Wait for any trailing newlines
    while (Serial.available() > 0) {
      char c = Serial.peek();
      if (c == '\n' || c == '\r') {
        Serial.read();
      } else {
        break;
      }
    }
  }
  
  Serial.println();  // Newline after input
  return input;
}

bool readMacFromSerial(uint8_t* mac) {
  String input = readSerialLine();
  
  if (input.length() == 0) {
    return false;  // Use default
  }
  
  // Remove colons and spaces
  input.replace(":", "");
  input.replace(" ", "");
  input.toUpperCase();
  
  // Check length
  if (input.length() != 12) {
    Serial.println("⚠ Invalid MAC format, using default");
    return false;
  }
  
  // Parse hex values
  for (int i = 0; i < 6; i++) {
    String byteStr = input.substring(i * 2, i * 2 + 2);
    mac[i] = strtoul(byteStr.c_str(), NULL, 16);
  }
  
  Serial.print("✓ MAC set to: ");
  for (int i = 0; i < 6; i++) {
    Serial.printf("%02X", mac[i]);
    if (i < 5) Serial.print(":");
  }
  Serial.println();
  
  return true;
}

bool validateConfig(const DiceConfig& config) {
  Serial.println("\nValidating configuration...");
  
  // Check if diceId is null-terminated and contains printable characters
  bool validId = false;
  for (int i = 0; i < 16; i++) {
    if (config.diceId[i] == '\0') {
      validId = (i > 0);
      break;
    }
    if (!isprint(config.diceId[i])) {
      Serial.println("  ✗ Invalid diceId characters");
      return false;
    }
  }
  
  if (!validId) {
    Serial.println("  ✗ Invalid diceId format");
    return false;
  }
  
  // Validate RSSI limit
  if (config.rssiLimit > 0 || config.rssiLimit < -100) {
    Serial.printf("  ✗ Invalid RSSI limit: %d\n", config.rssiLimit);
    return false;
  }
  
  // Validate randomSwitchPoint
  if (config.randomSwitchPoint > 100) {
    Serial.printf("  ✗ Invalid randomSwitchPoint: %d\n", config.randomSwitchPoint);
    return false;
  }
  
  // Validate tumbleConstant
  if (config.tumbleConstant <= 0 || config.tumbleConstant > 10.0) {
    Serial.printf("  ✗ Invalid tumbleConstant: %.2f\n", config.tumbleConstant);
    return false;
  }
  
  // Validate deepSleepTimeout
  if (config.deepSleepTimeout < 10000 || config.deepSleepTimeout > 3600000) {
    Serial.printf("  ✗ Invalid deepSleepTimeout: %lu ms\n", config.deepSleepTimeout);
    return false;
  }
  
  Serial.println("✓ All validation checks passed");
  return true;
}

void printConfig(const DiceConfig& config, const char* title) {
  Serial.println("========================================");
  Serial.println(title);
  Serial.println("========================================");
  
  Serial.print("Dice ID: ");
  Serial.println(config.diceId);
  
  Serial.print("Device A MAC:  ");
  for (int i = 0; i < 6; i++) {
    Serial.printf("%02X", config.deviceA_mac[i]);
    if (i < 5) Serial.print(":");
  }
  Serial.println();
  
  Serial.print("Device B1 MAC: ");
  for (int i = 0; i < 6; i++) {
    Serial.printf("%02X", config.deviceB1_mac[i]);
    if (i < 5) Serial.print(":");
  }
  Serial.println();
  
  Serial.print("Device B2 MAC: ");
  for (int i = 0; i < 6; i++) {
    Serial.printf("%02X", config.deviceB2_mac[i]);
    if (i < 5) Serial.print(":");
  }
  Serial.println();
  
  Serial.println("\nDisplay Colors:");
  Serial.printf("  X Background:       0x%04X\n", config.x_background);
  Serial.printf("  Y Background:       0x%04X\n", config.y_background);
  Serial.printf("  Z Background:       0x%04X\n", config.z_background);
  Serial.printf("  Entanglement AB1:   0x%04X\n", config.entang_ab1_color);
  Serial.printf("  Entanglement AB2:   0x%04X\n", config.entang_ab2_color);
  
  Serial.println("\nHardware Configuration:");
  Serial.printf("  Board Type:         %s\n", config.isNano ? "NANO" : "DEVKIT");
  Serial.printf("  Screen Type:        %s\n", config.isSMD ? "SMD" : "HDR");
  
  Serial.println("\nOperational Parameters:");
  Serial.printf("  RSSI Limit:         %d dBm\n", config.rssiLimit);
  Serial.printf("  Always Seven:       %s\n", config.alwaysSeven ? "Yes" : "No");
  Serial.printf("  Random Switch:      %d\n", config.randomSwitchPoint);
  Serial.printf("  Tumble Constant:    %.2f\n", config.tumbleConstant);
  Serial.printf("  Sleep Timeout:      %lu ms (%.1f min)\n", 
                config.deepSleepTimeout, 
                config.deepSleepTimeout / 60000.0);
  
  Serial.println("========================================");
}

bool readEEPROMConfig(DiceConfig& config) {
  EEPROM.get(EEPROM_CONFIG_ADDRESS, config);
  return validateConfig(config);
}

void writeEEPROMConfig(const DiceConfig& config) {
  Serial.println("\nWriting configuration to EEPROM...");
  
  // Write byte-by-byte for reliability
  const uint8_t* data = (const uint8_t*)&config;
  for (size_t i = 0; i < sizeof(DiceConfig); i++) {
    EEPROM.write(EEPROM_CONFIG_ADDRESS + i, data[i]);
  }
  
  // Commit to flash
  if (EEPROM.commit()) {
    Serial.println("✓ EEPROM commit successful");
  } else {
    Serial.println("✗ EEPROM commit failed!");
  }
  
  delay(100);  // Ensure write completes
}