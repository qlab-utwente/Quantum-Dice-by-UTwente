#ifndef HANDYHELPERS_H_
#define HANDYHELPERS_H_

#include "IMUhelpers.hpp"
#include <Button2.h>
#include <cstdint>

// Existing declarations
extern RTC_DATA_ATTR int bootCount;
extern Button2 button;
extern volatile bool clicked;
extern volatile bool longclicked;

void initButton();
void initBattery();
auto getBatteryPercentage() -> float;
auto getBatteryVoltage() -> float;
auto mapFloat(float x, float in_min, float in_max, float out_min, float out_max, bool clipOutput) -> float;
auto withinBounds(float val, float minimum, float maximum) -> bool;
auto generateDiceRollRejection() -> uint8_t;
auto generateDiceRoll() -> uint8_t;

#endif /* HANDYHELPERS_H_ */
