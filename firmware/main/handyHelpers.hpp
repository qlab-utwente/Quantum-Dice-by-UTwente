#ifndef HANDYHELPERS_H_
#define HANDYHELPERS_H_

#include "IMUhelpers.hpp"
#include <Button2.h>

// Existing declarations
extern Button2 button;
extern volatile bool clicked;
extern volatile bool longclicked;

void initButton();
void initBattery();
auto getBatteryPercentage() -> float;
auto getBatteryVoltage() -> float;

#endif /* HANDYHELPERS_H_ */
