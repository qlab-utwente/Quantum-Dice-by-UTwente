#ifndef HANDYHELPERS_H_
#define HANDYHELPERS_H_

#include <sys/_stdint.h>
#include <SparkFun_ATECCX08a_Arduino_Library.h>
#include <Button2.h>
#include <EEPROM.h>

// EEPROM Memory Layout
#define EEPROM_SIZE 512
#define EEPROM_BNO_SENSOR_ID_ADDR 0                    // 4 bytes for sensor ID (long)
#define EEPROM_BNO_CALIBRATION_ADDR 4                  // 20 bytes for calibration data
#define EEPROM_CONFIG_ADDRESS 24                        // Start config after calibration data

// Configuration structure to store in EEPROM
struct DiceConfig {
  char diceId[16];              // "TEST1", "BART1", etc.
  uint8_t deviceA_mac[6];       // MAC address of device A
  uint8_t deviceB1_mac[6];      // MAC address of device B1
  uint8_t deviceB2_mac[6];      // MAC address of device B2
  uint16_t x_background;        // Display background colors
  uint16_t y_background;
  uint16_t z_background;
  uint16_t entang_ab1_color;
  uint16_t entang_ab2_color;
  int8_t rssiLimit;             // RSSI limit for entanglement detection
  bool isSMD;                   // true for SMD, false for HDR
  bool isNano;                  // true for NANO, false for DEVKIT
  bool alwaysSeven;             // Force dice to always produce 7
  
  // Additional constants previously in diceConfig.h
  uint8_t randomSwitchPoint;    // Threshold for random value (0-100)
  float tumbleConstant;         // Number of tumbles to detect tumbling
  uint32_t deepSleepTimeout;    // Deep sleep timeout in milliseconds
  
  uint8_t checksum;             // Simple checksum for validation
};

// Hardware pin assignments structure
struct HardwarePins {
  // TFT Display pins
  uint8_t tft_cs;
  uint8_t tft_rst;
  uint8_t tft_dc;
  
  // Screen CS pins (6 screens)
  uint8_t screen_cs[6];
  
  // Screen address mapping for SMD/HDR
  uint8_t screenAddress[16];
  
  // ADC pin for battery monitoring
  uint8_t adc_pin;
};

// Global configuration and hardware objects
extern DiceConfig currentConfig;
extern HardwarePins hwPins;

// EEPROM functions
void initEEPROM();
bool loadConfigFromEEPROM();
uint8_t calculateChecksum(const DiceConfig& config);
bool validateConfig(const DiceConfig& config);
void printConfig(const DiceConfig& config);
void printEEPROMMemoryMap();

// Hardware initialization
void initHardwarePins();
void printHardwarePins();

// Existing declarations
extern RTC_DATA_ATTR int bootCount;
extern bool randomChipPresent;
extern Button2 button;
extern bool clicked;
extern bool longclicked;

void initButton();
void longClickDetected(Button2& btn);
void click(Button2& btn);
void checkTimeForDeepSleep(IMUSensor *imuSensor);
bool checkMinimumVoltage();
float mapFloat(float x, float in_min, float in_max, float out_min, float out_max, bool clipOutput);
bool withinBounds(float val, float minimum, float maximum);
void initSerial();
void initRandomGenerators();
uint8_t generateDiceRollRejection();
uint8_t generateDiceRoll();

#endif /* HANDYHELPERS_H_ */