#include <Arduino.h>

#include "defines.hpp"
#include "IMUhelpers.hpp"
#include "handyHelpers.hpp"
#include "DiceConfigManager.hpp"

// Define global configuration object
HardwarePins hwPins;

// Existing global variables
RTC_DATA_ATTR int bootCount = 0;
Button2 button;
bool clicked = false;
bool longclicked = false;

/**
 * Initialize hardware pins based on configuration
 * Sets up pin assignments for NANO vs DEVKIT and SMD vs HDR
 */
void initHardwarePins() {
    Serial.println("Initializing hardware pins...");

    // Set TFT pins based on NANO vs DEVKIT
    if (currentConfig.isNano) {
        hwPins.tft_cs = 21;
        hwPins.tft_rst = 4;
        hwPins.tft_dc = 2;
        hwPins.adc_pin = 1;

        // Screen CS pins for NANO
        hwPins.screen_cs[0] = 5;
        hwPins.screen_cs[1] = 6;
        hwPins.screen_cs[2] = 7;
        hwPins.screen_cs[3] = 8;
        hwPins.screen_cs[4] = 9;
        hwPins.screen_cs[5] = 10;
    } else {
        // DEVKIT
        hwPins.tft_cs = 10;
        hwPins.tft_rst = 48;
        hwPins.tft_dc = 47;
        hwPins.adc_pin = 2;

        // Screen CS pins for DEVKIT
        hwPins.screen_cs[0] = 4;
        hwPins.screen_cs[1] = 5;
        hwPins.screen_cs[2] = 6;
        hwPins.screen_cs[3] = 7;
        hwPins.screen_cs[4] = 15;
        hwPins.screen_cs[5] = 16;
    }

    // Set screen address mapping based on SMD vs HDR
    if (currentConfig.isSMD) {
        // SMD screen addresses
        uint8_t smdAddresses[16] = {
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
        };
        memcpy(hwPins.screenAddress, smdAddresses, 16);
    } else {
        // HDR screen addresses
        uint8_t hdrAddresses[16] = {
            // singles
            0b00001000,  // x0
            0b00000010,  // x1
            0b00000100,  // y0
            0b00010000,  // y1
            0b00100000,  // z0
            0b00000001,  // z1
                         // doubles
            0b00001010,  // xx
            0b00010100,  // yy
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
        };
        memcpy(hwPins.screenAddress, hdrAddresses, 16);
    }

    Serial.println("Hardware pins initialized successfully!");
    printHardwarePins();
}

/**
 * Print hardware pin configuration for debugging
 */
void printHardwarePins() {
    infoln("\n=== Hardware Pin Configuration ===");
    infof("Board Type: %s\n", currentConfig.isNano ? "NANO" : "DEVKIT");
    infof("Screen Type: %s\n", currentConfig.isSMD ? "SMD" : "HDR");
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
    if (!isMoving && (millis() - lastMovementTime > currentConfig.deepSleepTimeout)) {
        lastMovementTime = millis();  // Reset the timer
        debugln("Time to sleep");
        digitalWrite(REGULATOR_PIN, HIGH);
    }
}

void initButton() {
    button.begin(BUTTON_PIN, INPUT, false);
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

auto checkMinimumVoltage() -> bool {
    double voltage = analogReadMilliVolts(hwPins.adc_pin) / 1000.0 * 2.0;  //ADC measures 50% of battery voltage by 50/50 voltage divider
                                                                                //debugln(voltage);
    return (voltage < MINBATERYVOLTAGE && voltage > 0.5);  //while on USB the voltage is 0
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
