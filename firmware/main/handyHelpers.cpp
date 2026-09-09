#include <Arduino.h>
#include <Adafruit_MAX1704X.h>
#include "handyHelpers.hpp"

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
