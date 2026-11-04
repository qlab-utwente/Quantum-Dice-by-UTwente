#ifndef POWER_HPP
#define POWER_HPP

#include "Arduino.h"

/**
 * A singleton class for interacting with the power pins.
 */
class PowerClass
{
public:
	/**
	 * Prepares the power pins.
	 *
	 * This function sets the pinMode of the three power pins and sets them to their default state.
	 *
	 * Due to a bug in the PCB, the power pin of the I2C is always enabled and held on during deep
	 * sleep, as the button is connected to the power of the I2C devices.
	 */
	inline void prepare() noexcept __attribute__((always_inline)) {
		pinMode(PowerClass::PIN_SHUTDOWN, OUTPUT);
		digitalWrite(PowerClass::PIN_SHUTDOWN, LOW);

		pinMode(PowerClass::PIN_SCREENS, OUTPUT);
		digitalWrite(PowerClass::PIN_SCREENS, LOW);

		pinMode(PowerClass::PIN_I2C, OUTPUT);
		digitalWrite(PowerClass::PIN_I2C, HIGH);
		gpio_hold_en(static_cast<gpio_num_t>(digitalPinToGPIONumber(PowerClass::PIN_I2C)));
	}

	/**
	 * Enables the power to the screens.
	 *
	 * This function sets the SCR_EN pin to HIGH, which will enable the power to the screens.
	 */
	inline void screensOn() noexcept __attribute__((always_inline)) {
		digitalWrite(PowerClass::PIN_SCREENS, HIGH);
	}

	/**
	 * Disables the power to the screens.
	 *
	 * This function sets the SCR_EN pin to LOW, which will disable the power to the screens.
	 */
	inline void screensOff() noexcept __attribute__((always_inline)) {
		digitalWrite(PowerClass::PIN_SCREENS, LOW);
	}

	/**
	 * Shuts down the power on the old PCBs.
	 *
	 * This function sets the SHUTDOWN pin to HIGH, which will cause the old power board to cut the
	 * power.
	 */
	inline void shutdown() noexcept __attribute__((always_inline)) {
		digitalWrite(PowerClass::PIN_SHUTDOWN, HIGH);
	}

	/**
	 * Returns the singleton instance.
	 *
	 * The singleton instance will be instantiated when this function is called for the first time.
	 *
	 * @return The singleton instance.
	 */
	[[nodiscard]] static PowerClass &getSingleton() noexcept {
		static PowerClass singleton;
		return singleton;
	}

	/**
	 * The pin on the new PCBs that is used to enable/disable the power to the screens.
	 *
	 * On the PCB schematics, this pin is named SCR_EN.
	 */
	static constexpr uint8_t PIN_SCREENS = 3;

	/**
	 * The pin on the new PCBs that is used to enable/disable the power to the I2C devices.
	 *
	 * This pin is bugged on the latest version of the PCB and must stay on as the button is
	 * connected to the power of the I2C devices.
	 *
	 * On the PCB schematics, this pin is named I2C_EN.
	 */
	static constexpr uint8_t PIN_I2C = 10;

	/**
	 * The pin on the old PCBs that is used to tell the power board to cut the power.
	 */
	static constexpr uint8_t PIN_SHUTDOWN = 18;

	// Prevent copying.
	PowerClass(const PowerClass &) = delete;
	PowerClass &operator=(const PowerClass &) = delete;

private:
	/**
	 * Constructs the power singleton.
	 */
	constexpr PowerClass() noexcept = default;
};

/**
 * A macro to the singleton method of the power class.
 *
 * This macro is defined to match the Arduino coding style.
 */
#define Power PowerClass::getSingleton()

#endif // POWER_HPP
