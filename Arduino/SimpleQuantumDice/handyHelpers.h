#include <sys/_stdint.h>
#ifndef HANDYHELPERS_H_
#define HANDYHELPERS_H_

#include <ArduinoECCX08.h>
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
bool checkMinimumVoltage();
float mapFloat(float x, float in_min, float in_max, float out_min, float out_max, bool clipOutput);
bool withinBounds(float val, float minimum, float maximum);
void initSerial();
//void printDiceNumber(DiceNumbers diceNumber);
void initRandomGenerators();
int generateRandom();


#endif /* HANDYHELPERS_H_ */