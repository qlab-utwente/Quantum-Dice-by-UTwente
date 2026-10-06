#include "Battery.hpp"

#include "Arduino.h"
#include "Adafruit_MAX1704X.h"
#include "freertos/atomic.h"

const char *BatteryClass::TASK_NAME = "batteryTask";

/**
 * Maps a floating point number input from an input range to an output range with optional clamping.
 *
 * @param input The value to map from the input range to the output range.
 * @param inputMin The minimum value of the input range.
 * @param inputMax The maximum value of the input range.
 * @param outputMin The minimum value of the output range.
 * @param outputMax The maximum value of the output range.
 * @param clamp Whether to apply clamping on the output value.
 * @return The input value mapped from the input range to the output range.
 */
static float mapFloat(const float input, const float inputMin, const float inputMax, const float outputMin, const float outputMax, const bool clamp) noexcept {
	const float input01 = (input - inputMin) / (inputMax - inputMin);
	const float output = (input01 * (outputMax - outputMin)) + outputMin;
	return clamp ? constrain(output, outputMin, outputMax) : output;
}

/**
 * Reads the voltage on the battery voltage pin.
 *
 * @return The voltage on the battery voltage pin.
 */
static float readVoltage() noexcept {
	constexpr float FACTOR = BatteryClass::VOLTAGE_DIVIDER_FACTOR / 1000.0F;
	return static_cast<float>(analogReadMilliVolts(BatteryClass::VOLTAGE_PIN)) * FACTOR;
}

void BatteryClass::begin() noexcept {
	pinMode(BatteryClass::VOLTAGE_PIN, ANALOG);

	// If we do not know which battery monitor is present, then we have not yet started the battery
	// monitor and we must first detect which it is and if necessary, start it.
	if (this->monitor == BatteryMonitor::UNKNOWN) {
		uint32_t correctAnalogReadings = 0;
		while (true) {
			if (this->adafruitMAX17048.begin()) {
				this->monitor = BatteryMonitor::MAX17048;
				break;
			}

			if (readVoltage() >= BatteryClass::VOLTAGE_MINIMUM) {
				if (++correctAnalogReadings >= VOLTAGE_MINIMUM_READINGS) {
					this->monitor = BatteryMonitor::ADC;
					break;
				}
			}
		}
	}

	// Start the task to regularly measure/determine the voltage and state of charge if it has not
	// been started yet.
	if (!this->taskHandle) {
		this->taskHandle = xTaskCreateStaticPinnedToCore(
			BatteryClass::taskFunction,
			BatteryClass::TASK_NAME,
			BatteryClass::TASK_STACK_SIZE,
			nullptr,
			BatteryClass::TASK_PRIORITY,
			this->taskStack,
			&this->taskBuffer,
			BatteryClass::TASK_CORE
		);
	}
}

BatteryClass &BatteryClass::getSingleton() noexcept {
	static BatteryClass singleton;
	return singleton;
}

void BatteryClass::taskFunction(void *parameter) noexcept {
	static TickType_t lastWake = xTaskGetTickCount();
	BatteryClass &singleton = BatteryClass::getSingleton();

	while (true) {
		// We will first read the voltage and state of charge into these two variables. When we have
		// both values ready, we can atomically update the values in the singleton to match.
		float voltage = NAN, stateOfCharge = NAN;
		switch (singleton.monitor) {
			case BatteryMonitor::ADC:
				voltage = readVoltage();
				stateOfCharge = mapFloat(voltage, BatteryClass::VOLTAGE_SOC0, BatteryClass::VOLTAGE_SOC100, 0.0F, 100.0F, true);
				break;

			case BatteryMonitor::MAX17048:
				voltage = singleton.adafruitMAX17048.cellVoltage();
				stateOfCharge = singleton.adafruitMAX17048.cellPercent();
				stateOfCharge = mapFloat(stateOfCharge, BatteryClass::MAX17048_FALSE_SOC0, 100.0F, 0.0F, 100.0F, true);
				break;
		}

		// Atomically update the values in the singleton using the values we just
		// measured/determined.
		ATOMIC_ENTER_CRITICAL();
		{
			singleton.voltage = voltage;
			singleton.stateOfCharge = stateOfCharge;
		}
		ATOMIC_EXIT_CRITICAL();

		// Sleep until next update interval.
		xTaskDelayUntil(&lastWake, BatteryClass::TASK_UPDATE_INTERVAL);
	}
}

const char *toString(const BatteryMonitor monitor) noexcept {
	switch (monitor) {
		case BatteryMonitor::UNKNOWN: return "UNKNOWN";
		case BatteryMonitor::ADC: return "ADC";
		case BatteryMonitor::MAX17048: return "MAX17048";
	}
}
