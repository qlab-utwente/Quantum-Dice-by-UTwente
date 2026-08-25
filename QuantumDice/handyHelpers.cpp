#include <Arduino.h>
#include <Adafruit_MAX1704X.h>
#include <driver/gpio.h>
#include <driver/rtc_io.h>
#include <esp_sleep.h>

#include "defines.hpp"
#include "IMUhelpers.hpp"
#include "handyHelpers.hpp"
#include "DiceConfigManager.hpp"

// Define global configuration object
const struct HardwarePins hwPins{
    .tft_cs = 0xFF,
    .tft_rst = GPIO_NUM_48,
    .tft_dc = GPIO_NUM_47,

    .screen_cs{
        GPIO_NUM_4,
        GPIO_NUM_5,
        GPIO_NUM_6,
        GPIO_NUM_7,
        GPIO_NUM_15,
        GPIO_NUM_16
    },

    .screenAddress{
        // singles
        0b00000100,  // x0
        0b00010000,  // x1
        0b00001000,  // y0
        0b00000010,  // y1
        0b00100000,  // z0
        0b00000001,  // z1
                     // doubles
        0b00010100,  // xx
        0b00001010,  // yy
        0b00100001,  // zz
                     // quarters
        0b00011110,
        0b00101011,
        0b00110101,
        // triples + / -
        0b00101100,  // x0y0z0
        0b00010011,  // x1y1z1
                     // others
        0b00111111,
        0b00000000
    },

    .adc_pin = GPIO_NUM_2
};

// Existing global variables
RTC_DATA_ATTR int bootCount = 0;
Button2 button;
bool clicked = false;
bool longclicked = false;

/**
 * Print hardware pin configuration for debugging
 */
void printHardwarePins() {
    infoln("\n=== Hardware Pin Configuration ===");
    infoln("\nTFT Display Pins:");
    infof("  CS:  GPIO%d\n", hwPins.tft_cs);
    infof("  RST: GPIO%d\n", hwPins.tft_rst);
    infof("  DC:  GPIO%d\n", hwPins.tft_dc);
    infoln("\nScreen CS Pins:");
    for (int i = 0; i < 6; i++) {
        infof("  Screen %d: GPIO%d\n", i, hwPins.screen_cs[i]);
    }
    infof("\nADC Pin: GPIO%d\n", hwPins.adc_pin);
    infoln("==================================\n");
}

void checkTimeForDeepSleep(IMUSensor* imuSensor) {
    static bool isMoving = false;
    static unsigned long lastMovementTime = 0;

    if (imuSensor->stable()) {
        if (isMoving) {
            lastMovementTime = millis();
            isMoving = false;
        }
    } else {
        isMoving = true;
    }

    // Use the timeout from configuration
    if (!isMoving && !button.isPressed() && (millis() - lastMovementTime > currentConfig.deepSleepTimeout)) {
        debugln("Time to sleep");
        digitalWrite(REGULATOR_PIN, HIGH);
        digitalWrite(I2C_POWER_PIN, LOW);
        digitalWrite(SCREEN_POWER_PIN, LOW);
        esp_deep_sleep_start();
    }
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

auto checkMinimumVoltage() -> bool {
    double voltage = getBatteryVoltage();
    return (voltage < MINBATERYVOLTAGE && voltage > 0.5);  //while on USB the voltage is 0
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
