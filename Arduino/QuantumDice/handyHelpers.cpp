#include "Arduino.h"
#include "diceConfig.h"
#include "defines.h"
#include "IMUhelpers.h"
#include "handyHelpers.h"

// Define and initialize the global variables
ATECCX08A atecc;
RTC_DATA_ATTR int bootCount = 0;  // Retains value across deep sleep
bool randomChipPresent = false;   // Default to false
Button2 button;                   // Initialize the Button2 object
bool clicked = false;             // Default to false
bool longclicked = false;         // Default to false

#if defined(NANO)
uint8_t ADCpin = 1;               //GPIO1
#endif

#if defined(DEVKIT)
uint8_t ADCpin = 2;               //GPIO2
#endif

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

void initRandomGenerators() {
  //create pseudo random numbers based on analogRead value
  randomSeed(analogRead(A0));
  randomChipPresent = atecc.begin();
}

uint8_t generateDiceRoll() {
  // Get a random 32-bit integer from the crypto chip
  uint32_t randomNumber = atecc.getRandomInt();

  // Check if we got a valid random number (0 indicates error)
  if (randomNumber == 0) {
    Serial.println("ERROR: Failed to get random number");
    return 1;  // Default value in case of error
  }

  // Map to 1-6 range using modulo
  return (randomNumber % 6) + 1;
}

uint8_t generateDiceRollRejection() {
  uint8_t randomByte;

  do {
    // Get a random byte from the crypto chip
    randomByte = atecc.getRandomByte();

    // Check for error (getRandomByte might return 0 on error)
    if (randomByte == 0) {
      Serial.println("ERROR: Failed to get random byte");
      return 1;
    }
  } while (randomByte >= 252);  // 252 = 6 * 42, ensures uniform distribution

  return (randomByte % 6) + 1;
}

// int generateRandom() {
//   long outcome;
//   if (randomChipPresent) {
//     debug("random chip used: ");
//     outcome = ECCX08.random(100);
//   } else {
//     debug("randomseed used:");
//     outcome = random(100);
//   }
//   debugln(outcome);
//   if (outcome < RANDOMSWITCHPOINT) return 0;
//   else return 1;
// }

bool checkMinimumVoltage() {
  float voltage = analogReadMilliVolts(ADCpin) / 1000.0 * 2.0;  //A0 measures 50% of batery voltage bij 50/50 voltage devider
  if (voltage < MINBATERYVOLTAGE && voltage > 0.5) //while on USB the voltage is 0
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