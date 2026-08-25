#warning "Compile with Pin Numbering By GPIO (legacy)"
#warning "ESP version 3.3.2 ,board esp32/Arduino Nano ESP32 or esp32/ESP32S3 Dev Module

#include <driver/gpio.h>
#include <driver/rtc_io.h>
#include <esp_sleep.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

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

constexpr TickType_t TICK_INTERVAL = pdMS_TO_TICKS(50);
static TickType_t lastWake;

void batteryIsrRising() {
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_13, 0);
}

void batteryIsrFalling() {
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_13, 1);
}

void setup() {
    // Make sure power switch keeps on - do this FIRST before anything else
    pinMode(REGULATOR_PIN, OUTPUT);
    digitalWrite(REGULATOR_PIN, LOW);
    pinMode(GPIO_NUM_10, OUTPUT);
    digitalWrite(GPIO_NUM_10, HIGH);
    pinMode(GPIO_NUM_3, OUTPUT);
    digitalWrite(GPIO_NUM_3, HIGH);

    // Initialize serial for debugging
    initSerial();  // delay(1000) included
    initBattery();

    uint32_t level = gpio_get_level(GPIO_NUM_13);
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_13, level == 0 ? 1 : 0);
    gpio_set_direction(GPIO_NUM_13, GPIO_MODE_INPUT);
    rtc_gpio_pullup_dis(GPIO_NUM_13);
    rtc_gpio_pulldown_dis(GPIO_NUM_13);
    attachInterrupt(GPIO_NUM_13, batteryIsrRising, RISING);
    attachInterrupt(GPIO_NUM_13, batteryIsrFalling, FALLING);

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

    // Print the hardware pins and display addresses
    printHardwarePins();

    infof(" - Dice ID: %s\n", (char*) currentConfig.diceId.c_str());  // Use diceId from config

    // Initialize displays - now uses hwPins from loaded configuration
    initDisplays();

    // Show startup logo during setup
    displayQLab(ALL);

    // ═══════════════════════════════════════════════════════════════════
    // Step 3: Initialize IMU sensor
    // ═══════════════════════════════════════════════════════════════════
    infoln("Step 3: Initializing IMU sensor...\n");

    // Initialize IMU sensor
    IMUSensor* imuSensor = new LSM6DS3TRCIMUSensor();
    if (!imuSensor->init()) {  // Show initialization progress
        warnln("Failed to initialize sensor!");
        while (true);
    }

    imuSensor->update();
    imuSensor->resetTumbleDetection();

    welcomeInfo(screenselections::X0);
    imuSensor->update();

    voltageIndicator(screenselections::X0);
    imuSensor->update();

    displayQRcode(screenselections::X1);
    imuSensor->update();

    displayEinstein(screenselections::ZZ);
    imuSensor->update();

    displayUTlogo(screenselections::YY);
    imuSensor->update();

    for (uint8_t i = 0; i < 10; i++) {
        delay(100);
        imuSensor->update();
    }

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

    lastWake = xTaskGetTickCount();
}

void loop() {
    button.loop();
    stateMachine.update();

	vTaskDelayUntil(&lastWake, TICK_INTERVAL);
}
