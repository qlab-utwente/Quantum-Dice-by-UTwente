#include "Arduino.h"
#include "defines.h"
#include "IMUhelpers.h"
#include "handyHelpers.h"

// Define global configuration object
DiceConfig currentConfig;
HardwarePins hwPins;

// Existing global variables
ATECCX08A atecc;
RTC_DATA_ATTR int bootCount = 0;
bool randomChipPresent = false;
Button2 button;
bool clicked = false;
bool longclicked = false;

/**
 * Initialize hardware pins based on configuration
 * Sets up pin assignments for NANO vs DEVKIT and SMD vs HDR
 */
void initHardwarePins() {
  Serial.println("Initializing hardware pins...");
  
  // Set TFT pins based on NANO vs DEVKIT
  if (currentConfig.isNano) {
    hwPins.tft_cs = 21;
    hwPins.tft_rst = 4;
    hwPins.tft_dc = 2;
    hwPins.adc_pin = 1;
    
    // Screen CS pins for NANO
    hwPins.screen_cs[0] = 5;
    hwPins.screen_cs[1] = 6;
    hwPins.screen_cs[2] = 7;
    hwPins.screen_cs[3] = 8;
    hwPins.screen_cs[4] = 9;
    hwPins.screen_cs[5] = 10;
  } else {
    // DEVKIT
    hwPins.tft_cs = 10;
    hwPins.tft_rst = 48;
    hwPins.tft_dc = 47;
    hwPins.adc_pin = 2;
    
    // Screen CS pins for DEVKIT
    hwPins.screen_cs[0] = 4;
    hwPins.screen_cs[1] = 5;
    hwPins.screen_cs[2] = 6;
    hwPins.screen_cs[3] = 7;
    hwPins.screen_cs[4] = 15;
    hwPins.screen_cs[5] = 16;
  }
  
  // Set screen address mapping based on SMD vs HDR
  if (currentConfig.isSMD) {
    // SMD screen addresses
    uint8_t smdAddresses[16] = {
      // singles
      0b00000100,  // x0
      0b00010000,  // x1
      0b00001000,  // y0
      0b00000010,  // y1
      0b00100000,  // z0
      0b00000001,  // z1
      // doubles
      0b00010100,  // xx
      0b00001010,  // yy
      0b00100001,  // zz
      // quarters
      0b00011110,
      0b00101011,
      0b00110101,
      // triples + / -
      0b00101100,  // x0y0z0
      0b00010011,  // x1y1z1
      // others
      0b00111111,
      0b00000000
    };
    memcpy(hwPins.screenAddress, smdAddresses, 16);
  } else {
    // HDR screen addresses
    uint8_t hdrAddresses[16] = {
      // singles
      0b00001000,  // x0
      0b00000010,  // x1
      0b00000100,  // y0
      0b00010000,  // y1
      0b00100000,  // z0
      0b00000001,  // z1
      // doubles
      0b00001010,  // xx
      0b00010100,  // yy
      0b00100001,  // zz
      // quarters
      0b00011110,
      0b00101011,
      0b00110101,
      // triples + / -
      0b00101100,  // x0y0z0
      0b00010011,  // x1y1z1
      // others
      0b00111111,
      0b00000000
    };
    memcpy(hwPins.screenAddress, hdrAddresses, 16);
  }
  
  Serial.println("Hardware pins initialized successfully!");
  printHardwarePins();
}

/**
 * Print hardware pin configuration for debugging
 */
void printHardwarePins() {
  Serial.println("\n=== Hardware Pin Configuration ===");
  Serial.printf("Board Type: %s\n", currentConfig.isNano ? "NANO" : "DEVKIT");
  Serial.printf("Screen Type: %s\n", currentConfig.isSMD ? "SMD" : "HDR");
  Serial.println("\nTFT Display Pins:");
  Serial.printf("  CS:  GPIO%d\n", hwPins.tft_cs);
  Serial.printf("  RST: GPIO%d\n", hwPins.tft_rst);
  Serial.printf("  DC:  GPIO%d\n", hwPins.tft_dc);
  Serial.println("\nScreen CS Pins:");
  for (int i = 0; i < 6; i++) {
    Serial.printf("  Screen %d: GPIO%d\n", i, hwPins.screen_cs[i]);
  }
  Serial.printf("\nADC Pin: GPIO%d\n", hwPins.adc_pin);
  Serial.println("==================================\n");
}

/**
 * Initialize EEPROM with proper size
 * Note: This is called before IMU init, so both systems share the same EEPROM
 */
void initEEPROM() {
  if (!EEPROM.begin(EEPROM_SIZE)) {
    Serial.println("ERROR: Failed to initialize EEPROM");
  } else {
    Serial.println("EEPROM initialized successfully");
    Serial.printf("EEPROM size: %d bytes\n", EEPROM_SIZE);
    Serial.printf("Configuration size: %d bytes\n", sizeof(DiceConfig));
    printEEPROMMemoryMap();
  }
}

/**
 * Print EEPROM memory layout for debugging
 */
void printEEPROMMemoryMap() {
  Serial.println("\n=== EEPROM Memory Map ===");
  Serial.printf("BNO055 Sensor ID:    0x%04X - 0x%04X (%d bytes)\n", 
                EEPROM_BNO_SENSOR_ID_ADDR, 
                EEPROM_BNO_SENSOR_ID_ADDR + sizeof(long) - 1,
                sizeof(long));
  Serial.printf("BNO055 Calibration:  0x%04X - 0x%04X (%d bytes)\n", 
                EEPROM_BNO_CALIBRATION_ADDR, 
                EEPROM_BNO_CALIBRATION_ADDR + sizeof(adafruit_bno055_offsets_t) - 1,
                sizeof(adafruit_bno055_offsets_t));
  Serial.printf("Dice Configuration:  0x%04X - 0x%04X (%d bytes)\n", 
                EEPROM_CONFIG_ADDRESS, 
                EEPROM_CONFIG_ADDRESS + sizeof(DiceConfig) - 1,
                sizeof(DiceConfig));
  Serial.printf("Total used:          %d bytes\n", 
                EEPROM_CONFIG_ADDRESS + sizeof(DiceConfig));
  Serial.printf("Free space:          %d bytes\n", 
                EEPROM_SIZE - (EEPROM_CONFIG_ADDRESS + sizeof(DiceConfig)));
  Serial.println("========================\n");
}

// /**
//  * Calculate simple checksum for configuration validation
//  */
// uint8_t calculateChecksum(const DiceConfig& config) {
//   uint8_t checksum = 0;
//   const uint8_t* data = (const uint8_t*)&config;
//   // Calculate checksum over all bytes except the checksum field itself
//   size_t dataSize = sizeof(DiceConfig) - sizeof(config.checksum);
  
//   for (size_t i = 0; i < dataSize; i++) {
//     checksum ^= data[i];
//   }
  
//   return checksum;
// }

/**
 * Validate configuration data
 */
bool validateConfig(const DiceConfig& config) {
  // Check if diceId is null-terminated and contains printable characters
  bool validId = false;
  for (int i = 0; i < 16; i++) {
    if (config.diceId[i] == '\0') {
      validId = (i > 0);  // At least one character before null
      break;
    }
    if (!isprint(config.diceId[i])) {
      Serial.println("ERROR: Invalid diceId characters");
      return false;
    }
  }
  
  if (!validId) {
    Serial.println("ERROR: Invalid diceId format");
    return false;
  }
  
  // // Validate checksum
  // uint8_t calculatedChecksum = calculateChecksum(config);
  // if (calculatedChecksum != config.checksum) {
  //   Serial.printf("ERROR: Checksum mismatch (expected 0x%02X, got 0x%02X)\n", 
  //                 config.checksum, calculatedChecksum);
  //   return false;
  // }
  
  // Validate RSSI limit is in reasonable range
  if (config.rssiLimit > 0 || config.rssiLimit < -100) {
    Serial.printf("ERROR: Invalid RSSI limit: %d\n", config.rssiLimit);
    return false;
  }
  
  // Validate randomSwitchPoint (should be 0-100)
  if (config.randomSwitchPoint > 100) {
    Serial.printf("ERROR: Invalid randomSwitchPoint: %d\n", config.randomSwitchPoint);
    return false;
  }
  
  // Validate tumbleConstant (should be positive and reasonable)
  if (config.tumbleConstant <= 0 || config.tumbleConstant > 10.0) {
    Serial.printf("ERROR: Invalid tumbleConstant: %.2f\n", config.tumbleConstant);
    return false;
  }
  
  // Validate deepSleepTimeout (should be reasonable, e.g., 10 sec to 1 hour)
  if (config.deepSleepTimeout < 10000 || config.deepSleepTimeout > 3600000) {
    Serial.printf("ERROR: Invalid deepSleepTimeout: %lu ms\n", config.deepSleepTimeout);
    return false;
  }
  
  return true;
}

/**
 * Load configuration from EEPROM
 * Returns true if successful, false otherwise
 */
bool loadConfigFromEEPROM() {
  Serial.println("Loading configuration from EEPROM...");
  
  // Read configuration from EEPROM
  EEPROM.get(EEPROM_CONFIG_ADDRESS, currentConfig);
  
  // Validate the loaded configuration
  if (!validateConfig(currentConfig)) {
    Serial.println("ERROR: Invalid configuration in EEPROM");
    return false;
  }
  
  Serial.println("Configuration loaded successfully!");
  printConfig(currentConfig);
  
  // Initialize hardware pins based on loaded configuration
  initHardwarePins();
  
  return true;
}

/**
 * Print configuration for debugging
 */
void printConfig(const DiceConfig& config) {
  Serial.println("\n=== Dice Configuration ===");
  Serial.print("Dice ID: ");
  Serial.println(config.diceId);
  
  Serial.print("Device A MAC: ");
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
  
  Serial.printf("Background Colors:\n");
  Serial.printf("  X: 0x%04X\n", config.x_background);
  Serial.printf("  Y: 0x%04X\n", config.y_background);
  Serial.printf("  Z: 0x%04X\n", config.z_background);
  Serial.printf("Entanglement Colors:\n");
  Serial.printf("  AB1: 0x%04X\n", config.entang_ab1_color);
  Serial.printf("  AB2: 0x%04X\n", config.entang_ab2_color);
  
  Serial.printf("RSSI Limit: %d dBm\n", config.rssiLimit);
  Serial.printf("Hardware: %s, %s\n", 
                config.isSMD ? "SMD" : "HDR",
                config.isNano ? "NANO" : "DEVKIT");
  Serial.printf("Always Seven: %s\n", config.alwaysSeven ? "Yes" : "No");
  
  Serial.printf("\nTiming Constants:\n");
  Serial.printf("  Random Switch Point: %d\n", config.randomSwitchPoint);
  Serial.printf("  Tumble Constant: %.2f\n", config.tumbleConstant);
  Serial.printf("  Deep Sleep Timeout: %lu ms (%.1f minutes)\n", 
                config.deepSleepTimeout, 
                config.deepSleepTimeout / 60000.0);
  
  Serial.printf("Checksum: 0x%02X\n", config.checksum);
  Serial.println("==========================\n");
}

// ... (rest of existing functions remain the same)

void checkTimeForDeepSleep(IMUSensor *imuSensor) {
  static bool isMoving = false;
  static unsigned long lastMovementTime = 0;

  if (imuSensor->isNotMoving()) {
    if (isMoving) {
      lastMovementTime = millis();
      isMoving = false;
    }
  } else {
    isMoving = true;
  }

  // Use the timeout from configuration
  if (!isMoving && (millis() - lastMovementTime > currentConfig.deepSleepTimeout)) {
    lastMovementTime = millis();  // Reset the timer
    debugln("Time to sleep");
    digitalWrite(REGULATOR_PIN, HIGH);
  }
}

void initButton() {
  button.begin(BUTTON_PIN, INPUT, false);
  button.setLongClickDetectedHandler(longClickDetected);
  button.setLongClickTime(1000);
  button.setClickHandler(click);
}

void longClickDetected(Button2& btn) {
  debugln("long pressed");
  longclicked = true;
}

void click(Button2& btn) {
  debugln("short pressed");
  clicked = true;
}

void initRandomGenerators() {
  //create pseudo random numbers based on analogRead value
  randomSeed(analogRead(A0));
  randomChipPresent = atecc.begin();
}

uint8_t generateDiceRoll() {
  // Get a random 32-bit integer from the crypto chip
  uint32_t randomNumber = atecc.getRandomInt();

  // Check if we got a valid random number (0 indicates error)
  if (randomNumber == 0) {
    Serial.println("ERROR: Failed to get random number");
    return 1;  // Default value in case of error
  }

  // Map to 1-6 range using modulo
  return (randomNumber % 6) + 1;
}

uint8_t generateDiceRollRejection() {
  uint8_t randomByte;

  do {
    // Get a random byte from the crypto chip
    randomByte = atecc.getRandomByte();

    // Check for error (getRandomByte might return 0 on error)
    if (randomByte == 0) {
      Serial.println("ERROR: Failed to get random byte");
      return 1;
    }
  } while (randomByte >= 252);  // 252 = 6 * 42, ensures uniform distribution

  return (randomByte % 6) + 1;
}

bool checkMinimumVoltage() {
  float voltage = analogReadMilliVolts(hwPins.adc_pin) / 1000.0 * 2.0;  //ADC measures 50% of battery voltage by 50/50 voltage divider
  //debugln(voltage);
  if (voltage < MINBATERYVOLTAGE && voltage > 0.5) //while on USB the voltage is 0
    return true;
  else
    return false;
}

float mapFloat(float x, float in_min, float in_max, float out_min, float out_max, bool clipOutput) {
  float mappedValue = (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;

  // Apply clipping if clipOutput is true
  if (clipOutput) {
    mappedValue = max(out_min, min(mappedValue, out_max));
  }

  return mappedValue;
}

bool withinBounds(float val, float minimum, float maximum) {
  return ((minimum <= val) && (val <= maximum));
}

void initSerial() {
  Serial.begin(115200);
  delay(1000);
}