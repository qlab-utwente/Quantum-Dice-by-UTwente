#include <sys/_stdint.h>
#ifndef HANDYHELPERS_H_
#define HANDYHELPERS_H_

#include <SparkFun_ATECCX08a_Arduino_Library.h>
#include <Button2.h>

extern RTC_DATA_ATTR int bootCount; // To retain value across deep sleep cycles
extern bool randomChipPresent;
extern Button2 button;
extern bool clicked;
extern bool longclicked;
extern uint8_t ADCpin;

void initButton();
void longClickDetected(Button2& btn);
void click(Button2& btn);
void checkTimeForDeepSleep(IMUSensor *imuSensor);
int generateRandom();
bool checkMinimumVoltage();
float mapFloat(float x, float in_min, float in_max, float out_min, float out_max, bool clipOutput);
bool withinBounds(float val, float minimum, float maximum);
void initSerial();
void initRandomGenerators();
uint8_t generateDiceRollRejection();
uint8_t generateDiceRoll();

#endif /* HANDYHELPERS_H_ */