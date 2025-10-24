#ifndef STATEMACHINE_H
#define STATEMACHINE_H

#include <Arduino.h>
#include "IMUhelpers.h"
#include "Globals.h"

//#include "EntangStateMachine.h"

#define FSM_UPDATE_INTERVAL 0  // Update interval in milliseconds

enum class DiceStates : uint8_t;
enum class DiceNumbers : uint8_t;
enum class MeasuredAxises : uint8_t;
enum class UpSide : uint8_t;

class StateMachine;  // Forward declaration

#define IDLETIME 3000                //5000 ms-en
#define SHOWNEWSTATETIME 1000        //ms-en to show when new state is initated
#define MAXENTANGLEDWAITTIME 120000  //ms-en wait for throw in entangled wait, befor return to intitSingle state
#define STABTIME 800                 //ms-en to stabilize after measurement
//#define WAITTOTHROW 1000            //minumum time it stays in wait to trow

enum class State {
  IDLE,
  INITSINGLE,
  INITENTANGLED_AB1,
  WAITFORTHROW,
  THROWING,
  INITMEASURED,
  LOWBATTERY,
  CLASSIC_STATE,
  INITENTANGLED_AB2,
  INITSINGLE_AFTER_ENT
};

enum class Trigger {
  onthemove,
  nonMoving,
  startRolling,
  buttonPressed,
  measureXYZ,
  measurementFail,
  closeByAB1,
  entanglementSucces,
  entanglementFail,
  lowbattery,
  timed,
  closeByAB2,
  entangleStopReceived
};

enum class Roles : uint8_t {
  ROLE_A,
  ROLE_B1,
  ROLE_B2,
  NONE
};

struct StateTransition {
  State currentState;
  Trigger trigger;
  State nextState;
};

void setInitialState();
void printStateName(const char *objectName, State state);

class StateMachine {
public:
  StateMachine();
  void begin();  // New function to initialize the state machine
  void changeState(Trigger trigger);
  void update();

  void setImuSensor(IMUSensor *imuSensor) {
    _imuSensor = imuSensor;
  }

  //customized functions
  void enterIDLE();
  void whileIDLE();
  void enterINITSINGLE();
  void whileINITSINGLE();
  void enterINITENTANGLED_AB1();
  void whileINITENTANGLED_AB1();
  void enterWAITFORTHROW();
  void whileWAITFORTHROW();
  void enterTHROWING();
  void whileTHROWING();
  void enterINITMEASURED();
  void whileINITMEASURED();
  void enterLOWBATTERY();
  void whileLOWBATTERY();
  void enterCLASSIC_STATE();
  void whileCLASSIC_STATE();
  void enterINITENTANGLED_AB2();
  void whileINITENTANGLED_AB2();
  void enterINITSINGLE_AFTER_ENT();
  void whileINITSINGLE_AFTER_ENT();

private:
  void determineRoles();
  void sendWatchDog();
  void sendMeasurements(Roles targetRole, State state, DiceStates diceState, DiceNumbers diceNumber, UpSide upSide, MeasuredAxises measureAxis);
  void sendEntangleRequest(Roles targetRole);
  void sendEntanglementConfirm(Roles targetRole);
  void sendStopEntanglement(Roles targetRole);

private:
  IMUSensor *_imuSensor;
  State currentState;

  Roles roleSelf, roleA, roleB1, roleB2, roleBrother, roleSister;

  unsigned long stateEntryTime;

  //EntangStateMachine entangStateMachine;

  struct StateFunctions {
    void (StateMachine::*onEntry)();
    void (StateMachine::*whileInState)();
  };

  static const StateFunctions stateFunctions[];
  static const StateTransition stateTransitions[];

  bool entangleRequestRcvA;
  bool entangleConfirmRcvB1;
  bool entangleConfirmRcvB2;
  bool entangleStopRcv;
  bool measurementReceived;

  //void printState(State state);
};

#endif  // STATEMACHINE_H