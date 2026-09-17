#include "Battery.hpp"
#include "Arduino.h"
#include "Adafruit_MAX1704X.h"

const char *BatteryClass::TASK_NAME = "batteryTask";

void BatteryClass::begin() {
	Chip chipPresent = this->detectChip();

	switch (chipPresent) {
		case Chip::ADC: this->startAnalog(); break;
		case Chip::MAX17048: this->startMAX17048(); break;
	}
}

BatteryClass &BatteryClass::getSingleton() {
	static BatteryClass singleton;
	return singleton;
}

BatteryClass::Chip BatteryClass::detectChip() {
	uint32_t correctReadings = 0;

	// Loop until either we have a valid reading on the ADC, or if the MAX17048 chip is detected.
	while (true) {
		if (this->adafruitMAX17048.begin()) {
			return Chip::MAX17048;
		}

		if (BatteryClass::readVoltage() >= BatteryClass::VOLTAGE_ABS_MINIMUM) {
			correctReadings++;

			if (correctReadings >= VOLTAGE_MINIMUM_READINGS) {
				return Chip::ADC;
			}
		}
	}
}

void BatteryClass::startAnalog() {
	// Preparing the ADC has already been done.
	// We only have to start the task that periodically updates the variables.
	this->lastWake = xTaskGetTickCount();
	this->taskHandle = xTaskCreateStaticPinnedToCore(
		BatteryClass::taskAnalog,
		BatteryClass::TASK_NAME,
		BatteryClass::TASK_STACK_SIZE,
		nullptr,
		BatteryClass::TASK_PRIORITY,
		this->taskStack,
		&this->taskBuffer,
		BatteryClass::TASK_CORE
	);
}

void BatteryClass::startMAX17048() {
	// Now we start the task that will periodically read from the MAX17048 to update the variables.
	this->lastWake = xTaskGetTickCount();
	this->taskHandle = xTaskCreateStaticPinnedToCore(
		BatteryClass::taskMAX17048,
		BatteryClass::TASK_NAME,
		BatteryClass::TASK_STACK_SIZE,
		nullptr,
		BatteryClass::TASK_PRIORITY,
		this->taskStack,
		&this->taskBuffer,
		BatteryClass::TASK_CORE
	);
}

void BatteryClass::taskAnalog(void *parameter) {
	while (true) {
		// Update the voltage and state of charge.
		BatteryClass &singleton = BatteryClass::getSingleton();
		singleton.voltage = BatteryClass::readVoltage();
		singleton.stateOfCharge = calculateStateOfCharge(singleton.voltage);

		// Sleep until it is time again.
		xTaskDelayUntil(&singleton.lastWake, BatteryClass::TASK_UPDATE_INTERVAL);
	}
}

void BatteryClass::taskMAX17048(void *parameter) {
	while (true) {
		// Update the voltage, state of charge, and charge/discharge rate.
		BatteryClass &singleton = BatteryClass::getSingleton();
		singleton.voltage = singleton.adafruitMAX17048.cellVoltage();
		float rawStateOfCharge = singleton.adafruitMAX17048.cellPercent();
		singleton.stateOfCharge = BatteryClass::adjustStateOfCharge(rawStateOfCharge);
		singleton.chargeRate = singleton.adafruitMAX17048.chargeRate();

		// Sleep until it is time again.
		xTaskDelayUntil(&singleton.lastWake, BatteryClass::TASK_UPDATE_INTERVAL);
	}
}

float BatteryClass::readVoltage() {
	pinMode(BatteryClass::VOLTAGE_MONITOR_PIN, ANALOG);
	float milliVolts = static_cast<float>(analogReadMilliVolts(BatteryClass::VOLTAGE_MONITOR_PIN)) / 1000.0f;
	return milliVolts * BatteryClass::VOLTAGE_DIVIDER_FACTOR;
}

float BatteryClass::calculateStateOfCharge(float voltage) {
	constexpr float FACTOR = 100.0f / (BatteryClass::VOLTAGE_MAXIMUM - BatteryClass::VOLTAGE_MINIMUM);
	float stateOfCharge = (voltage - BatteryClass::VOLTAGE_MINIMUM) * FACTOR;
	return std::clamp(stateOfCharge, 0.0f, 100.0f);
}

float BatteryClass::adjustStateOfCharge(float stateOfCharge) {
	constexpr float FACTOR = (100.0f / (100.0f - BatteryClass::FAKE_MINIMUM));
	float adjustedStateOfCharge = (stateOfCharge - BatteryClass::FAKE_MINIMUM) * FACTOR;
	return std::clamp(adjustedStateOfCharge, 0.0f, 100.0f);
}
