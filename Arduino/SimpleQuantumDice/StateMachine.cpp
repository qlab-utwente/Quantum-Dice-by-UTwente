#include "Arduino.h"
#include "diceConfig.h"
#include "defines.h"
#include "ScreenStateDefs.h"
#include "IMUhelpers.h"
#include "handyHelpers.h"
#include "ESPNowHelpers.h"
#include "Screenfunctions.h"
#include "StateMachine.h"

//definitions of functions related to states

const StateMachine::StateFunctions StateMachine::stateFunctions[] = {
  { &StateMachine::enterIDLE, &StateMachine::whileIDLE },
  { &StateMachine::enterSHOW, &StateMachine::whileSHOW },
  { &StateMachine::enterMEASURE, &StateMachine::whileMEASURE },
  { &StateMachine::enterMOVING, &StateMachine::whileMOVING },
  { &StateMachine::enterLOWBATTERY, &StateMachine::whileLOWBATTERY }
};
void printStateName(const char* objectName, State state) {
  static State previousState = State::IDLE;  // Local static variable to retain its value between function calls

  if (state != previousState) {
    debug(objectName);
    debug(": ");
    switch (state) {
      case State::IDLE:
        debugln("IDLE");
        break;
      case State::MEASURE:
        debugln("MEASURE");
        break;
      case State::MOVING:
        debugln("MOVING");
        break;
      case State::SHOW:
        debugln("SHOW");
        break;
      case State::LOWBATTERY:
        debugln("LOWBATTERY");
        break;
    }
    previousState = state;  // Update previousState
  }
}

void setInitialState() {
  diceNumberSister = DiceNumbers::NONE;
  diceNumberSelf = DiceNumbers::NONE;
  upSideSelf = UpSide::NONE;
  upSideSister = UpSide::NONE;
  measureAxisSelf = MeasuredAxises::UNDEFINED;
  measureAxisSister = MeasuredAxises::UNDEFINED;
}

const StateTransition StateMachine::stateTransitions[] = {
  { State::IDLE, Trigger::timed, State::MOVING },  //
  { State::SHOW, Trigger::startRolling, State::MOVING },
  { State::MOVING, Trigger::nonMoving, State::MEASURE },
  { State::MEASURE, Trigger::measurementFail, State::MOVING },
  { State::MEASURE, Trigger::measureXYZ, State::SHOW },
  { State::SHOW, Trigger::lowbattery, State::LOWBATTERY },
  { State::MEASURE, Trigger::lowbattery, State::LOWBATTERY },
  { State::MOVING, Trigger::lowbattery, State::LOWBATTERY }
};

//declaration of instance
StateMachine::StateMachine()
  : currentState(State::IDLE), stateEntryTime(0) {
  // Constructor does not call onEntry
}

void StateMachine::begin() {
  Serial.println("StateMachine Begin: Calling onEntry for initial state");
  (this->*stateFunctions[static_cast<int>(currentState)].onEntry)();
}

void StateMachine::changeState(Trigger trigger) {
  for (const StateTransition& transition : stateTransitions) {
    if (transition.currentState == currentState && transition.trigger == trigger) {
      currentState = transition.nextState;
      printStateName("stateMachine", currentState);
      //add functions called at stateChange.
      (this->*stateFunctions[static_cast<int>(currentState)].onEntry)();
      break;
    }
  }
}

void StateMachine::update() {
  static unsigned long lastUpdateTime = 0;
    _imuSensor->update();
  unsigned long currentTime = millis();
  if (currentTime - lastUpdateTime >= FSM_UPDATE_INTERVAL) {
    //add functions called at state update
    lastUpdateTime = currentTime;
    (this->*stateFunctions[static_cast<int>(currentState)].whileInState)();
  }

  checkTimeForDeepSleep(_imuSensor);
}

void StateMachine::enterIDLE() {
  stateEntryTime = millis();
  state = currentState;
  setInitialState();  //set initial states for screens
  refreshScreens();
}

void StateMachine::whileIDLE() {
  voltageIndicator(X0);

  if (millis() - stateEntryTime > IDLETIME) {
    //setInitialState();  //when leaving the IDLE state, set all states
    changeState(Trigger::timed);
  }
}

void StateMachine::enterSHOW() {
  stateEntryTime = millis();
  state = currentState;
  _imuSensor->reset();
  refreshScreens();
}

void StateMachine::whileSHOW() {
  if (checkMinimumVoltage()) {
    changeState(Trigger::lowbattery);
  } else if (_imuSensor->tumbled(TUMBLECONSTANT)) changeState(Trigger::startRolling);
};

void StateMachine::enterMOVING() {
  stateEntryTime = millis();
  state = currentState;
  diceNumberSelf = DiceNumbers::NONE;  //?????
  upSideSelf = UpSide::NONE;
  measureAxisSelf = MeasuredAxises::UNDEFINED;
  //  debugln("Are you sending measurements?");
  sendMeasurements(diceNumberSelf, upSideSelf, measureAxisSelf);
  refreshScreens();
}

void StateMachine::whileMOVING() {
  if (checkMinimumVoltage()) {
    changeState(Trigger::lowbattery);
  } else if (_imuSensor->isNotMoving()) {
    debugln("isNotMoving triggered");
    changeState(Trigger::nonMoving);
  }
}

void StateMachine::enterMEASURE() {
  stateEntryTime = millis();
  state = currentState;
  if (_imuSensor->isMoving()) {
    changeState(Trigger::measurementFail);  //back to throwing state
  }
}

void StateMachine::whileMEASURE() {  //this could run multiple times when no clear axis can be measured
  bool ready = false;
  int randomNumber = 0;
  debug("gravity XYZ: ");
  debug(_imuSensor->getXGravity());
  debug(", ");
  debug(_imuSensor->getYGravity());
  debug(", ");
  debug(_imuSensor->getZGravity());
  debugln("");

  //detection algoritmn: which side up?
  if (withinBounds(abs(_imuSensor->getXGravity()), LOWERBOUND, UPPERBOUND)) {
    debugln("measureAxisSelf = MeasuredAxises::ZAXIS;");
    measureAxisSelf = MeasuredAxises::ZAXIS;
    if (_imuSensor->getXGravity() < 0) upSideSelf = UpSide::Z1;
    else upSideSelf = UpSide::Z0;
    ready = true;
  } else if (withinBounds(abs(_imuSensor->getYGravity()), LOWERBOUND, UPPERBOUND)) {
    debugln("measureAxisSelf = MeasuredAxises::XAXIS;");
    measureAxisSelf = MeasuredAxises::XAXIS;
    if (_imuSensor->getYGravity() < 0) upSideSelf = UpSide::X0;
    else upSideSelf = UpSide::X1;
    ready = true;  //
  } else if (withinBounds(abs(_imuSensor->getZGravity()), LOWERBOUND, UPPERBOUND)) {
    debugln("measureAxisSelf = MeasuredAxises::YAXIS;");
    measureAxisSelf = MeasuredAxises::YAXIS;
    if (_imuSensor->getZGravity() < 0) upSideSelf = UpSide::Y0;
    else upSideSelf = UpSide::Y1;
    ready = true;  // //
  } else {
    debugln("no clear axis");
    ready = false;
  }

  //determine upSide number and send to sister
  if (ready) {
    switch (upSideSelf) {
      case UpSide::X0:
        debugln("upside X0");
        break;
      case UpSide::X1:
        debugln("upside X1");
        break;
      case UpSide::Y0:
        debugln("upside Y0");
        break;
      case UpSide::Y1:
        debugln("upside Y1");
        break;
      case UpSide::Z0:
        debugln("upside Z0");
        break;
      case UpSide::Z1:
        debugln("upside Z1");
        break;
    }
    //two cases:
    //1. measureAxisSelf and measureAxisSister are equal -> diceNumberSelf follows diceNumberSister (or the opposite)
    //2. measureAxisSelf and measureAxisSister are notequal. A random diceNumber is generated

    //#ifdef ALWAYS_SEVEN
    //    if (measureAxisSister != MeasuredAxises::UNDEFINED) {
    //#else
    if (measureAxisSelf == measureAxisSister) {
      //#endif
      debugln("same axis");

      switch (diceNumberSister) {
        case DiceNumbers::ONE:
          debugln("sisterDiceNumer: 1");
          diceNumberSelf = DiceNumbers::SIX;
          break;
        case DiceNumbers::TWO:
          debugln("sisterDiceNumer: 2");
          diceNumberSelf = DiceNumbers::FIVE;
          break;
        case DiceNumbers::THREE:
          debugln("sisterDiceNumer: 3");
          diceNumberSelf = DiceNumbers::FOUR;
          break;
        case DiceNumbers::FOUR:
          debugln("sisterDiceNumer: 4");
          diceNumberSelf = DiceNumbers::THREE;
          break;
        case DiceNumbers::FIVE:
          debugln("sisterDiceNumer: 5");
          diceNumberSelf = DiceNumbers::TWO;
          break;
        case DiceNumbers::SIX:
          debugln("sisterDiceNumer: 6");
          diceNumberSelf = DiceNumbers::ONE;
          break;
      }
    } else {
      randomNumber = (int)ECCX08.random(6) + 1;
      debug("different axis. Randomnumber: ");
      debugln(randomNumber);
      switch (randomNumber) {  //dit kan vast handiger
        case 1:
          diceNumberSelf = DiceNumbers::ONE;
          break;
        case 2:
          diceNumberSelf = DiceNumbers::TWO;
          break;
        case 3:
          diceNumberSelf = DiceNumbers::THREE;
          break;
        case 4:
          diceNumberSelf = DiceNumbers::FOUR;
          break;
        case 5:
          diceNumberSelf = DiceNumbers::FIVE;
          break;
        case 6:
          diceNumberSelf = DiceNumbers::SIX;
          break;
      }
    }
    debugln("Are you sending measurements?");
    sendMeasurements(diceNumberSelf, upSideSelf, measureAxisSelf);
    changeState(Trigger::measureXYZ);
  }
}

void StateMachine::enterLOWBATTERY() {
  stateEntryTime = millis();
  state = currentState;
  refreshScreens();
};
void StateMachine::whileLOWBATTERY() {
  voltageIndicator(ZZ);
};