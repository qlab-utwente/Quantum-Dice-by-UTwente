#ifndef HANDYHELPERS_H_
#define HANDYHELPERS_H_

#include "IMUhelpers.hpp"
#include <Button2.h>
#include <cstdint>

// Existing declarations
extern RTC_DATA_ATTR int bootCount;
extern Button2           button;
extern bool              clicked;
extern bool              longclicked;

void initButton();
void initBattery();
void longClickDetected(Button2 &btn);
void click(Button2 &btn);
auto getBatteryPercentage() -> float;
auto getBatteryVoltage() -> float;
auto mapFloat(float x, float in_min, float in_max, float out_min, float out_max, bool clipOutput) -> float;
auto withinBounds(float val, float minimum, float maximum) -> bool;
void initSerial();
auto generateDiceRollRejection() -> uint8_t;
auto generateDiceRoll() -> uint8_t;

#endif /* HANDYHELPERS_H_ */
