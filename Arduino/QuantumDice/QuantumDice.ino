
//version control see Version.h
#warning "Compile with Pin Numbering By GPIO (legacy)"
#warning "ESP version 3.3.2 ,board esp32/Arduino Nano ESP32 or esp32/ESP32S3 Dev Module

#include "Version.h"
#include "diceConfig.h"
#include "defines.h"
#include "ImageLibrary/ImageLibrary.h"
#include "ScreenStateDefs.h"
#include "IMUhelpers.h"
#include "Screenfunctions.h"
#include "handyHelpers.h"
#include "StateMachine.h"

StateMachine stateMachine;

#define UPDATE_INTERVAL 50  //loop functions
unsigned long previousMillisWatchDog = 0;

void setup() {
  // Make sure power switch keeps on - do this FIRST before anything else
  pinMode(REGULATOR_PIN, OUTPUT);
  digitalWrite(REGULATOR_PIN, LOW);
  
  // Initialize serial for debugging
  initSerial(); // delay(1000) included
  
  // Initialize EEPROM ONCE for both config and IMU calibration
  initEEPROM();
  
  // Load dice configuration from EEPROM
  // This MUST be done early because it initializes hwPins which are needed for displays
  if (!loadConfigFromEEPROM()) {
    Serial.println("FATAL ERROR: Cannot load configuration!");
    Serial.println("Please flash configuration using the setup sketch.");
    // Halt execution - can't proceed without valid configuration
    while(1) {
      delay(1000);
    }
  }
  
  // Print version and configuration info
  Serial.println("\n" __FILE__ " " __DATE__ " " __TIME__);
  Serial.print("Version: ");
  Serial.print(VERSION);
  Serial.print(" - Dice ID: ");
  Serial.println(currentConfig.diceId);  // Use diceId from config
  Serial.print("Board type: ");
  Serial.println(currentConfig.isNano ? "NANO" : "DEVKIT");  // Use config instead of defines
  
  // Initialize displays - now uses hwPins from loaded configuration
  initDisplays();
  
  // Show startup logo during setup
  displayQLab(ALL);
  
  // Initialize IMU sensor
  IMUSensor *imuSensor;
  imuSensor = new BNO055IMUSensor();
  imuSensor->init();  // This will load BNO055 calibration from EEPROM
  imuSensor->update();
  imuSensor->reset();
  
  // Set IMU sensor in state machine
  stateMachine.setImuSensor(imuSensor);
  
  // Initialize button
  initButton();
  
  // Initialize random number generators
  initRandomGenerators();
  
  // Initialize the state machine - this sets up ESP-NOW with config MACs
  stateMachine.begin();
  
  Serial.println("Setup complete!");
  Serial.println("==================================\n");
}

void loop() {
  static unsigned long lastUpdateTime = 0;
  unsigned long currentTime = millis();
  if (currentTime - lastUpdateTime >= UPDATE_INTERVAL) {
    button.loop();
    stateMachine.update();
    //   refreshScreens();
    //   sendWatchDog(); //sendWatchDog removed. Is called at every onEntry function after the states are set. More efficient.
  }
}
