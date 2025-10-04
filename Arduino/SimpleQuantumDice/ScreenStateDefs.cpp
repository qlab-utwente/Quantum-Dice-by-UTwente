#include "Arduino.h"
#include "diceConfig.h"
#include "defines.h"
#include "IMUhelpers.h"
#include "handyHelpers.h"
#include "Screenfunctions.h"
//#include "StateMachine.h"
#include "ScreenStateDefs.h"

State state;                                   //state is used for TruthTable. Is copy of currenState.
DiceNumbers diceNumberSelf, diceNumberSister;  //diceNumberSister is number of sister, diceNumberSelf is own number
UpSide upSideSelf, upSideSister;               //upSideSister is upside from the sister

ScreenStates x0ReqScreenState, x1ReqScreenState, y0ReqScreenState, y1ReqScreenState, z0ReqScreenState, z1ReqScreenState;
MeasuredAxises measureAxisSelf, measureAxisSister;

TruthTableEntry truthTable[] = {
  {
    State::IDLE,
    DiceNumbers::ANY,
    UpSide::ANY,
    ScreenStates::WELCOME,
    ScreenStates::UT_LOGO,
    ScreenStates::GODDICE,
    ScreenStates::GODDICE,
    ScreenStates::QLAB_LOGO,
    ScreenStates::GODDICE,
  },
  {
    State::MEASURE,
    DiceNumbers::ANY,
    UpSide::ANY,
    ScreenStates::MIX1TO6,
    ScreenStates::MIX1TO6,
    ScreenStates::MIX1TO6,
    ScreenStates::MIX1TO6,
    ScreenStates::MIX1TO6,
    ScreenStates::MIX1TO6,
  },
  {
    State::LOWBATTERY,
    DiceNumbers::ANY,
    UpSide::ANY,
    ScreenStates::LOWBATTERY,
    ScreenStates::LOWBATTERY,
    ScreenStates::LOWBATTERY,
    ScreenStates::LOWBATTERY,
    ScreenStates::WELCOME,
    ScreenStates::WELCOME,
  },
  {
    State::MOVING,
    DiceNumbers::ANY,
    UpSide::ANY,
    ScreenStates::MIX1TO6,
    ScreenStates::MIX1TO6,
    ScreenStates::MIX1TO6,
    ScreenStates::MIX1TO6,
    ScreenStates::MIX1TO6,
    ScreenStates::MIX1TO6,
  },
  {
    State::SHOW,
    DiceNumbers::ONE,
    UpSide::X0,
    ScreenStates::N1,
    ScreenStates::N6,
    ScreenStates::N2,
    ScreenStates::N5,
    ScreenStates::N3,
    ScreenStates::N4,
  },
  {
    State::SHOW,
    DiceNumbers::ONE,
    UpSide::X1,
    ScreenStates::N6,
    ScreenStates::N1,
    ScreenStates::N2,
    ScreenStates::N5,
    ScreenStates::N3,
    ScreenStates::N4,
  },
  {
    State::SHOW,
    DiceNumbers::ONE,
    UpSide::Y0,
    ScreenStates::N3,
    ScreenStates::N4,
    ScreenStates::N1,
    ScreenStates::N6,
    ScreenStates::N2,
    ScreenStates::N5,
  },
  {
    State::SHOW,
    DiceNumbers::ONE,
    UpSide::Y1,
    ScreenStates::N3,
    ScreenStates::N4,
    ScreenStates::N6,
    ScreenStates::N1,
    ScreenStates::N2,
    ScreenStates::N5,
  },
  {
    State::SHOW,
    DiceNumbers::ONE,
    UpSide::Z0,
    ScreenStates::N2,
    ScreenStates::N5,
    ScreenStates::N3,
    ScreenStates::N4,
    ScreenStates::N1,
    ScreenStates::N6,
  },
  {
    State::SHOW,
    DiceNumbers::ONE,
    UpSide::Z1,
    ScreenStates::N2,
    ScreenStates::N5,
    ScreenStates::N3,
    ScreenStates::N4,
    ScreenStates::N6,
    ScreenStates::N1,
  },
  {
    State::SHOW,
    DiceNumbers::TWO,
    UpSide::X0,
    ScreenStates::N2,
    ScreenStates::N5,
    ScreenStates::N3,
    ScreenStates::N4,
    ScreenStates::N1,
    ScreenStates::N6,
  },
  {
    State::SHOW,
    DiceNumbers::TWO,
    UpSide::X1,
    ScreenStates::N5,
    ScreenStates::N2,
    ScreenStates::N3,
    ScreenStates::N4,
    ScreenStates::N1,
    ScreenStates::N6,
  },
  {
    State::SHOW,
    DiceNumbers::TWO,
    UpSide::Y0,
    ScreenStates::N1,
    ScreenStates::N6,
    ScreenStates::N2,
    ScreenStates::N5,
    ScreenStates::N3,
    ScreenStates::N4,
  },
  {
    State::SHOW,
    DiceNumbers::TWO,
    UpSide::Y1,
    ScreenStates::N1,
    ScreenStates::N6,
    ScreenStates::N5,
    ScreenStates::N2,
    ScreenStates::N3,
    ScreenStates::N4,
  },
  {
    State::SHOW,
    DiceNumbers::TWO,
    UpSide::Z0,
    ScreenStates::N3,
    ScreenStates::N4,
    ScreenStates::N1,
    ScreenStates::N6,
    ScreenStates::N2,
    ScreenStates::N5,
  },
  {
    State::SHOW,
    DiceNumbers::TWO,
    UpSide::Z1,
    ScreenStates::N3,
    ScreenStates::N4,
    ScreenStates::N1,
    ScreenStates::N6,
    ScreenStates::N5,
    ScreenStates::N2,
  },
  {
    State::SHOW,
    DiceNumbers::THREE,
    UpSide::X0,
    ScreenStates::N3,
    ScreenStates::N4,
    ScreenStates::N1,
    ScreenStates::N6,
    ScreenStates::N2,
    ScreenStates::N5,
  },
  {
    State::SHOW,
    DiceNumbers::THREE,
    UpSide::X1,
    ScreenStates::N4,
    ScreenStates::N3,
    ScreenStates::N1,
    ScreenStates::N6,
    ScreenStates::N2,
    ScreenStates::N5,
  },
  {
    State::SHOW,
    DiceNumbers::THREE,
    UpSide::Y0,
    ScreenStates::N2,
    ScreenStates::N5,
    ScreenStates::N3,
    ScreenStates::N4,
    ScreenStates::N1,
    ScreenStates::N6,
  },
  {
    State::SHOW,
    DiceNumbers::THREE,
    UpSide::Y1,
    ScreenStates::N2,
    ScreenStates::N5,
    ScreenStates::N4,
    ScreenStates::N3,
    ScreenStates::N1,
    ScreenStates::N6,
  },
  {
    State::SHOW,
    DiceNumbers::THREE,
    UpSide::Z0,
    ScreenStates::N1,
    ScreenStates::N6,
    ScreenStates::N2,
    ScreenStates::N5,
    ScreenStates::N3,
    ScreenStates::N4,
  },
  {
    State::SHOW,
    DiceNumbers::THREE,
    UpSide::Z1,
    ScreenStates::N1,
    ScreenStates::N6,
    ScreenStates::N2,
    ScreenStates::N5,
    ScreenStates::N4,
    ScreenStates::N3,
  },
  {
    State::SHOW,
    DiceNumbers::FOUR,
    UpSide::X0,
    ScreenStates::N4,
    ScreenStates::N3,
    ScreenStates::N1,
    ScreenStates::N6,
    ScreenStates::N2,
    ScreenStates::N5,
  },
  {
    State::SHOW,
    DiceNumbers::FOUR,
    UpSide::X1,
    ScreenStates::N3,
    ScreenStates::N4,
    ScreenStates::N1,
    ScreenStates::N6,
    ScreenStates::N2,
    ScreenStates::N5,
  },
  {
    State::SHOW,
    DiceNumbers::FOUR,
    UpSide::Y0,
    ScreenStates::N2,
    ScreenStates::N5,
    ScreenStates::N4,
    ScreenStates::N3,
    ScreenStates::N1,
    ScreenStates::N6,
  },
  {
    State::SHOW,
    DiceNumbers::FOUR,
    UpSide::Y1,
    ScreenStates::N2,
    ScreenStates::N5,
    ScreenStates::N3,
    ScreenStates::N4,
    ScreenStates::N1,
    ScreenStates::N6,
  },
  {
    State::SHOW,
    DiceNumbers::FOUR,
    UpSide::Z0,
    ScreenStates::N1,
    ScreenStates::N6,
    ScreenStates::N2,
    ScreenStates::N5,
    ScreenStates::N4,
    ScreenStates::N3,
  },
  {
    State::SHOW,
    DiceNumbers::FOUR,
    UpSide::Z1,
    ScreenStates::N1,
    ScreenStates::N6,
    ScreenStates::N2,
    ScreenStates::N5,
    ScreenStates::N3,
    ScreenStates::N4,
  },
  {
    State::SHOW,
    DiceNumbers::FIVE,
    UpSide::X0,
    ScreenStates::N5,
    ScreenStates::N2,
    ScreenStates::N3,
    ScreenStates::N4,
    ScreenStates::N1,
    ScreenStates::N6,
  },
  {
    State::SHOW,
    DiceNumbers::FIVE,
    UpSide::X1,
    ScreenStates::N2,
    ScreenStates::N5,
    ScreenStates::N3,
    ScreenStates::N4,
    ScreenStates::N1,
    ScreenStates::N6,
  },
  {
    State::SHOW,
    DiceNumbers::FIVE,
    UpSide::Y0,
    ScreenStates::N1,
    ScreenStates::N6,
    ScreenStates::N5,
    ScreenStates::N2,
    ScreenStates::N3,
    ScreenStates::N4,
  },
  {
    State::SHOW,
    DiceNumbers::FIVE,
    UpSide::Y1,
    ScreenStates::N1,
    ScreenStates::N6,
    ScreenStates::N2,
    ScreenStates::N5,
    ScreenStates::N3,
    ScreenStates::N4,
  },
  {
    State::SHOW,
    DiceNumbers::FIVE,
    UpSide::Z0,
    ScreenStates::N3,
    ScreenStates::N4,
    ScreenStates::N1,
    ScreenStates::N6,
    ScreenStates::N5,
    ScreenStates::N2,
  },
  {
    State::SHOW,
    DiceNumbers::FIVE,
    UpSide::Z1,
    ScreenStates::N3,
    ScreenStates::N4,
    ScreenStates::N1,
    ScreenStates::N6,
    ScreenStates::N2,
    ScreenStates::N5,
  },
  {
    State::SHOW,
    DiceNumbers::SIX,
    UpSide::X0,
    ScreenStates::N6,
    ScreenStates::N1,
    ScreenStates::N2,
    ScreenStates::N5,
    ScreenStates::N3,
    ScreenStates::N4,
  },
  {
    State::SHOW,
    DiceNumbers::SIX,
    UpSide::X1,
    ScreenStates::N1,
    ScreenStates::N6,
    ScreenStates::N2,
    ScreenStates::N5,
    ScreenStates::N3,
    ScreenStates::N4,
  },
  {
    State::SHOW,
    DiceNumbers::SIX,
    UpSide::Y0,
    ScreenStates::N3,
    ScreenStates::N4,
    ScreenStates::N6,
    ScreenStates::N1,
    ScreenStates::N2,
    ScreenStates::N5,
  },
  {
    State::SHOW,
    DiceNumbers::SIX,
    UpSide::Y1,
    ScreenStates::N3,
    ScreenStates::N4,
    ScreenStates::N1,
    ScreenStates::N6,
    ScreenStates::N2,
    ScreenStates::N5,
  },
  {
    State::SHOW,
    DiceNumbers::SIX,
    UpSide::Z0,
    ScreenStates::N2,
    ScreenStates::N5,
    ScreenStates::N3,
    ScreenStates::N4,
    ScreenStates::N6,
    ScreenStates::N1,
  },
  {
    State::SHOW,
    DiceNumbers::SIX,
    UpSide::Z1,
    ScreenStates::N2,
    ScreenStates::N5,
    ScreenStates::N3,
    ScreenStates::N4,
    ScreenStates::N1,
    ScreenStates::N6,
  }
};

ScreenStates getPairState(ScreenStates state) {
  switch (state) {
    case ScreenStates::N1: return ScreenStates::N4;
    case ScreenStates::N2: return ScreenStates::N5;
    case ScreenStates::N3: return ScreenStates::N6;
  }
  return ScreenStates::BLANC;  // Default case, should never be reached
}

bool findValues(State state, DiceNumbers diceNumber, UpSide upSide,
                ScreenStates &x0ScreenState, ScreenStates &x1ScreenState, ScreenStates &y0ScreenState, ScreenStates &y1ScreenState, ScreenStates &z0ScreenState, ScreenStates &z1ScreenState) {
  // bool findValues(State state, UpSide upSide, UpSide upSideSister,
  //                 ScreenStates &x0ScreenState, ScreenStates &x1ScreenState, ScreenStates &y0ScreenState, ScreenStates &y1ScreenState, ScreenStates &z0ScreenState, ScreenStates &z1ScreenState) {
  for (const auto &entry : truthTable) {
    if (entry.state == state && (entry.diceNumber == diceNumber || entry.diceNumber == DiceNumbers::ANY) && (entry.upSide == upSide || entry.upSide == UpSide::ANY)) {
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
      Serial.println("N1 function called");
      break;
    case ScreenStates::N2:
      displayN2(screens);
      Serial.println("N2 function called");
      break;
    case ScreenStates::N3:
      displayN3(screens);
      Serial.println("N3 function called");
      break;
    case ScreenStates::N4:
      displayN4(screens);
      Serial.println("N4 function called");
      break;
    case ScreenStates::N5:
      displayN5(screens);
      Serial.println("N5 function called");
      break;
    case ScreenStates::N6:
      displayN6(screens);
      Serial.println("N6 function called");
      break;
    case ScreenStates::MIX1TO6:
      displayMix1to6(screens);
      Serial.println("MIX1TO6 function called");
      break;
    case ScreenStates::LOWBATTERY:
      displayLowBattery(screens);
      Serial.println("LOWBATTERY function called");
      break;
    case ScreenStates::BLANC:
      blankScreen(screens);
      Serial.println("BLANC function called");
      break;
    case ScreenStates::QLAB_LOGO:
      displayQLab(screens);
      Serial.println("QLAB_LOGO function called");
      break;
    case ScreenStates::UT_LOGO:
      displayUTlogo(screens);
      Serial.println("UT_LOGO function called");
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
    debug("X0:");
    callFunction(x0, X0);
    prevX0 = x0;
  }
  if (y0 != prevY0) {
    debug("Y0:");
    callFunction(y0, Y0);
    prevY0 = y0;
  }
  if (z0 != prevZ0) {
    debug("Z0:");
    callFunction(z0, Z0);
    prevZ0 = z0;
  }
  if (x1 != prevX1) {
    debug("X1:");
    callFunction(x1, X1);
    prevX1 = x1;
  }
  if (y1 != prevY1) {
    debug("Y1:");
    callFunction(y1, Y1);
    prevY1 = y1;
  }
  if (z1 != prevZ1) {
    debug("Z1:");
    callFunction(z1, Z1);
    prevZ1 = z1;
  }
}

void refreshScreens() {
  // set screens, depending of states
  if (findValues(state, diceNumberSelf, upSideSelf, x0ReqScreenState, x1ReqScreenState, y0ReqScreenState, y1ReqScreenState, z0ReqScreenState, z1ReqScreenState)) {

    checkAndCallFunctions(x0ReqScreenState, x1ReqScreenState, y0ReqScreenState, y1ReqScreenState, z0ReqScreenState, z1ReqScreenState);
  } else {
    debugln("no match found");
    // No match found, handle accordingly
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
