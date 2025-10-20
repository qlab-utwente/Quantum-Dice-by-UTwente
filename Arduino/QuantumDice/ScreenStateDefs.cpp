#include "Arduino.h"
#include "defines.h"
#include "IMUhelpers.h"
#include "handyHelpers.h"
#include "Screenfunctions.h"
#include "ScreenDeterminator.h"  //TruthTable to select screens from various states
//#include "StateMachine.h"
#include "ScreenStateDefs.h"

State stateSelf, stateSister;  //state is used for TruthTable. Is copy of currenState.
DiceStates diceStateSelf, prevDiceStateSelf, diceStateSister;
MeasuredAxises measureAxisSelf, prevMeasureAxisSelf, measureAxisSister;
DiceNumbers diceNumberSelf, diceNumberSister;     //diceNumberSister is number of sister, diceNumberSelf is own number
UpSide upSideSelf, prevUpSideSelf, upSideSister;  //upSideSister is upside from the sister
ScreenStates x0ReqScreenState, x1ReqScreenState, y0ReqScreenState, y1ReqScreenState, z0ReqScreenState, z1ReqScreenState;

BlinkStates blinkState;

void printDiceStateName(const char *objectName, DiceStates diceState) {
  static DiceStates previousDiceState = DiceStates::NONE;  // Local static variable to retain its value between function calls
                                                           /*
DiceStates::SINGLE
DiceStates::ENTANGLED
DiceStates::MEASURED
DiceStates::ALL
DiceStates::NONE
DiceStates::CLASSIC
*/
  if (diceState != previousDiceState) {
    debug(objectName);
    debug(": ");
    switch (diceState) {
      case DiceStates::SINGLE:
        debugln("DiceStates::SINGLE");
        break;
      case DiceStates::ENTANGLED_AB1:
        debugln("DiceStates::ENTANGLED_AB1");
        break;
      case DiceStates::ENTANGLED_AB2:
        debugln("DiceStates::ENTANGLED_AB2");
        break;
      case DiceStates::MEASURED:
        debugln("DiceStates::MEASURED");
        break;
      case DiceStates::MEASURED_AFTER_ENT:
        debugln("DiceStates::MEASURED_AFTER_ENT");
        break;
      case DiceStates::ALL:
        debugln("DiceStates::ALL");
        break;
      case DiceStates::NONE:
        debugln("DiceStates::NONE");
        break;
      case DiceStates::CLASSIC:
        debugln("DiceStates::CLASSIC");
        break;
    }
    previousDiceState = diceState;  // Update previousState
  }
}

void printDiceStateName2(const char *objectName, DiceStates diceState) {
  static DiceStates previousDiceState = DiceStates::NONE;  // Local static variable to retain its value between function calls
                                                           /*
DiceStates::SINGLE
DiceStates::ENTANGLED
DiceStates::MEASURED
DiceStates::ALL
DiceStates::NONE
DiceStates::CLASSIC
*/
  debug(objectName);
  debug(": ");
  switch (diceState) {
    case DiceStates::SINGLE:
      debugln("DiceStates::SINGLE");
      break;
    case DiceStates::ENTANGLED_AB1:
      debugln("DiceStates::ENTANGLED_AB1");
      break;
    case DiceStates::ENTANGLED_AB2:
      debugln("DiceStates::ENTANGLED_AB2");
      break;
    case DiceStates::MEASURED:
      debugln("DiceStates::MEASURED");
      break;
    case DiceStates::MEASURED_AFTER_ENT:
      debugln("DiceStates::MEASURED_AFTER_ENT");
      break;
    case DiceStates::ALL:
      debugln("DiceStates::ALL");
      break;
    case DiceStates::NONE:
      debugln("DiceStates::NONE");
      break;
    case DiceStates::CLASSIC:
      debugln("DiceStates::CLASSIC");
      break;
  }
}

bool findValues(State state, DiceStates diceState, DiceNumbers diceNumber, UpSide upSide, ScreenStates &x0ScreenState, ScreenStates &x1ScreenState, ScreenStates &y0ScreenState, ScreenStates &y1ScreenState, ScreenStates &z0ScreenState, ScreenStates &z1ScreenState) {
  // bool findValues(State state, DiceNumbers diceNumber, UpSide upSide,
  //                 ScreenStates &x0ScreenState, ScreenStates &x1ScreenState, ScreenStates &y0ScreenState, ScreenStates &y1ScreenState, ScreenStates &z0ScreenState, ScreenStates &z1ScreenState) {
  // bool findValues(State state, UpSide upSide, UpSide upSideSister,
  //                 ScreenStates &x0ScreenState, ScreenStates &x1ScreenState, ScreenStates &y0ScreenState, ScreenStates &y1ScreenState, ScreenStates &z0ScreenState, ScreenStates &z1ScreenState) {
  for (const auto &entry : truthTable) {
    if (entry.state == stateSelf && (entry.diceState == diceStateSelf || entry.diceState == DiceStates::ANY) && (entry.diceNumber == diceNumberSelf || entry.diceNumber == DiceNumbers::ANY) && (entry.upSide == upSideSelf || entry.upSide == UpSide::ANY)) {
      x0ScreenState = entry.x0ScreenState;
      y0ScreenState = entry.y0ScreenState;
      z0ScreenState = entry.z0ScreenState;
      x1ScreenState = entry.x1ScreenState;
      y1ScreenState = entry.y1ScreenState;
      z1ScreenState = entry.z1ScreenState;
      return true;
    }
  }
  return false;  // No match found
}

void callFunction(ScreenStates result, screenselections screens) {
  switch (result) {
    case ScreenStates::GODDICE:
      displayEinstein(screens);
      Serial.println("GODDICE function called");
      break;
    case ScreenStates::WELCOME:
      welcomeInfo(screens);
      Serial.println("WELCOME function called");
      break;
    case ScreenStates::N1:
      displayN1(screens);
      //Serial.println("N1 function called");
      break;
    case ScreenStates::N2:
      displayN2(screens);
      //Serial.println("N2 function called");
      break;
    case ScreenStates::N3:
      displayN3(screens);
      //Serial.println("N3 function called");
      break;
    case ScreenStates::N4:
      displayN4(screens);
      //Serial.println("N4 function called");
      break;
    case ScreenStates::N5:
      displayN5(screens);
      //Serial.println("N5 function called");
      break;
    case ScreenStates::N6:
      displayN6(screens);
      //Serial.println("N6 function called");
      break;
    case ScreenStates::MIX1TO6:
      displayMix1to6(screens);
      //Serial.println("MIX1TO6 function called");
      break;
    case ScreenStates::MIX1TO6_ENTAB1:
      displayMix1to6_entAB1(screens);
      //Serial.println("MIX1TO6_ENTAB1 function called");
      break;
    case ScreenStates::MIX1TO6_ENTAB2:
      displayMix1to6_entAB2(screens);
      //Serial.println("MIX1TO6_ENTAB2 function called");
      break;
    case ScreenStates::LOWBATTERY:
      displayLowBattery(screens);
      Serial.println("LOWBATTERY function called");
      break;
    case ScreenStates::BLANC:
      blankScreen(screens);
      Serial.println("BLANC function called");
      break;
    case ScreenStates::DIAGNOSE:
      voltageIndicator(screens);
      Serial.println("DIAGNOSE function called");
      break;
    case ScreenStates::XO:
      displayCrossCircle(screens);
      Serial.println("XO function called");
      break;
    case ScreenStates::XOENTANG:
      displayEntangled(screens);
      Serial.println("XOENTANG function called");
      break;
    case ScreenStates::RESET:
      displayNewDie(screens);
      Serial.println("RESET function called");
      break;
    case ScreenStates::X_STATE:
      displayCross(screens);
      Serial.println("X_STATE function called");
      break;
    case ScreenStates::O_STATE:
      displayCircle(screens);
      Serial.println("O_STATE function called");
      break;
    case ScreenStates::QLAB_LOGO:
      displayQLab(screens);
      Serial.println("QLAB_LOGO function called");
      break;
    case ScreenStates::UT_LOGO:
      displayUTlogo(screens);
      Serial.println("UT_LOGO function called");
      break;
    case ScreenStates::QRCODE:
      displayQRcode(screens);
      Serial.println("QRCode function called");
      break;
    default:
      Serial.println("No specific function for state");
  }
}
void checkAndCallFunctions(ScreenStates x0, ScreenStates x1, ScreenStates y0, ScreenStates y1, ScreenStates z0, ScreenStates z1) {
  static ScreenStates prevX0 = ScreenStates::BLANC;
  static ScreenStates prevY0 = ScreenStates::BLANC;
  static ScreenStates prevZ0 = ScreenStates::BLANC;
  static ScreenStates prevX1 = ScreenStates::BLANC;
  static ScreenStates prevY1 = ScreenStates::BLANC;
  static ScreenStates prevZ1 = ScreenStates::BLANC;

  if (x0 != prevX0) {
    //debug("X0:");
    callFunction(x0, X0);
    prevX0 = x0;
  }
  if (y0 != prevY0) {
    //debug("Y0:");
    callFunction(y0, Y0);
    prevY0 = y0;
  }
  if (z0 != prevZ0) {
    //debug("Z0:");
    callFunction(z0, Z0);
    prevZ0 = z0;
  }
  if (x1 != prevX1) {
    //debug("X1:");
    callFunction(x1, X1);
    prevX1 = x1;
  }
  if (y1 != prevY1) {
    //debug("Y1:");
    callFunction(y1, Y1);
    prevY1 = y1;
  }
  if (z1 != prevZ1) {
    //debug("Z1:");
    callFunction(z1, Z1);
    prevZ1 = z1;
  }
}

void refreshScreens() {
  // set screens, depending of states
  if (findValues(stateSelf, diceStateSelf, diceNumberSelf, upSideSelf, x0ReqScreenState, x1ReqScreenState, y0ReqScreenState, y1ReqScreenState, z0ReqScreenState, z1ReqScreenState)) {
    checkAndCallFunctions(x0ReqScreenState, x1ReqScreenState, y0ReqScreenState, y1ReqScreenState, z0ReqScreenState, z1ReqScreenState);
  } else {
    debugln("no match found");
    // No match found, handle accordingly
  }
}
DiceNumbers selectOneToSix() {

  //int randomNumber = (int)ECCX08.random(6) + 1;
  uint8_t randomNumber = generateDiceRoll();
  debug("Select one to six. Randomnumber: ");
  debugln(randomNumber);
  switch (randomNumber) {  //dit kan vast handiger
    case 1:
      return DiceNumbers::ONE;
      break;
    case 2:
      return DiceNumbers::TWO;
      break;
    case 3:
      return DiceNumbers::THREE;
      break;
    case 4:
      return DiceNumbers::FOUR;
      break;
    case 5:
      return DiceNumbers::FIVE;
      break;
    case 6:
      return DiceNumbers::SIX;
      break;
  }
}
DiceNumbers selectOppositeOneToSix(DiceNumbers diceNumberTop) {
  debugln("select opposite number");
  switch (diceNumberTop) {  //select the opposite value
    case DiceNumbers::ONE:
      return DiceNumbers::SIX;
      break;
    case DiceNumbers::TWO:
      return DiceNumbers::FIVE;
      break;
    case DiceNumbers::THREE:
      return DiceNumbers::FOUR;
      break;
    case DiceNumbers::FOUR:
      return DiceNumbers::THREE;
      break;
    case DiceNumbers::FIVE:
      return DiceNumbers::TWO;
      break;
    case DiceNumbers::SIX:
      return DiceNumbers::ONE;
      break;
  }
}
MeasuredAxises getAxis(IMUSensor *imuSensor) {
  //detection algoritmn: which side up?
  if (withinBounds(abs(imuSensor->getXGravity()), LOWERBOUND, UPPERBOUND)) {
    return MeasuredAxises::ZAXIS;
  } else if (withinBounds(abs(imuSensor->getYGravity()), LOWERBOUND, UPPERBOUND)) {
    return MeasuredAxises::YAXIS;
  } else if (withinBounds(abs(imuSensor->getZGravity()), LOWERBOUND, UPPERBOUND)) {
    return MeasuredAxises::XAXIS;
  } else {
    debugln("no clear axis");
    return MeasuredAxises::UNDEFINED;
  }
}