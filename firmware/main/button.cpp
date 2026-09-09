#include "button.h"

#include "Arduino.h"
#include "defines.hpp"
#include "driver/gpio.h"
#include "driver/rtc_io.h"
#include "esp_sleep.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "stdint.h"

// Button constants.
#define BUTTON_PIN GPIO_NUM_14
#define BUTTON_TIME_DEBOUNCE 50'000
#define BUTTON_TIME_PRESS_DOUBLE 300'000
#define BUTTON_TIME_PRESS_LONG 1'000'000

// Notifications.
#define NOTIFICATION_EDGE 0x01
#define NOTIFICATION_TIMER 0x02

// Task constants.
#define TASK_STACK_SIZE 8192 // Number of words on the stack.
#define TASK_PRIORITY 10
#define TASK_CORE 1

// Task constant and variables.
static const char *TASK_NAME = "button_task";
static StackType_t task_stack[TASK_STACK_SIZE];
static StaticTask_t task_buffer;
static TaskHandle_t task_handle;

// Button timer constant and handle.
static const char *BUTTON_TIMER_NAME = "button_timer";
static esp_timer_handle_t button_timer_handle = NULL;

// Button variables.
static bool button_pullup = false;
static bool button_press_short = false;
static bool button_press_double = false;
static bool button_press_long = false;
static int64_t button_last_short_release_time = 0;
static int32_t button_level = 0;
static enum {
	BUTTON_STATE_RELEASED,
	BUTTON_STATE_DEBOUNCE,
	BUTTON_STATE_PRESS_SHORT,
	BUTTON_STATE_AWAIT_PRESS_DOUBLE,
	BUTTON_STATE_DEBOUNCE_DOUBLE,
	BUTTON_STATE_PRESS_DOUBLE,
	BUTTON_STATE_PRESS_LONG,
	BUTTON_STATE_PRESS_IGNORE
} button_state = BUTTON_STATE_RELEASED;

// Button interrupt and callback.
static void IRAM_ATTR interrupt_edge(void *parameter);
static void callback_timer(void *parameter);

// Task functions.
static void task_function(void *parameter);
static inline void task_setup();
static inline void task_loop();

// Button edge interrupt functions.
static inline void edge_setup();
static inline void edge_handle();

// Button timer functions.
static inline void timer_setup();
static inline void timer_handle();

// Functions handling press notifications.
static inline void handle_press_short();
static inline void handle_press_double();
static inline void handle_press_long();

void button_start(bool pullup) {
	button_pullup = pullup;

	task_handle = xTaskCreateStaticPinnedToCore(
		task_function,
		TASK_NAME,
		TASK_STACK_SIZE,
		NULL,
		TASK_PRIORITY,
		task_stack,
		&task_buffer,
		TASK_CORE
	);
}

bool button_is_pressed() {
	return button_state == BUTTON_STATE_DEBOUNCE
			|| button_state == BUTTON_STATE_PRESS_SHORT
			|| button_state == BUTTON_STATE_DEBOUNCE_DOUBLE
			|| button_state == BUTTON_STATE_PRESS_DOUBLE
			|| button_state == BUTTON_STATE_PRESS_LONG;
}

bool button_poll_press_short() {
	if (button_press_short) {
		button_press_short = false;
		return true;
	} else {
		return false;
	}
}

bool button_poll_press_double() {
	if (button_press_double) {
		button_press_double = false;
		return true;
	} else {
		return false;
	}
}

bool button_poll_press_long() {
	if (button_press_long) {
		button_press_long = false;
		return true;
	} else {
		return false;
	}
}

static void IRAM_ATTR interrupt_edge(void *parameter) {
	BaseType_t higher_priority_task_woken = pdFALSE;

	xTaskNotifyFromISR(
		task_handle,
		NOTIFICATION_EDGE,
		eSetBits,
		&higher_priority_task_woken
	);

	portYIELD_FROM_ISR(higher_priority_task_woken);
}

static void callback_timer(void *parameter) {
	xTaskNotify(
		task_handle,
		NOTIFICATION_TIMER,
		eSetBits
	);
}

static void task_function(void *parameter) {
	task_setup();

	for ( ; ; ) {
		task_loop();
	}
}

static inline void task_setup() {
	edge_setup();
	timer_setup();
}

static inline void task_loop() {
	static uint32_t notification_value = 0;
	xTaskNotifyWait(
		0,
		UINT32_MAX,
		&notification_value,
		portMAX_DELAY
	);

	if ((notification_value & NOTIFICATION_TIMER) != 0) {
		timer_handle();
	}

	if ((notification_value & NOTIFICATION_EDGE) != 0) {
		edge_handle();
	}
}

static inline void edge_setup() {
	static gpio_config_t GPIO_CONFIG = {
		.pin_bit_mask = (1 << BUTTON_PIN),
		.mode = GPIO_MODE_INPUT,
		.pull_up_en = button_pullup ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE,
		.pull_down_en = button_pullup ? GPIO_PULLDOWN_DISABLE : GPIO_PULLDOWN_ENABLE,
		.intr_type = GPIO_INTR_ANYEDGE
	};

	gpio_config(&GPIO_CONFIG);
	gpio_install_isr_service(ESP_INTR_FLAG_IRAM);
	gpio_isr_handler_add(BUTTON_PIN, interrupt_edge, NULL);
	button_level = gpio_get_level(BUTTON_PIN);

	if (button_pullup) {
		rtc_gpio_pulldown_dis(BUTTON_PIN);
		rtc_gpio_pullup_en(BUTTON_PIN);
		esp_sleep_enable_ext0_wakeup(BUTTON_PIN, 0);

		if (button_level == 0) {
			button_state = BUTTON_STATE_PRESS_IGNORE;
		}
	} else {
		rtc_gpio_pullup_dis(BUTTON_PIN);
		rtc_gpio_pulldown_en(BUTTON_PIN);
		esp_sleep_enable_ext0_wakeup(BUTTON_PIN, 1);

		if (button_level == 1) {
			button_state = BUTTON_STATE_PRESS_IGNORE;
		}
	}
}

static inline void edge_handle() {
	int32_t level = gpio_get_level(BUTTON_PIN);
	if (level == button_level) {
		return;
	}

	bool rising_edge = level > button_level;
	bool press_edge = button_pullup ? !rising_edge : rising_edge;
	button_level = level;

	if (button_state == BUTTON_STATE_RELEASED && press_edge) {
		button_state = BUTTON_STATE_DEBOUNCE;
		esp_timer_start_once(button_timer_handle, BUTTON_TIME_DEBOUNCE);
	} else if (button_state == BUTTON_STATE_DEBOUNCE && !press_edge) {
		button_state = BUTTON_STATE_RELEASED;
		esp_timer_stop(button_timer_handle);
	} else if (button_state == BUTTON_STATE_PRESS_SHORT && !press_edge) {
		button_state = BUTTON_STATE_AWAIT_PRESS_DOUBLE;
		button_last_short_release_time = esp_timer_get_time();
		esp_timer_start_once(button_timer_handle, BUTTON_TIME_PRESS_DOUBLE);
	} else if (button_state == BUTTON_STATE_AWAIT_PRESS_DOUBLE && press_edge) {
		button_state = BUTTON_STATE_DEBOUNCE_DOUBLE;
		esp_timer_stop(button_timer_handle);
		esp_timer_start_once(button_timer_handle, BUTTON_TIME_DEBOUNCE);
	} else if (button_state == BUTTON_STATE_DEBOUNCE_DOUBLE && !press_edge) {
		esp_timer_stop(button_timer_handle);
		int64_t time_difference = esp_timer_get_time() - button_last_short_release_time;
		int64_t time_left = BUTTON_TIME_PRESS_DOUBLE - time_difference;

		if (time_left > 0) {
			button_state = BUTTON_STATE_AWAIT_PRESS_DOUBLE;
			esp_timer_start_once(button_timer_handle, time_left);
		} else {
			button_state = BUTTON_STATE_RELEASED;
			handle_press_short();
		}
	} else if (
				(button_state == BUTTON_STATE_PRESS_DOUBLE
					|| button_state == BUTTON_STATE_PRESS_LONG
					|| button_state == BUTTON_STATE_PRESS_IGNORE)
				&& !press_edge
			) {
		button_state = BUTTON_STATE_RELEASED;
	}
}

static inline void timer_setup() {
	static const esp_timer_create_args_t TIMER_CREATE_ARGS = {
		.callback = callback_timer,
		.arg = NULL,
		.dispatch_method = ESP_TIMER_TASK,
		.name = BUTTON_TIMER_NAME,
	};

	esp_timer_create(&TIMER_CREATE_ARGS, &button_timer_handle);
}

static inline void timer_handle() {
	switch (button_state) {
		case BUTTON_STATE_DEBOUNCE:
			button_state = BUTTON_STATE_PRESS_SHORT;
			esp_timer_start_once(button_timer_handle, BUTTON_TIME_PRESS_LONG - BUTTON_TIME_DEBOUNCE);
			break;

		case BUTTON_STATE_PRESS_SHORT:
			button_state = BUTTON_STATE_PRESS_LONG;
			handle_press_long();
			break;

		case BUTTON_STATE_AWAIT_PRESS_DOUBLE:
			button_state = BUTTON_STATE_RELEASED;
			handle_press_short();
			break;

		case BUTTON_STATE_DEBOUNCE_DOUBLE:
			button_state = BUTTON_STATE_PRESS_DOUBLE;
			handle_press_double();
			break;
	}
}

static inline void handle_press_short() {
	debugln("BUTTON NOTIFY SHORT");
	button_press_short = true;
}

static inline void handle_press_double() {
	debugln("BUTTON NOTIFY DOUBLE");
	button_press_double = true;
}

static inline void handle_press_long() {
	debugln("BUTTON NOTIFY LONG");
	button_press_long = true;
}
