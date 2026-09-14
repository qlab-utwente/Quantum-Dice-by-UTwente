#include "power.h"

#include "driver/gpio.h"
#include "esp_sleep.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define POWER_PIN_SHUTDOWN GPIO_NUM_18
#define POWER_PIN_I2C GPIO_NUM_10
#define POWER_PIN_SCREENS GPIO_NUM_3
#define DEEP_SLEEP_LOOP_WAIT_TIME (10 / portTICK_PERIOD_MS)

void power_prepare() {
	static const gpio_config_t GPIO_CONFIG = {
		.pin_bit_mask = (1 << POWER_PIN_SHUTDOWN) | (1 << POWER_PIN_I2C) | (1 << POWER_PIN_SCREENS),
		.mode = GPIO_MODE_OUTPUT,
		.pull_up_en = GPIO_PULLUP_DISABLE,
		.pull_down_en = GPIO_PULLDOWN_DISABLE,
		.intr_type = GPIO_INTR_DISABLE
	};

	gpio_config(&GPIO_CONFIG);
	gpio_set_level(POWER_PIN_SHUTDOWN, 0);
	gpio_set_level(POWER_PIN_I2C, 1);
	gpio_set_level(POWER_PIN_SCREENS, 0);
}

void power_on_screens() {
	gpio_set_level(POWER_PIN_SCREENS, 1);
}

void power_off_screens() {
	gpio_set_level(POWER_PIN_SCREENS, 0);
}

void power_shutdown() {
	gpio_set_level(POWER_PIN_SHUTDOWN, 1);
	gpio_set_level(POWER_PIN_I2C, 0);
	gpio_set_level(POWER_PIN_SCREENS, 0);

	// Try to to start deep sleep in a loop.
	while (true) {
		esp_deep_sleep_try_to_start();
		vTaskDelay(DEEP_SLEEP_LOOP_WAIT_TIME);
	}
}
