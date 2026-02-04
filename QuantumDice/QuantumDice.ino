#warning "Compile with Pin Numbering By GPIO (legacy)"
#warning "ESP version 3.3.2 ,board esp32/Arduino Nano ESP32 or esp32/ESP32S3 Dev Module

#include "defines.hpp"
#include "ScreenStateDefs.hpp"
#include "IMUhelpers.hpp"
#include "Screenfunctions.hpp"
#include "handyHelpers.hpp"
#include "StateMachine.hpp"
#include "DiceConfigManager.hpp"

constexpr uint16_t UPDATE_INTERVAL = 50;  //loop functions
constexpr uint8_t SECOND = 1000;

StateMachine stateMachine;

void setup() {
    // Make sure power switch keeps on - do this FIRST before anything else
    pinMode(REGULATOR_PIN, OUTPUT);
    digitalWrite(REGULATOR_PIN, LOW);

    // Initialize serial for debugging
    initSerial();  // delay(1000) included

    // Print version and configuration info
    infoln("╔════════════════════════════════════════╗");
    infoln("║      QUANTUM DICE INITIALIZATION       ║");
    infoln("╚════════════════════════════════════════╝\n");
    infoln(__FILE__ " " __DATE__ " " __TIME__);
    infof("FW: %s\n", VERSION);

    // ═══════════════════════════════════════════════════════════════════
    // Step 1: Initialize LittleFS and ensure config file exists
    // ═══════════════════════════════════════════════════════════════════
    infoln("Step 1: Initializing filesystem and configuration...\n");
    
    if (!ensureLittleFSAndConfig()) {
        errorln("✗ CRITICAL: Failed to initialize filesystem or config!");
        errorln("Device cannot operate. Check serial output above.");
        while(true) { 
            delay(SECOND);  // Halt
        }
    }
    
    infoln("✓ Filesystem and configuration ready!\n");
    
    // Print loaded configuration
    printGlobalConfig();

    // ═══════════════════════════════════════════════════════════════════
    // Step 2: Initialize hardware
    // ═══════════════════════════════════════════════════════════════════
    infoln("Step 2: Initializing hardware...\n");
    
    // intitialise the hardware pins and display addresses
    initHardwarePins();

    infof(" - Dice ID: %s\n", (char*) currentConfig.diceId.c_str());  // Use diceId from config
    infof("Board type: %s\n", currentConfig.isNano ? "NANO" : "DEVKIT");  // Use config instead of defines
    infof("Connection type isSMD: %s\n\n", currentConfig.isSMD ? "SMD" : "HDR");  // Use config instead of defines

    // Initialize displays - now uses hwPins from loaded configuration
    initDisplays();

    // Show startup logo during setup
    displayQLab(ALL);

    // ═══════════════════════════════════════════════════════════════════
    // Step 3: Initialize IMU sensor
    // ═══════════════════════════════════════════════════════════════════
    infoln("Step 3: Initializing IMU sensor...\n");

    // Initialize IMU sensor
    IMUSensor* imuSensor = new BNO055IMUSensor();
    if (!imuSensor->init()) {  // Show initialization progress
        warnln("Failed to initialize sensor!");
    }
    if (currentConfig.isNano) {
        constexpr uint8_t NANO_AXIS_REMAP_CONFIG = 0x06;
        imuSensor->setAxisRemap(NANO_AXIS_REMAP_CONFIG, NANO_AXIS_REMAP_CONFIG);
    }

    imuSensor->update();
    imuSensor->resetTumbleDetection();

    usleep(SECOND);

    welcomeInfo(screenselections::X0);
    voltageIndicator(screenselections::X0);
    displayQRcode(screenselections::X1);

    displayEinstein(screenselections::ZZ);
    displayUTlogo(screenselections::YY);

    usleep(SECOND);
    
    // ═══════════════════════════════════════════════════════════════════
    // Step 4: Complete initialization
    // ═══════════════════════════════════════════════════════════════════
    infoln("Step 4: Completing initialization...\n");

    // Set IMU sensor in state machine
    stateMachine.setImuSensor(imuSensor);

    // Initialize button
    initButton();

    // Initialize the state machine - this sets up ESP-NOW with config MACs
    stateMachine.begin();

    infoln("╔════════════════════════════════════════╗");
    infoln("║       SETUP COMPLETE - READY!          ║");
    infoln("╚════════════════════════════════════════╝\n");
}

void loop() {
    static unsigned long lastUpdateTime = 0;
    unsigned long currentTime = millis();
    if (currentTime - lastUpdateTime >= UPDATE_INTERVAL) {
        button.loop();
        stateMachine.update();
    }
}