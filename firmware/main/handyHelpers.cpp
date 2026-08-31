#include <Arduino.h>
#include <Adafruit_MAX1704X.h>
#include <driver/gpio.h>
#include <driver/rtc_io.h>
#include <esp_sleep.h>

#include "defines.hpp"
#include "IMUhelpers.hpp"
#include "handyHelpers.hpp"
#include "DiceConfigManager.hpp"

// Existing global variables
RTC_DATA_ATTR int bootCount = 0;
Button2 button;
bool clicked = false;
bool longclicked = false;

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

void longClickDetected(Button2& btn) {
    debugln("long pressed");
    longclicked = true;
}

void click(Button2& btn) {
    debugln("short pressed");
    clicked = true;
}

auto generateDiceRoll() -> uint8_t {
    // Get a random 32-bit integer from the crypto chip
    uint32_t randomNumber = esp_random();

    // Check if we got a valid random number (0 indicates error)
    if (randomNumber == 0) {
        Serial.println("ERROR: Failed to get random number");
        return 1;  // Default value in case of error
    }

    // Map to 1-6 range using modulo
    return (randomNumber % 6) + 1;
}

auto generateDiceRollRejection() -> uint8_t {
    uint8_t randomByte;

    do {
        // Get a random byte from the crypto chip
        uint32_t randomNumber = esp_random();

        // Check for error (getRandomByte might return 0 on error)
        if (randomByte == 0) {
            Serial.println("ERROR: Failed to get random byte");
            return 1;
        }
    } while (randomByte >= 252);  // 252 = 6 * 42, ensures uniform distribution

    return (randomByte % 6) + 1;
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

auto mapFloat(float x, float in_min, float in_max, float out_min, float out_max, bool clipOutput) -> float {
    float mappedValue = (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;

    // Apply clipping if clipOutput is true
    if (clipOutput) {
        mappedValue = max(out_min, min(mappedValue, out_max));
    }

    return mappedValue;
}

auto withinBounds(float val, float minimum, float maximum) -> bool {
    return ((minimum <= val) && (val <= maximum));
}

void initSerial() {
    Serial.begin(115200);
    delay(1000);
}
