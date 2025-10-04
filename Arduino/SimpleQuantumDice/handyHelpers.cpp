#include "Arduino.h"
#include "diceConfig.h"
#include "defines.h"
#include "IMUhelpers.h"
#include "ScreenStateDefs.h"
#include "handyHelpers.h"

// Define and initialize the global variables
RTC_DATA_ATTR int bootCount = 0;  // Retains value across deep sleep
bool randomChipPresent = false;   // Default to false
Button2 button;                   // Initialize the Button2 object
bool clicked = false;             // Default to false
bool longclicked = false;         // Default to false
uint8_t ADCpin = 1;               //GPIO1

void initButton() {
  button.begin(BUTTON_PIN, INPUT, false);

  button.setLongClickDetectedHandler(longClickDetected);
  button.setLongClickTime(1000);
  button.setClickHandler(click);

  // button.setChangedHandler(changed);
  // button.setPressedHandler(pressed);
  // button.setReleasedHandler(released);

  // button.setTapHandler(tap);
  // button.setClickHandler(click);
  // button.setLongClickDetectedHandler(longClickDetected);
  // button.setLongClickHandler(longClick);
  // button.setLongClickDetectedRetriggerable(false);

  // button.setDoubleClickHandler(doubleClick);
  // button.setTripleClickHandler(tripleClick);
}

void longClickDetected(Button2& btn) {
  debugln("long pressed");
  longclicked = true;
}

void click(Button2& btn) {
  debugln("short pressed");
  clicked = true;
}


void checkTimeForDeepSleep(IMUSensor *imuSensor) {
  static bool isMoving = false;
  static unsigned long lastMovementTime = 0;

  if (imuSensor->isNotMoving()) {
    if (isMoving) {
      lastMovementTime = millis();
      isMoving = false;
    }
  } else {
    isMoving = true;
  }

  if (!isMoving && (millis() - lastMovementTime > DEEPSLEEPTIMEOUT)) {
    lastMovementTime = millis();  // Reset the timer
    debugln("Time to sleep");
    digitalWrite(REGULATOR_PIN, HIGH);
  }
}



bool checkMinimumVoltage() {
  float voltage = analogReadMilliVolts(ADCpin) / 1000.0 * 2.0;  //A0 measures 50% of batery voltage bij 50/50 voltage devider
  if (voltage < MINBATERYVOLTAGE && voltage > 0.5) //when connected to USB, there is no voltage
    return true;
  else
    return false;
}

float mapFloat(float x, float in_min, float in_max, float out_min, float out_max, bool clipOutput) {
  float mappedValue = (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;

  // Apply clipping if clipOutput is true
  if (clipOutput) {
    mappedValue = max(out_min, min(mappedValue, out_max));
  }

  return mappedValue;
}

bool withinBounds(float val, float minimum, float maximum) {
  return ((minimum <= val) && (val <= maximum));
}

void initSerial() {
  Serial.begin(115200);
  delay(1000);
}
void initRandomGenerators() {
  //create pseudo random numbers based on analogRead value
  randomSeed(analogRead(A0));
  randomChipPresent = ECCX08.begin();
}

int generateRandom() {
  int outcome;
  if (randomChipPresent) {
    debug("random chip used: ");
    outcome = ECCX08.random(100);
  } else {
    debug("randomseed used:");
    outcome = random(100);
  }
  debugln(outcome);
  if (outcome < RANDOMSWITCHPOINT) return 0;
  else return 1;
}
// void printDiceNumber(DiceNumbers diceNumber) {
//   switch (diceNumber) {
//     case DiceNumbers::NONE:
//       debugln("check: DiceNumbers::NONE");
//       break;
//     case DiceNumbers::ONE:
//       debugln("check: DiceNumbers::ONE");
//       break;
//     case DiceNumbers::TWO:
//       debugln("check: DiceNumbers::TWO");
//       break;
//     case DiceNumbers::THREE:
//       debugln("check: DiceNumbers::TREE");
//       break;
//     case DiceNumbers::FOUR:
//       debugln("check: DiceNumbers::FOUR");
//       break;
//     case DiceNumbers::FIVE:
//       debugln("check: DiceNumbers::FIVE");
//       break;
//     case DiceNumbers::SIX:
//       debugln("check: DiceNumbers::SIX");
//       break;
//   }
// }