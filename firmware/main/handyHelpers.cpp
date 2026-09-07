#include <Arduino.h>
#include <Adafruit_MAX1704X.h>
#include <Button2.h>
#include <driver/gpio.h>
#include <driver/rtc_io.h>
#include <esp_sleep.h>

#include "defines.hpp"
#include "handyHelpers.hpp"
#include "DiceConfigManager.hpp"

// Existing global variables
Button2 button;
volatile bool clicked = false;
volatile bool longclicked = false;

static void longClickDetected(Button2& btn) {
	longclicked = true;
}

static void click(Button2& btn) {
	clicked = true;
}

void initButton() {
    if (currentConfig.buttonPullup) {
        button.begin(BUTTON_PIN);

        rtc_gpio_pulldown_dis(BUTTON_PIN);
        rtc_gpio_pullup_en(BUTTON_PIN);
        esp_sleep_enable_ext0_wakeup(BUTTON_PIN, LOW);
    } else {
        button.begin(BUTTON_PIN, INPUT_PULLDOWN, false);

        rtc_gpio_pullup_dis(BUTTON_PIN);
        rtc_gpio_pulldown_en(BUTTON_PIN);
        esp_sleep_enable_ext0_wakeup(BUTTON_PIN, HIGH);
    }

    button.setLongClickDetectedHandler(longClickDetected);
    button.setLongClickTime(1000);
    button.setClickHandler(click);
}

static Adafruit_MAX17048 batteryChip;

void initBattery() {
    batteryChip.begin();
    while (!batteryChip.isDeviceReady()) {
        delay(10);
    }
}

auto getBatteryPercentage() -> float {
    return batteryChip.cellPercent();
}

auto getBatteryVoltage() -> float {
    return batteryChip.cellVoltage();
}
