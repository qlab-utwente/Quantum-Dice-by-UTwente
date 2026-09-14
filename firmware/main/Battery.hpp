#ifndef BATTERY_HPP
#define BATTERY_HPP

#include "Arduino.h"
#include "Adafruit_MAX1704X.h"

class BatteryClass {
public:
	// Starts the battery monitoring.
	//
	// Detects which method of battery monitoring is available, prepares that method and then starts
	// a task that will periodically read the voltage, state of charge, and charge rate (if
	// available).
	void begin();

	// Returns the voltage of the battery.
	//
	// NaN is returned if the battery monitoring has not been started or if it was unable to
	// read the voltage of the battery.
	[[nodiscard]] inline float getVoltage() __attribute__((always_inline)) {
		return this->voltage;
	}

	// Returns the state of charge of the battery.
	//
	// The state of charge is returned as a percentage, so 0% means battery empty and 100% means
	// battery fully charged.
	//
	// NaN is returned if the battery monitoring has not been started or if it was unable to
	// determine the state of charge of the battery.
	[[nodiscard]] inline float getStateOfCharge() __attribute__((always_inline)) {
		return this->stateOfCharge;
	}

	// Returns the rate at which the battery is charging.
	//
	// The charge rate is returned in state of charge percentage per hour. Positive numbers mean
	// the battery is charging.
	//
	// NaN is returned if the battery monitoring has not been started or if it was unable to
	// determine the charging rate of the battery.
	[[nodiscard]] inline float getChargeRate() __attribute__((always_inline)) {
		return this->chargeRate;
	}

	// Returns the singleton instance.
	//
	// The singleton instance will be instantiated when this function is called for the first
	// time.
	[[nodiscard]] static BatteryClass &getSingleton();

	// Here are the constructors and the assignment operator.
	//
	// First is a constructor that takes no arguments, which is set to default, it will initialize
	// all variables to their default state.
	//
	// The constructor that takes a const reference and the assignment operator are both deleted, as
	// this class is a singleton class.
	BatteryClass() = default;
	BatteryClass(const BatteryClass &) = delete;
	void operator=(const BatteryClass &) = delete;

private:
	// MAX17048 constants.
	static constexpr float FAKE_MINIMUM = 10.0f;

	// ADC voltage monitoring constants.
	static constexpr size_t VOLTAGE_MONITOR_PIN = 2;
	static constexpr float VOLTAGE_DIVIDER_FACTOR = 2.0f;
	static constexpr float VOLTAGE_MINIMUM = 3.7f;
	static constexpr float VOLTAGE_MAXIMUM = 4.0f;
	static constexpr float VOLTAGE_ABS_MINIMUM = 3.0f;
	static constexpr size_t VOLTAGE_MINIMUM_READINGS = 10;

	// Task constants.
	static const char *TASK_NAME; // Defined in Battery.cpp
	static constexpr size_t TASK_STACK_SIZE = 8192; // Meaning 8192 words, not bytes.
	static constexpr size_t TASK_PRIORITY = 10; // High priority.
	static constexpr size_t TASK_CORE = ARDUINO_RUNNING_CORE;
	static constexpr TickType_t TASK_UPDATE_INTERVAL = pdMS_TO_TICKS(250);

	// A simple enum class that can convey which battery monitoring method is present.
	enum class Chip {
		// There is no dedicated battery monitoring chip present, so we have to use the ADC and
		// read the voltage ourselves.
		ADC,

		// The MAX17048 chip is present and it can do the battery monitoring for us.
		MAX17048
	};

	// Startup functions.
	Chip detectChip();
	void startAnalog();
	void startMAX17048();

	// Task functions.
	[[noreturn]] static void taskAnalog(void *parameter);
	[[noreturn]] static void taskMAX17048(void *parameter);

	// MAX17048 helper function.
	[[nodiscard]] static float adjustStateOfCharge(float stateOfCharge);

	// Analog helper function.
	[[nodiscard]] static float readVoltage();
	[[nodiscard]] static float calculateStateOfCharge(float voltage);

	// Adafruit MAX17048 library.
	Adafruit_MAX17048 adafruitMAX17048;

	// Battery variables.
	float voltage = NAN;
	float stateOfCharge = NAN;
	float chargeRate = NAN;

	// Task variables.
	StackType_t taskStack[TASK_STACK_SIZE];
	StaticTask_t taskBuffer;
	TaskHandle_t taskHandle = nullptr;
	TickType_t lastWake = 0;
};

// A macro to the singleton method of the battery class.
//
// This macro is defined to match the Arduino coding style.
#define Battery BatteryClass::getSingleton()

#endif // BATTERY_HPP
