#include "Arduino.h"
#include "defines.h"
#include "ScreenStateDefs.h"
#include "handyHelpers.h"
#include "IMUhelpers.h"
#include "Screenfunctions.h"
#include "StateMachine.h"
#include "EspNowSensor.h"

typedef enum _message_type {
  MESSAGE_TYPE_WATCH_DOG,
  MESSAGE_TYPE_MEASUREMENT,
  MESSAGE_TYPE_ENTANGLE_REQUEST,
  MESSAGE_TYPE_ENTANGLE_CONFIRM,
  MESSAGE_TYPE_ENTANGLE_STOP
} message_type;

typedef struct _message {
  message_type type;
  Roles senderRole;
  union _data {
    struct _watchDogData {
      State state;
    } watchDog;
    struct _measurementData {
      State state;
      DiceStates diceState;
      MeasuredAxises measureAxis;
      DiceNumbers diceNumber;
      UpSide upSide;
    } measurement;
  } data;
} message;


//definitions of functions related to states
const StateMachine::StateFunctions StateMachine::stateFunctions[] = {
  { &StateMachine::enterIDLE, &StateMachine::whileIDLE },
  { &StateMachine::enterINITSINGLE, &StateMachine::whileINITSINGLE },
  { &StateMachine::enterINITENTANGLED_AB1, &StateMachine::whileINITENTANGLED_AB1 },
  { &StateMachine::enterWAITFORTHROW, &StateMachine::whileWAITFORTHROW },
  { &StateMachine::enterTHROWING, &StateMachine::whileTHROWING },
  { &StateMachine::enterINITMEASURED, &StateMachine::whileINITMEASURED },
  { &StateMachine::enterLOWBATTERY, &StateMachine::whileLOWBATTERY },
  { &StateMachine::enterCLASSIC_STATE, &StateMachine::whileCLASSIC_STATE },
  { &StateMachine::enterINITENTANGLED_AB2, &StateMachine::whileINITENTANGLED_AB2 },
  { &StateMachine::enterINITSINGLE_AFTER_ENT, &StateMachine::whileINITSINGLE_AFTER_ENT }
};

// Get MAC address for a specific role - use config values
uint8_t* getMacForRole(Roles role) {
  switch (role) {
    case Roles::ROLE_A: return currentConfig.deviceA_mac;
    case Roles::ROLE_B1: return currentConfig.deviceB1_mac;
    case Roles::ROLE_B2: return currentConfig.deviceB2_mac;
    default: return nullptr;
  }
}

void printRole(Roles role) {
  uint8_t macAddress[6];
  switch (role) {
    case Roles::ROLE_A: Serial.println("ROLE_A"); break;
    case Roles::ROLE_B1: Serial.println("ROLE_B1"); break;
    case Roles::ROLE_B2: Serial.println("ROLE_B2"); break;
    default: Serial.println("NONE"); break;
  }
}

void StateMachine::determineRoles() {
  uint8_t selfMac[6];
  EspNowSensor<message>::GetMacAddress(selfMac);
  roleA = Roles::ROLE_A;
  roleB1 = Roles::ROLE_B1;
  roleB2 = Roles::ROLE_B2;

  // Determine own role by comparing with known MACs from config
  if (memcmp(selfMac, currentConfig.deviceA_mac, 6) == 0) {
    isDeviceA = true;
    roleSelf = Roles::ROLE_A;
    roleSister = Roles::ROLE_B1;
    roleBrother = Roles::ROLE_B2;
  } else if (memcmp(selfMac, currentConfig.deviceB1_mac, 6) == 0) {
    isDeviceB1 = true;
    roleSelf = Roles::ROLE_B1;
    roleSister = Roles::ROLE_A;
    roleBrother = Roles::ROLE_B2;
  } else if (memcmp(selfMac, currentConfig.deviceB2_mac, 6) == 0) {
    isDeviceB2 = true;
    roleSelf = Roles::ROLE_B2;
    roleSister = Roles::ROLE_B1;
    roleBrother = Roles::ROLE_A;
  } else {
    roleSelf = Roles::NONE;
    Serial.println("WARNING: This device's MAC address doesn't match any known devices!");
  }

  // Print the role assignment
  Serial.print("Self role: ");
  switch (roleSelf) {
    case Roles::ROLE_A: Serial.println("ROLE_A"); break;
    case Roles::ROLE_B1: Serial.println("ROLE_B1"); break;
    case Roles::ROLE_B2: Serial.println("ROLE_B2"); break;
    default: Serial.println("NONE"); break;
  }
}

void printStateName(const char* objectName, State state) {
  static State previousState = State::IDLE;  // Local static variable to retain its value between function calls

  if (state != previousState) {
    debug(objectName);
    debug(": ");
    switch (state) {
      case State::IDLE:
        debugln("IDLE");
        break;
      case State::INITSINGLE:
        debugln("INITSINGLE");
        break;
      case State::INITENTANGLED_AB1:
        debugln("INITENTANGLED_AB1");
        break;
      case State::INITENTANGLED_AB2:
        debugln("INITENTANGLED_AB2");
        break;
      case State::INITSINGLE_AFTER_ENT:
        debugln("INITSINGLE_AFTER_ENT");
        break;
      case State::WAITFORTHROW:
        debugln("WAITFORTHROW");
        break;
      case State::THROWING:
        debugln("THROWING");
        break;
      case State::INITMEASURED:
        debugln("INITMEASURED");
        break;
      case State::LOWBATTERY:
        debugln("LOWBATTERY");
        break;
      case State::CLASSIC_STATE:
        debugln("CLASSIC_STATE");
        break;
    }
    previousState = state;  // Update previousState
  }
}

void StateMachine::sendWatchDog() {
  message watchDog;
  watchDog.senderRole = roleSelf;
  watchDog.type = MESSAGE_TYPE_WATCH_DOG;
  watchDog.data.watchDog.state = stateSelf;
  EspNowSensor<message>::Send(watchDog, getMacForRole(roleBrother));
  EspNowSensor<message>::Send(watchDog, getMacForRole(roleSister));
}

void StateMachine::sendMeasurements(Roles targetRole, State state, DiceStates diceState, DiceNumbers diceNumber, UpSide upSide, MeasuredAxises measureAxis) {
  message myData;
  debugln("Send Measurements message initated");
  myData.senderRole = roleSelf;
  myData.type = MESSAGE_TYPE_MEASUREMENT;
  myData.data.measurement.state = state;
  myData.data.measurement.diceState = diceState;
  myData.data.measurement.measureAxis = measureAxis;
  myData.data.measurement.diceNumber = diceNumber;
  myData.data.measurement.upSide = upSide;
  EspNowSensor<message>::Send(myData, getMacForRole(targetRole));
}

void StateMachine::sendEntangleRequest(Roles targetRole) {
  message myData;
  myData.senderRole = roleSelf;
  myData.type = MESSAGE_TYPE_ENTANGLE_REQUEST;
  EspNowSensor<message>::Send(myData, getMacForRole(targetRole));
}

void StateMachine::sendEntanglementConfirm(Roles targetRole) {
  debugln("Send entanglement confirm");
  message myData;
  myData.senderRole = roleSelf;
  myData.type = MESSAGE_TYPE_ENTANGLE_CONFIRM;
  EspNowSensor<message>::Send(myData, getMacForRole(targetRole));
}

void StateMachine::sendStopEntanglement(Roles targetRole) {
  debugln("Send stop Entanglement");
  message myData;
  myData.senderRole = roleSelf;
  myData.type = MESSAGE_TYPE_ENTANGLE_STOP;
  EspNowSensor<message>::Send(myData, getMacForRole(targetRole));
}

void setInitialState() {
  diceStateSelf = DiceStates::SINGLE;
  prevDiceStateSelf = DiceStates::NONE;
  measureAxisSelf = MeasuredAxises::UNDEFINED;
  prevMeasureAxisSelf = MeasuredAxises::UNDEFINED;
  diceNumberSelf = DiceNumbers::NONE;
  upSideSelf = UpSide::NONE;
  prevUpSideSelf = UpSide::NONE;
}

const StateTransition StateMachine::stateTransitions[] = {
  { State::IDLE, Trigger::timed, State::CLASSIC_STATE },
  { State::CLASSIC_STATE, Trigger::buttonPressed, State::INITSINGLE },
  { State::INITSINGLE, Trigger::timed, State::WAITFORTHROW },
  { State::INITSINGLE, Trigger::lowbattery, State::LOWBATTERY },
  { State::INITENTANGLED_AB1, Trigger::lowbattery, State::LOWBATTERY },
  { State::INITENTANGLED_AB1, Trigger::timed, State::WAITFORTHROW },
  { State::WAITFORTHROW, Trigger::buttonPressed, State::INITSINGLE },
  { State::WAITFORTHROW, Trigger::closeByAB1, State::INITENTANGLED_AB1 },
  { State::WAITFORTHROW, Trigger::startRolling, State::THROWING },
  { State::WAITFORTHROW, Trigger::lowbattery, State::LOWBATTERY },
  { State::WAITFORTHROW, Trigger::timed, State::INITSINGLE },
  { State::THROWING, Trigger::nonMoving, State::INITMEASURED },
  { State::THROWING, Trigger::lowbattery, State::LOWBATTERY },
  { State::INITMEASURED, Trigger::measureXYZ, State::WAITFORTHROW },
  { State::INITMEASURED, Trigger::measurementFail, State::THROWING },
  { State::INITMEASURED, Trigger::lowbattery, State::LOWBATTERY },
  { State::CLASSIC_STATE, Trigger::lowbattery, State::LOWBATTERY },
  { State::INITENTANGLED_AB2, Trigger::lowbattery, State::LOWBATTERY },
  { State::INITENTANGLED_AB2, Trigger::timed, State::WAITFORTHROW },
  { State::WAITFORTHROW, Trigger::closeByAB2, State::INITENTANGLED_AB2 },
  { State::INITSINGLE_AFTER_ENT, Trigger::lowbattery, State::LOWBATTERY },
  { State::INITSINGLE_AFTER_ENT, Trigger::timed, State::WAITFORTHROW },
  { State::WAITFORTHROW, Trigger::entangleStopReceived, State::INITSINGLE_AFTER_ENT }
};

//declaration of instance
StateMachine::StateMachine()
  : currentState(State::IDLE), stateEntryTime(0) {
  // Constructor does not call onEntry. That's done in StateMachine::begin()
}

void StateMachine::begin() {
  // Initialize ESP-NOW with device A MAC from config
  EspNowSensor<message>::Init(currentConfig.deviceA_mac);

  // Determine roles based on MAC address
  determineRoles();

  // Check if role was assigned
  assert(roleSelf != Roles::NONE && "Role cannot be NONE");

  // Register peers (both sister and brother devices)
  EspNowSensor<message>::AddPeer(getMacForRole(roleSister));
  EspNowSensor<message>::AddPeer(getMacForRole(roleBrother));

  Serial.println("ESP-NOW initialized successfully!");

  EspNowSensor<message>::PrintMacAddress();

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

  message data;

  while (EspNowSensor<message>::Poll(&data)) {
    switch (data.type) {
      case MESSAGE_TYPE_WATCH_DOG:  // watch dog, send by all dices
        stateSister = data.data.watchDog.state;
        break;

      case MESSAGE_TYPE_MEASUREMENT:  //send by 2 entangled dices to each other  (A <->B1 or A<->B2). Just store the data in the sisterStates
        debug("measurement data received from ");
        printRole(data.senderRole);
        stateSister = data.data.measurement.state;
        diceStateSister = data.data.measurement.diceState;
        diceNumberSister = data.data.measurement.diceNumber;
        measureAxisSister = data.data.measurement.measureAxis;
        measurementReceived = true;
        break;

      case MESSAGE_TYPE_ENTANGLE_REQUEST:  //device B1 and/or B2 receives entangle request from A
        entangleRequestRcvA = true;
        break;

      case MESSAGE_TYPE_ENTANGLE_CONFIRM:  //device A receives confirmation entangle request
        debug("entanglement confirmation received from ");
        printRole(data.senderRole);
        switch (data.senderRole) {
          case Roles::ROLE_B1:
            entangleConfirmRcvB1 = true;
            break;
          case Roles::ROLE_B2:
            entangleConfirmRcvB2 = true;
            break;
        }
        break;

      case MESSAGE_TYPE_ENTANGLE_STOP:  //device A sends to B1 or B2 direct
        debug("stop entanglement received from: ");
        printRole(data.senderRole);
        entangleStopRcv = true;
        break;
    }
  }

  _imuSensor->update();
  unsigned long currentTime = millis();
  if (currentTime - lastUpdateTime >= FSM_UPDATE_INTERVAL) {
    //add functions called at state update
    printDiceStateName("DiceState", diceStateSelf);
    lastUpdateTime = currentTime;
    (this->*stateFunctions[static_cast<int>(currentState)].whileInState)();
  }

  checkTimeForDeepSleep(_imuSensor);
}

void StateMachine::enterIDLE() {
  debugln("------------ enter IDLE state -------------");
  printDiceStateName2("Curent diceState: ", diceStateSelf);
  printDiceStateName2("Previous diceState: ", prevDiceStateSelf);
  delay(100);

  stateEntryTime = millis();
  stateSelf = currentState;
  diceStateSelf = DiceStates::NONE;
  prevDiceStateSelf = DiceStates::NONE;
  diceNumberSelf = DiceNumbers::NONE;
  upSideSelf = UpSide::NONE;
  measureAxisSelf = MeasuredAxises::UNDEFINED;
  prevMeasureAxisSelf = MeasuredAxises::UNDEFINED;
  prevUpSideSelf = UpSide::NONE;
  sendWatchDog();
  refreshScreens();
};

void StateMachine::whileIDLE() {
  voltageIndicator(X0);
  if (millis() - stateEntryTime > IDLETIME) {
    setInitialState();  //when leaving the IDLE state, set all states
    changeState(Trigger::timed);
  }
}

void StateMachine::enterINITSINGLE() {
  debugln("------------ enter INITSINGLE state -------------");
  printDiceStateName2("Curent diceState: ", diceStateSelf);
  printDiceStateName2("Previous diceState: ", prevDiceStateSelf);
  delay(100);
  stateEntryTime = millis();
  stateSelf = currentState;
  prevDiceStateSelf = diceStateSelf;  //store for the future
  diceStateSelf = DiceStates::SINGLE;
  diceNumberSelf = DiceNumbers::NONE;
  upSideSelf = UpSide::NONE;
  measureAxisSelf = MeasuredAxises::UNDEFINED;
  prevMeasureAxisSelf = MeasuredAxises::UNDEFINED;
  prevUpSideSelf = UpSide::NONE;
  refreshScreens();
  sendWatchDog();

  printDiceStateName2("Curent diceState: ", diceStateSelf);
  printDiceStateName2("Previous diceState: ", prevDiceStateSelf);
  debugln("------------ exit INITSINGLE state -------------");
  delay(100);
};

void StateMachine::whileINITSINGLE() {
  if (checkMinimumVoltage()) {
    changeState(Trigger::lowbattery);
  } else if (millis() - stateEntryTime > SHOWNEWSTATETIME) {
    changeState(Trigger::timed);
  }
}

void StateMachine::enterINITENTANGLED_AB1() {
  debugln("------------ enter INITENTANGLED_AB1 state -------------");
  printDiceStateName2("Curent diceState: ", diceStateSelf);
  printDiceStateName2("Previous diceState: ", prevDiceStateSelf);
  delay(100);
  stateEntryTime = millis();
  stateSelf = currentState;

  //before changing the entangled states, send the currentstate of B1 to B2
  if (roleSelf == Roles::ROLE_B1) {
    debugln("B1 sends measurement data to B2");
    delay(100);
    sendMeasurements(roleB2, stateSelf, diceStateSelf, diceNumberSelf, upSideSelf, measureAxisSelf);
  }

  //reset all states
  prevDiceStateSelf = diceStateSelf;  //store for the future
  diceStateSelf = DiceStates::ENTANGLED_AB1;
  diceNumberSelf = DiceNumbers::NONE;
  upSideSelf = UpSide::NONE;
  measureAxisSelf = MeasuredAxises::UNDEFINED;
  prevMeasureAxisSelf = MeasuredAxises::UNDEFINED;
  prevUpSideSelf = UpSide::NONE;

  //and send them to the oponent dice
  if (roleSelf == Roles::ROLE_A) {
    debugln("A send measurement to B1");
    sendMeasurements(roleB1, stateSelf, diceStateSelf, diceNumberSelf, upSideSelf, measureAxisSelf);
  } else if (roleSelf == Roles::ROLE_B1) {
    debugln("B1 send measurement to A");
    sendMeasurements(roleA, stateSelf, diceStateSelf, diceNumberSelf, upSideSelf, measureAxisSelf);
  }

  //now kill the other entanglement
  if (prevDiceStateSelf == DiceStates::ENTANGLED_AB2) {  //only for A
    //and stop the entanglement AB2 by A
    if (roleSelf == roleA) {
      sendStopEntanglement(roleB2);
    }
  }

  refreshScreens();
  sendWatchDog();

  printDiceStateName2("Curent diceState: ", diceStateSelf);
  printDiceStateName2("Previous diceState: ", prevDiceStateSelf);
  debugln("------------ exit INITENTANGLED_AB1 state -------------");
  delay(100);
}

void StateMachine::whileINITENTANGLED_AB1() {
  if (checkMinimumVoltage()) {
    changeState(Trigger::lowbattery);
  } else if (millis() - stateEntryTime > SHOWNEWSTATETIME) {
    changeState(Trigger::timed);
  }
}

void StateMachine::enterINITENTANGLED_AB2() {
  debugln("------------ enter INITENTANGLED_AB2 state -------------");
  printDiceStateName2("Curent diceState: ", diceStateSelf);
  printDiceStateName2("Previous diceState: ", prevDiceStateSelf);
  delay(100);
  stateEntryTime = millis();
  stateSelf = currentState;
  delay(100);

  //before changing the entangled states, send the currentstate of B1 to B2
  if (roleSelf == Roles::ROLE_B2) {
    debugln("B2 sends measurement data to B1");
    delay(100);
    sendMeasurements(roleB1, stateSelf, diceStateSelf, diceNumberSelf, upSideSelf, measureAxisSelf);
  }
  //set all states
  prevDiceStateSelf = diceStateSelf;  //store for the future
  diceStateSelf = DiceStates::ENTANGLED_AB2;
  diceNumberSelf = DiceNumbers::NONE;
  upSideSelf = UpSide::NONE;
  measureAxisSelf = MeasuredAxises::UNDEFINED;
  prevMeasureAxisSelf = MeasuredAxises::UNDEFINED;
  prevUpSideSelf = UpSide::NONE;

  //and send them to the oponent dice
  if (roleSelf == Roles::ROLE_A) {
    debugln("A send measurement to B2");
    sendMeasurements(roleB2, stateSelf, diceStateSelf, diceNumberSelf, upSideSelf, measureAxisSelf);
    delay(100);
  } else if (roleSelf == Roles::ROLE_B2) {
    debugln("B2 send measurement to A");
    sendMeasurements(roleA, stateSelf, diceStateSelf, diceNumberSelf, upSideSelf, measureAxisSelf);
    delay(100);
  }
  //now kill the other entanglement
  if (prevDiceStateSelf == DiceStates::ENTANGLED_AB1) {  //only for A
    //and stop the entanglement AB2 by A
    if (roleSelf == roleA) {
      sendStopEntanglement(roleB1);
    }
  }

  refreshScreens();
  sendWatchDog();

  printDiceStateName2("Curent diceState: ", diceStateSelf);
  printDiceStateName2("Previous diceState: ", prevDiceStateSelf);
  debugln("------------ exit INITENTANGLED_AB2 state -------------");
  delay(100);
}

void StateMachine::whileINITENTANGLED_AB2() {
  if (checkMinimumVoltage()) {
    changeState(Trigger::lowbattery);
  } else if (millis() - stateEntryTime > SHOWNEWSTATETIME) {
    changeState(Trigger::timed);
  }
}

void StateMachine::enterINITSINGLE_AFTER_ENT() {
  debugln("------------ enter INITSINGLE_AFTER_ENT state -------------");
  printDiceStateName2("Curent diceState: ", diceStateSelf);
  printDiceStateName2("Previous diceState: ", prevDiceStateSelf);
  delay(100);
  stateEntryTime = millis();
  stateSelf = currentState;
  debugln("entered initSingle after entanglement");
  prevDiceStateSelf = diceStateSelf;  //store for the future
  diceStateSelf = DiceStates::MEASURED_AFTER_ENT;
  diceNumberSelf = DiceNumbers::NONE;
  upSideSelf = UpSide::NONE;
  measureAxisSelf = MeasuredAxises::UNDEFINED;
  prevMeasureAxisSelf = MeasuredAxises::UNDEFINED;
  prevUpSideSelf = UpSide::NONE;
  refreshScreens();
  sendWatchDog();

  printDiceStateName2("Curent diceState: ", diceStateSelf);
  printDiceStateName2("Previous diceState: ", prevDiceStateSelf);
  debugln("------------ exit INITSINGLE_AFTER_ENT state -------------");
  delay(100);
}

void StateMachine::whileINITSINGLE_AFTER_ENT() {
  if (checkMinimumVoltage()) {
    changeState(Trigger::lowbattery);
  } else if (millis() - stateEntryTime > SHOWNEWSTATETIME) {
    changeState(Trigger::timed);
  }
}

void StateMachine::enterWAITFORTHROW() {
  debugln("------------ enter WAIT FOR THROW state -------------");
  printDiceStateName2("Curent diceState: ", diceStateSelf);
  printDiceStateName2("Previous diceState: ", prevDiceStateSelf);
  delay(100);
  stateEntryTime = millis();
  stateSelf = currentState;     //all other states are unchanged.
  _imuSensor->reset();          //prepare for tumbling
  longclicked = false;          //prepare for button use
  entangleRequestRcvA = false;  //prepare for
  entangleConfirmRcvB1 = false;
  entangleConfirmRcvB2 = false;
  if (diceStateSelf != DiceStates::MEASURED) {  //no refresh in measured state, because this is done in INITMEASURED
    refreshScreens();
  }
  sendWatchDog();
};

void StateMachine::whileWAITFORTHROW() {
  static unsigned long lastBroadcastTimeB1 = 0;
  static unsigned long lastBroadcastTimeB2 = 0;

  if (checkMinimumVoltage()) {
    changeState(Trigger::lowbattery);
  } else if (longclicked) {
    longclicked = false;
    changeState(Trigger::buttonPressed);
  } else if (_imuSensor->tumbled(currentConfig.tumbleConstant)) {  // Use tumble constant from config
    changeState(Trigger::startRolling);
  } else if (entangleStopRcv) {  //quit the entanglement
    entangleStopRcv = false;
    changeState(Trigger::entangleStopReceived);
  }

  //diceA initiates entanglement request, only to dice not entangled to
  if (roleSelf == Roles::ROLE_A) {
    if (diceStateSelf != DiceStates::ENTANGLED_AB1) {  //SINGLE or ENTANGLED_AB2
      // Broadcast pairing request periodically
      if (millis() - lastBroadcastTimeB1 > 500) {
        sendEntangleRequest(roleB1);
        lastBroadcastTimeB1 = millis();
      }
      if (entangleConfirmRcvB1) {
        entangleConfirmRcvB1 = false;
        changeState(Trigger::closeByAB1);
      }
    }

    if (diceStateSelf != DiceStates::ENTANGLED_AB2) {  //SINGLE or ENTANGLED_AB1
      // Broadcast pairing request periodically
      if (millis() - lastBroadcastTimeB2 > 500) {
        sendEntangleRequest(roleB2);
        lastBroadcastTimeB2 = millis();
      }
      if (entangleConfirmRcvB2) {
        entangleConfirmRcvB2 = false;
        changeState(Trigger::closeByAB2);
      }
    }

    // and dice B1 and B2 respond to that
  } else if (roleSelf == Roles::ROLE_B1) {
    if (entangleRequestRcvA && EspNowSensor<message>::IsCloseBy()) {
      sendEntanglementConfirm(Roles::ROLE_A);
      entangleRequestRcvA = false;
      changeState(Trigger::closeByAB1);
    }
  } else if (roleSelf == Roles::ROLE_B2) {
    if (entangleRequestRcvA && EspNowSensor<message>::IsCloseBy()) {
      sendEntanglementConfirm(Roles::ROLE_A);
      entangleRequestRcvA = false;
      changeState(Trigger::closeByAB2);
    }
  }

  //timeout of entangled state. No rolling during a period, so return to initSingle state
  if ((diceStateSelf == DiceStates::ENTANGLED_AB1 || diceStateSelf == DiceStates::ENTANGLED_AB2) && (millis() - stateEntryTime > MAXENTANGLEDWAITTIME)) {  //return to initSingle state
    changeState(Trigger::timed);
  }

  //when in entangled state and the sister or brother dice sends the measurement, then change state of the dice to UN_ENTANGLED_AB1/2 and refresh screens
  if (measurementReceived) {
    measurementReceived = false;
    prevDiceStateSelf = diceStateSelf;  //store for the future
    if (diceStateSelf == DiceStates::ENTANGLED_AB1) {
      diceStateSelf = DiceStates::UN_ENTANGLED_AB1;
    }
    else if (diceStateSelf == DiceStates::ENTANGLED_AB2) {
      diceStateSelf = DiceStates::UN_ENTANGLED_AB2;
    }
    refreshScreens();  //update screens ackordingly
  }
}

void StateMachine::enterTHROWING() {
  debugln("------------ enter THROWING state -------------");
  stateEntryTime = millis();
  stateSelf = currentState;
  refreshScreens();
  sendWatchDog();
};

void StateMachine::whileTHROWING() {
  if (checkMinimumVoltage()) {
    changeState(Trigger::lowbattery);
  } else if (_imuSensor->isNotMoving() && (withinBounds(abs(_imuSensor->getXGravity()), LOWERBOUND, UPPERBOUND) || withinBounds(abs(_imuSensor->getYGravity()), LOWERBOUND, UPPERBOUND) || withinBounds(abs(_imuSensor->getZGravity()), LOWERBOUND, UPPERBOUND))) {
    debugln("isNotMoving triggered and Gravity values are within range");
    changeState(Trigger::nonMoving);
  }
  //when in entangled state and the sister or brother dice sends the measurement, then change state of the dice to UN_ENTANGLED_AB1/2 and refresh screens
  if (measurementReceived) {
    measurementReceived = false;
    prevDiceStateSelf = diceStateSelf;  //store for the future
    if (diceStateSelf == DiceStates::ENTANGLED_AB1) {
      diceStateSelf = DiceStates::UN_ENTANGLED_AB1;
    }
    else if (diceStateSelf == DiceStates::ENTANGLED_AB2) {
      diceStateSelf = DiceStates::UN_ENTANGLED_AB2;
    }
    refreshScreens();  //update screens ackordingly
  }
};

void StateMachine::enterINITMEASURED() {
  debugln("------------ enter MEASUREMENT state -------------");
  printDiceStateName2("Curent diceState: ", diceStateSelf);
  printDiceStateName2("Previous diceState: ", prevDiceStateSelf);
  delay(100);
  stateEntryTime = millis();
  stateSelf = currentState;
  if (_imuSensor->isMoving()) {
    changeState(Trigger::measurementFail);  //back to throwing state
  }

  debug("gravity XYZ: ");
  debug(_imuSensor->getXGravity());
  debug(", ");
  debug(_imuSensor->getYGravity());
  debug(", ");
  debug(_imuSensor->getZGravity());
  debugln("");

  // Detection algorithm: set measureAxis and which side up?
  // Use isNano from config instead of compile-time define
  if (currentConfig.isNano) {
    // IMU mounted on X+ side (left) - NANO configuration
    if (withinBounds(abs(_imuSensor->getXGravity()), LOWERBOUND, UPPERBOUND)) {
      debugln("measureAxisSelf = MeasuredAxises::ZAXIS;");
      measureAxisSelf = MeasuredAxises::ZAXIS;
      if (_imuSensor->getXGravity() < 0) upSideSelf = UpSide::Z1;
      else upSideSelf = UpSide::Z0;
    } else if (withinBounds(abs(_imuSensor->getYGravity()), LOWERBOUND, UPPERBOUND)) {
      debugln("measureAxisSelf = MeasuredAxises::XAXIS;");
      measureAxisSelf = MeasuredAxises::XAXIS;
      if (_imuSensor->getYGravity() < 0) upSideSelf = UpSide::X0;
      else upSideSelf = UpSide::X1;
    } else if (withinBounds(abs(_imuSensor->getZGravity()), LOWERBOUND, UPPERBOUND)) {
      debugln("measureAxisSelf = MeasuredAxises::YAXIS;");
      measureAxisSelf = MeasuredAxises::YAXIS;
      if (_imuSensor->getZGravity() < 0) upSideSelf = UpSide::Y0;
      else upSideSelf = UpSide::Y1;
    } else {
      debugln("no clear axis");
      changeState(Trigger::measurementFail);  //back to throwing state
    }
  } else {
    // IMU mounted on Y- side (rear) - DEVKIT configuration
    if (withinBounds(abs(_imuSensor->getXGravity()), LOWERBOUND, UPPERBOUND)) {
      debugln("measureAxisSelf = MeasuredAxises::ZAXIS;");
      measureAxisSelf = MeasuredAxises::ZAXIS;
      if (_imuSensor->getXGravity() > 0) upSideSelf = UpSide::Z0;
      else upSideSelf = UpSide::Z1;
    } else if (withinBounds(abs(_imuSensor->getYGravity()), LOWERBOUND, UPPERBOUND)) {
      debugln("measureAxisSelf = MeasuredAxises::YAXIS;");
      measureAxisSelf = MeasuredAxises::YAXIS;
      if (_imuSensor->getYGravity() > 0) upSideSelf = UpSide::Y1;
      else upSideSelf = UpSide::Y0;
    } else if (withinBounds(abs(_imuSensor->getZGravity()), LOWERBOUND, UPPERBOUND)) {
      debugln("measureAxisSelf = MeasuredAxises::XAXIS;");
      measureAxisSelf = MeasuredAxises::XAXIS;
      if (_imuSensor->getZGravity() > 0) upSideSelf = UpSide::X0;
      else upSideSelf = UpSide::X1;
    } else {
      debugln("no clear axis");
      changeState(Trigger::measurementFail);  //back to throwing state
    }
  }

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

  // The secret sauce to set diceNumber on top
  switch (diceStateSelf) {
    case DiceStates::SINGLE:
      debugln("single state secret sauce");
      diceNumberSelf = selectOneToSix();
      break;

    case DiceStates::MEASURED:  // 2 options: same measureAxisSelf or different measureAxis
      debugln("measured state secret sauce");
      if (measureAxisSelf != prevMeasureAxisSelf) {  // different, generate random upNumber
        debugln("measured state. different axis");
        diceNumberSelf = selectOneToSix();
      } else {  // same axis
        debugln("measured state. same axis. do nothing: ");
        // nothing to do - diceNumberSelf is already set and will be put on top
      }
      break;

    case DiceStates::ENTANGLED_AB1
      debugln("entang AB1 secret sauce");
      // Use alwaysSeven from config instead of compile-time define
      if (currentConfig.alwaysSeven) {
        if (measureAxisSister != MeasuredAxises::UNDEFINED) {
          debugln("sister ready. always seven mode");
          diceNumberSelf = selectOppositeOneToSix(diceNumberSister);
        } else {
          debugln("different axis");
          diceNumberSelf = selectOneToSix();
        }
      } else {
        // Normal mode - same axis check
        if (measureAxisSelf == measureAxisSister) {
          debugln("sister ready. same axis");
          diceNumberSelf = selectOppositeOneToSix(diceNumberSister);
        } else {
          debugln("different axis");
          diceNumberSelf = selectOneToSix();
        }
      }
      // Send measurements to the opponent dice
      if (roleSelf == Roles::ROLE_A) {
        sendMeasurements(roleB1, stateSelf, DiceStates::MEASURED, diceNumberSelf, upSideSelf, measureAxisSelf);
      } else if (roleSelf == Roles::ROLE_B1) {
        sendMeasurements(roleA, stateSelf, DiceStates::MEASURED, diceNumberSelf, upSideSelf, measureAxisSelf);
      }
      break;

    case DiceStates::UN_ENTANGLED_AB1
      debugln("entang AB1 secret sauce");
      // Use alwaysSeven from config instead of compile-time define
      if (currentConfig.alwaysSeven) {
        if (measureAxisSister != MeasuredAxises::UNDEFINED) {
          debugln("sister ready. always seven mode");
          diceNumberSelf = selectOppositeOneToSix(diceNumberSister);
        } else {
          debugln("different axis");
          diceNumberSelf = selectOneToSix();
        }
      } else {
        // Normal mode - same axis check
        if (measureAxisSelf == measureAxisSister) {
          debugln("sister ready. same axis");
          diceNumberSelf = selectOppositeOneToSix(diceNumberSister);
        } else {
          debugln("different axis");
          diceNumberSelf = selectOneToSix();
        }
      }
      // Send measurements to the opponent dice
      if (roleSelf == Roles::ROLE_A) {
        sendMeasurements(roleB1, stateSelf, DiceStates::MEASURED, diceNumberSelf, upSideSelf, measureAxisSelf);
      } else if (roleSelf == Roles::ROLE_B1) {
        sendMeasurements(roleA, stateSelf, DiceStates::MEASURED, diceNumberSelf, upSideSelf, measureAxisSelf);
      }
      break;

    case DiceStates::ENTANGLED_AB2:
      debugln("entang AB2 secret sauce");
      // Use alwaysSeven from config instead of compile-time define
      if (currentConfig.alwaysSeven) {
        if (measureAxisSister != MeasuredAxises::UNDEFINED) {
          debugln("sister ready. always seven mode");
          diceNumberSelf = selectOppositeOneToSix(diceNumberSister);
        } else {
          debugln("different axis");
          diceNumberSelf = selectOneToSix();
        }
      } else {
        // Normal mode - same axis check
        if (measureAxisSelf == measureAxisSister) {
          debugln("sister ready. same axis");
          diceNumberSelf = selectOppositeOneToSix(diceNumberSister);
        } else {
          debugln("different axis");
          diceNumberSelf = selectOneToSix();
        }
      }
      // Send measurements to the opponent dice
      if (roleSelf == Roles::ROLE_A) {
        sendMeasurements(roleB2, stateSelf, DiceStates::MEASURED, diceNumberSelf, upSideSelf, measureAxisSelf);
      } else if (roleSelf == Roles::ROLE_B2) {
        sendMeasurements(roleA, stateSelf, DiceStates::MEASURED, diceNumberSelf, upSideSelf, measureAxisSelf);
      }
      break;

   case DiceStates::UN_ENTANGLED_AB2:
      debugln("entang AB2 secret sauce");
      // Use alwaysSeven from config instead of compile-time define
      if (currentConfig.alwaysSeven) {
        if (measureAxisSister != MeasuredAxises::UNDEFINED) {
          debugln("sister ready. always seven mode");
          diceNumberSelf = selectOppositeOneToSix(diceNumberSister);
        } else {
          debugln("different axis");
          diceNumberSelf = selectOneToSix();
        }
      } else {
        // Normal mode - same axis check
        if (measureAxisSelf == measureAxisSister) {
          debugln("sister ready. same axis");
          diceNumberSelf = selectOppositeOneToSix(diceNumberSister);
        } else {
          debugln("different axis");
          diceNumberSelf = selectOneToSix();
        }
      }
      // Send measurements to the opponent dice
      if (roleSelf == Roles::ROLE_A) {
        sendMeasurements(roleB2, stateSelf, DiceStates::MEASURED, diceNumberSelf, upSideSelf, measureAxisSelf);
      } else if (roleSelf == Roles::ROLE_B2) {
        sendMeasurements(roleA, stateSelf, DiceStates::MEASURED, diceNumberSelf, upSideSelf, measureAxisSelf);
      }
      break;      


    case DiceStates::MEASURED_AFTER_ENT:  // 2 options: no sister diceNumber or with diceNumber
      debugln("measured after entang secret sauce");
      if (diceNumberSister == DiceNumbers::NONE) {  // different, generate random upNumber
        debugln("no diceNumberSister");
        diceNumberSelf = selectOneToSix();
      } else {
        debugln("defined diceNumberSister");
        diceNumberSelf = diceNumberSister;
      }
      break;
  }

  prevMeasureAxisSelf = measureAxisSelf;
  prevUpSideSelf = upSideSelf;           // preserve for the history
  prevDiceStateSelf = diceStateSelf;     // store for the future
  diceStateSelf = DiceStates::MEASURED;  // here the final diceState is set to measured

  refreshScreens();  // often redundant, because nothing changed after throwing
  sendWatchDog();
}

void StateMachine::whileINITMEASURED() {
  if (checkMinimumVoltage()) {
    changeState(Trigger::lowbattery);
  }
  if (millis() - stateEntryTime > STABTIME) {
    changeState(Trigger::measureXYZ);
  }
}

void StateMachine::enterLOWBATTERY() {
  stateEntryTime = millis();
  stateSelf = currentState;
  prevDiceStateSelf = diceStateSelf;  // store for the future
  diceStateSelf = DiceStates::NONE;
  measureAxisSelf = MeasuredAxises::UNDEFINED;
  prevMeasureAxisSelf = MeasuredAxises::UNDEFINED;
  diceNumberSelf = DiceNumbers::NONE;
  upSideSelf = UpSide::NONE;
  prevUpSideSelf = UpSide::NONE;
  sendWatchDog();
  refreshScreens();
};

void StateMachine::whileLOWBATTERY() {
  voltageIndicator(XX);
};

void StateMachine::enterCLASSIC_STATE() {
  stateEntryTime = millis();
  longclicked = false;
  stateSelf = currentState;
  diceStateSelf = DiceStates::CLASSIC;
  sendWatchDog();
  refreshScreens();
};

void StateMachine::whileCLASSIC_STATE() {
  if (checkMinimumVoltage()) {
    changeState(Trigger::lowbattery);
  } else if (longclicked) {
    longclicked = false;
    changeState(Trigger::buttonPressed);
  }
};