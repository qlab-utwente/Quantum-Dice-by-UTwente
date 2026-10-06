#ifndef BATTERY_HPP
#define BATTERY_HPP

#include "Arduino.h"
#include "Adafruit_MAX1704X.h"

/**
 * An enum that represents the battery monitor present on the PCB.
 */
enum class BatteryMonitor {
	/**
	 * The battery monitor is unknown.
	 */
	UNKNOWN,

	/**
	 * The battery monitor is the battery voltage connected to an analog pin.
	 */
	ADC,

	/**
	 * The battery monitor is the MAX17048 chip.
	 */
	MAX17048
};

/**
 * A singleton class for interacting with the battery monitor, giving access to the battery voltage
 * and the state of charge of the battery.
 */
class BatteryClass
{
public:
	/**
	 * Starts the battery monitoring.
	 *
	 * Detects which method of battery monitoring is available, prepares that method and then starts
	 * a task that will periodically measure the voltage and determine state of charge.
	 */
	void begin() noexcept;

	/**
	 * Returns the voltage of the battery.
	 *
	 * This function will return NaN if: (1) the battery monitoring has not yet been started, or (2)
	 * the battery monitor failed to measure the battery voltage.
	 */
	[[nodiscard]] constexpr float getVoltage() const noexcept {
		return this->voltage;
	}

	/**
	 * Returns the state of charge of the battery.
	 *
	 * The state of charge is returned as a percentage, so 0% means battery empty and 100% means
	 * battery fully charged.
	 *
	 * This function will return NaN if: (1) the battery monitoring has not yet been started, or (2)
	 * the battery monitor failed to determine the state of charge.
	 *
	 * @return The state of charge of the battery.
	 */
	[[nodiscard]] constexpr float getStateOfCharge() const noexcept {
		return this->stateOfCharge;
	}

	/**
	 * Returns the singleton instance.
	 *
	 * The singleton instance will be instantiated when this function is called for the first time.
	 *
	 * @return The singleton instance.
	 */
	[[nodiscard]] static BatteryClass &getSingleton() noexcept;

	/**
	 * This is the number of the pin to which the battery voltage is connected and from which we
	 * will read the voltage.
	 */
	static constexpr uint8_t VOLTAGE_PIN = 2;

	/**
	 * This is the factor by which the voltage divider connected to the battery voltage pin divides
	 * the voltage. To read the correct voltage, we must multiply the read voltage by this factor.
	 */
	static constexpr float VOLTAGE_DIVIDER_FACTOR = 2.0F;

	/**
	 * The voltage that maps to a state of charge value of 0% when using the battery voltage pin.
	 *
	 * Unit is % state of charge.
	 */
	static constexpr float VOLTAGE_SOC0 = 3.7F;

	/**
	 * The voltage that maps to a state of charge value of 100% when using the battery voltage pin.
	 *
	 * Unit is % state of charge.
	 */
	static constexpr float VOLTAGE_SOC100 = 4.0F;

	/**
	 * The minimum voltage that must be measured to determine that the battery monitor present on
	 * the PCB is the battery voltage pin.
	 *
	 * Unit is voltage.
	 */
	static constexpr float VOLTAGE_MINIMUM = 3.0F;

	/**
	 * The number of battery voltage measurements we must make to determine that the battery monitor
	 * present on the PCB is the battery voltage pin.
	 */
	static constexpr size_t VOLTAGE_MINIMUM_READINGS = 10;

	/**
	 * The state of charge that we will falsely report as 0% state of charge when getting the state
	 * of charge from the MAX17048.
	 *
	 * Unit is % state of charge.
	 */
	static constexpr float MAX17048_FALSE_SOC0 = 10.0F;

	// Prevent copying.
	BatteryClass(const BatteryClass &) = delete;
	BatteryClass &operator=(const BatteryClass &) = delete;

private:
	/**
	 * Constructs the battery class.
	 *
	 * This constructor will initialize all the variables to their default state, except for those
	 * which have been given a different initialization value in their declaration.
	 */
	BatteryClass() noexcept = default;

	/**
	 * This function is run on the battery task and will periodically read from the battery monitor
	 * and then atomically updates the voltage and state of charge in the singleton.
	 *
	 * @param parameter A pointer to pass as parameter to the task, unused.
	 */
	[[noreturn]] static void taskFunction(void *parameter) noexcept;

	// Task constants.
	static const char *TASK_NAME; // Defined in Battery.cpp
	static constexpr size_t TASK_STACK_SIZE = 8192; // Meaning 8192 words, not bytes.
	static constexpr size_t TASK_PRIORITY = 10; // High priority.
	static constexpr size_t TASK_CORE = ARDUINO_RUNNING_CORE;
	static constexpr TickType_t TASK_UPDATE_INTERVAL = pdMS_TO_TICKS(250);

	// The library object we use to interact with the MAX17048.
	Adafruit_MAX17048 adafruitMAX17048;

	// Battery variables.
	BatteryMonitor monitor = BatteryMonitor::UNKNOWN;
	float voltage = NAN;
	float stateOfCharge = NAN;

	// Task variables.
	StackType_t taskStack[TASK_STACK_SIZE];
	StaticTask_t taskBuffer;
	TaskHandle_t taskHandle;
};

/**
 * Returns a string representation of the battery monitor.
 *
 * @return A string representation of the battery monitor.
 */
const char *toString(const BatteryMonitor monitor) noexcept;

/**
 * A macro to the singleton method of the battery class.
 *
 * This macro is defined to match the Arduino coding style.
 */
#define Battery BatteryClass::getSingleton()

#endif // BATTERY_HPP
