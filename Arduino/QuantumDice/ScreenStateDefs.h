#include <sys/_stdint.h>
#ifndef SCREENSTATEDEFS_H_
#define SCREENSTATEDEFS_H_

#include "Statemachine.h"  // Ensure this is included
//truth table at the end of this file

extern State stateSelf, stateSister;  //use of the StateMachine inputs

enum class DiceStates : uint8_t {
  SINGLE,
  ENTANGLED_AB1,
  ENTANGLED_AB2,
  MEASURED,
  MEASURED_AFTER_ENT,
  ALL,
  NONE,
  CLASSIC,
  ANY,
  NA
};
extern DiceStates diceStateSelf, prevDiceStateSelf, diceStateSister;

enum class DiceNumbers : uint8_t {
  NONE,
  ONE,
  TWO,
  THREE,
  FOUR,
  FIVE,
  SIX,
  CROSS,
  CIRCLE,
  ANY,
  NA
};
extern DiceNumbers diceNumberSelf, diceNumberSister;

enum class MeasuredAxises : uint8_t {
  UNDEFINED,
  XAXIS,
  YAXIS,
  ZAXIS,
  ALL,
  NA  //not applicable
};
extern MeasuredAxises measureAxisSelf, prevMeasureAxisSelf, measureAxisSister;

enum class UpSide : uint8_t {
  NONE,
  X0,
  X1,
  Y0,
  Y1,
  Z0,
  Z1,
  ANY,
  NA
};
extern UpSide upSideSelf, prevUpSideSelf, upSideSister;

enum class ScreenStates : uint8_t {
  GODDICE,
  WELCOME,
  N1,
  N2,
  N3,
  N4,
  N5,
  N6,
  MIX1TO6,
  MIX1TO6_ENTAB1,
  MIX1TO6_ENTAB2,
  LOWBATTERY,
  BLANC,
  XO,
  XOENTANG,
  DIAGNOSE,
  RESET,
  X_STATE,
  O_STATE,
  QLAB_LOGO,
  QRCODE,
  UT_LOGO
};
extern ScreenStates x0ReqScreenState, x1ReqScreenState, y0ReqScreenState, y1ReqScreenState, z0ReqScreenState, z1ReqScreenState;  //actual screenstates of x, y and z

enum class BlinkStates : uint8_t {
  OFF,
  ON
};
extern BlinkStates blinkState;


struct TruthTableEntry {
  State state;
  DiceStates diceState;
  DiceNumbers diceNumber;
  UpSide upSide;
  ScreenStates x0ScreenState;  //defined screenstate
  ScreenStates x1ScreenState;
  ScreenStates y0ScreenState;
  ScreenStates y1ScreenState;
  ScreenStates z0ScreenState;
  ScreenStates z1ScreenState;
};
// Declare the truth table as an external variable
extern TruthTableEntry truthTable[];

// Function prototypes
bool findValues(State state, DiceStates diceState, DiceNumbers diceNumber, UpSide upSide, ScreenStates &x0ScreenState, ScreenStates &x1ScreenState, ScreenStates &y0ScreenState, ScreenStates &y1ScreenState, ScreenStates &z0ScreenState, ScreenStates &z1ScreenState);

//void printValues(ScreenStates x, ScreenStates y, ScreenStates z);

//const char *toString(ScreenStates value);

void callFunction(ScreenStates result);

void checkAndCallFunctions(ScreenStates x0, ScreenStates x1, ScreenStates y0, ScreenStates y1, ScreenStates z0, ScreenStates z1);
void refreshScreens();
DiceNumbers selectOneToSix();
DiceNumbers selectOppositeOneToSix(DiceNumbers diceNumberTop);
void printDiceStateName(const char *objectName, DiceStates diceState);
void printDiceStateName2(const char *objectName, DiceStates diceState);
MeasuredAxises getAxis();


#endif /* SCREENSTATEDEFS_H_ */