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

#include "power.h"

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
	// Prepare the power pins.
	// This should be done first, as the old PCBs have a SHUTDOWN pin that needs to be set to LOW.
	power_prepare();

	// Initialize the battery.
	// Currently, the power consumption is low and smooth, which is the ideal scenario for starting
	// the battery monitoring.
	initBattery();

	// Initialize serial for debugging
	initSerial();  // delay(1000) included

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
			delay(SECOND); // Halt
		}
	}

	infoln("✓ Filesystem and configuration ready!\n");
	infof("Dice ID: %s\n", (char *)currentConfig.diceId.c_str());

	// Print loaded configuration
	printGlobalConfig();

	// ═══════════════════════════════════════════════════════════════════
	// Step 2: Initialize displays
	// ═══════════════════════════════════════════════════════════════════
	infoln("Step 2: Initializing displays...\n");

    // Initialize displays and show UTwente QLab logo.
	power_on_screens();
	initDisplays();
	displayQLab(ALL);

	// ═══════════════════════════════════════════════════════════════════
	// Step 3: Initialize IMU sensor
	// ═══════════════════════════════════════════════════════════════════
	infoln("Step 3: Initializing IMU sensor...\n");

	// Initialize IMU sensor
	IMUSensor* imuSensor = new LSM6DS3TRCIMUSensor();
	if (!imuSensor->init()) {  // Show initialization progress
		warnln("Failed to initialize sensor!");
		while (true) {
			delay(SECOND); // Halt.
		}
	}

	imuSensor->update();
	imuSensor->resetTumbleDetection();

	// Show welcome info.
	welcomeInfo(screenselections::X0);
	voltageIndicator(screenselections::X0);
	displayQRcode(screenselections::X1);
	displayEinstein(screenselections::ZZ);
	displayUTlogo(screenselections::YY);

	// ═══════════════════════════════════════════════════════════════════
	// Step 4: Complete initialization
	// ═══════════════════════════════════════════════════════════════════
	infoln("Step 4: Completing initialization...\n");

	// Initialize button
	initButton();

	// Initialize the state machine
	stateMachine.setImuSensor(imuSensor);
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
