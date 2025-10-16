/**
 * EEPROM Configuration Writer/Reader for Quantum Dice
 * 
 * This sketch:
 * 1. Checks if valid configuration exists in EEPROM
 * 2. If valid, reads and displays the configuration
 * 3. If not valid, writes default configuration from diceConfig.h
 * 
 * Upload this sketch first to initialize EEPROM, then upload main sketch
 */

#include <EEPROM.h>
#include <Adafruit_GC9A01A.h>  // For color constants

// EEPROM Memory Layout (must match handyHelpers.h)
#define EEPROM_SIZE 512
#define EEPROM_BNO_SENSOR_ID_ADDR 0
#define EEPROM_BNO_CALIBRATION_ADDR 4
#define EEPROM_CONFIG_ADDRESS 24

// Configuration structure (must match handyHelpers.h)
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

// Default configuration from diceConfig.h DICE_SET_S000
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
  .isSMD = true,      // SMD configuration
  .isNano = false,    // DEVKIT configuration
  .alwaysSeven = false,
  .randomSwitchPoint = 50,
  .tumbleConstant = 0.2,
  .deepSleepTimeout = 5 * 60 * 1000  // 5 minutes
};

// Function prototypes
bool validateConfig(const DiceConfig& config);
void printConfig(const DiceConfig& config, const char* title);
void writeDefaultConfig();
bool readAndPrintConfig();

void setup() {
  Serial.begin(115200);
  delay(2000);  // Wait for Serial Monitor
  
  Serial.println("\n\n========================================");
  Serial.println("Quantum Dice EEPROM Configuration Tool");
  Serial.println("========================================\n");
  
  // Initialize EEPROM
  if (!EEPROM.begin(EEPROM_SIZE)) {
    Serial.println("ERROR: Failed to initialize EEPROM!");
    return;
  }
  
  Serial.println("EEPROM initialized successfully");
  Serial.printf("EEPROM size: %d bytes\n", EEPROM_SIZE);
  Serial.printf("Configuration size: %d bytes\n", sizeof(DiceConfig));
  Serial.printf("Configuration address: 0x%04X\n\n", EEPROM_CONFIG_ADDRESS);
  
  // Check if valid configuration exists
  Serial.println("Checking for existing configuration...\n");
  
  if (readAndPrintConfig()) {
    Serial.println("\n✓ Valid configuration found in EEPROM!");
    Serial.println("\nOptions:");
    Serial.println("  Type 'w' to overwrite with default configuration");
    Serial.println("  Type 'r' to re-read and display configuration");
    Serial.println("  Or close Serial Monitor if done");
  } else {
    Serial.println("\n✗ No valid configuration found in EEPROM");
    Serial.println("\nWriting default configuration...\n");
    writeDefaultConfig();
    Serial.println("\n✓ Default configuration written successfully!");
    Serial.println("\nVerifying written configuration...\n");
    readAndPrintConfig();
  }
}

void loop() {
  // Check for serial commands
  if (Serial.available() > 0) {
    char cmd = Serial.read();
    
    if (cmd == 'w' || cmd == 'W') {
      Serial.println("\n\nOverwriting with default configuration...\n");
      writeDefaultConfig();
      Serial.println("✓ Configuration updated!\n");
      Serial.println("Reading back configuration...\n");
      readAndPrintConfig();
      
    } else if (cmd == 'r' || cmd == 'R') {
      Serial.println("\n\nRe-reading configuration...\n");
      readAndPrintConfig();
    }
  }
  
  delay(100);
}

/**
 * Validate configuration
 */
bool validateConfig(const DiceConfig& config) {
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
  
  return true;
}

/**
 * Print configuration details
 */
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
  
  Serial.println("========================================\n");
}

/**
 * Write default configuration to EEPROM
 */
void writeDefaultConfig() {
  DiceConfig config = defaultConfig;
  
  Serial.println("Writing configuration to EEPROM...");
  printConfig(config, "Configuration to Write");
  
  // Write to EEPROM byte-by-byte for reliability
  const uint8_t* data = (const uint8_t*)&config;
  for (size_t i = 0; i < sizeof(DiceConfig); i++) {
    EEPROM.write(EEPROM_CONFIG_ADDRESS + i, data[i]);
  }
  
  // Commit to flash
  if (EEPROM.commit()) {
    Serial.println("✓ EEPROM commit successful");
  } else {
    Serial.println("✗ EEPROM commit failed!");
    return;
  }
  
  // Small delay to ensure write completes
  delay(100);
  
  Serial.println("✓ Configuration written to EEPROM");
}

/**
 * Read configuration from EEPROM and print it
 * Returns true if valid configuration found
 */
bool readAndPrintConfig() {
  DiceConfig config;
  
  // Read from EEPROM
  EEPROM.get(EEPROM_CONFIG_ADDRESS, config);
  
  Serial.println("Validating configuration...");
  
  // Validate
  if (!validateConfig(config)) {
    Serial.println("✗ Configuration validation failed");
    return false;
  }
  
  Serial.println("✓ Configuration validation passed\n");
  printConfig(config, "Current EEPROM Configuration");
  
  return true;
}