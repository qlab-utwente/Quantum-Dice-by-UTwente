#ifndef STATEMACHINE_H
#define STATEMACHINE_H

#include <Arduino.h>
#include "IMUhelpers.h"
//#include "EntangStateMachine.h"

#define FSM_UPDATE_INTERVAL 0  // Update interval in milliseconds

class StateMachine;  // Forward declaration

#define IDLETIME 5000               //ms-en
#define SHOWNEWSTATETIME 3000       //ms-en to show when new state is initated
#define MAXENTANGLEDWAITTIME 20000  //ms-en wait for throw in entangled wait, befor return to intitSingle state
#define SHOWTIME 2000               //ms-en in state showresults
//#define WAITTOTHROW 1000            //minumum time it stays in wait to trow

enum class State{
IDLE,
SHOW,
MEASURE,
MOVING,
LOWBATTERY
};

enum class Trigger{
onthemove,
nonMoving,
startRolling,
buttonPressed,
measureXYZ,
measurementFail,
lowbattery,
timed
};

struct StateTransition {
  State currentState;
  Trigger trigger;
  State nextState;
};

void setInitialState();
void printStateName(const char* objectName, State state);

class StateMachine {
public:
  StateMachine();
  void begin();  // New function to initialize the state machine
  void changeState(Trigger trigger);
  void update();

    void setImuSensor(IMUSensor *imuSensor)
  {
    _imuSensor = imuSensor;
  }

//customized functions
void enterIDLE();
void whileIDLE();
void enterSHOW();
void whileSHOW();
void enterMEASURE();
void whileMEASURE();
void enterMOVING();
void whileMOVING();
void enterLOWBATTERY();
void whileLOWBATTERY();

private:
  IMUSensor *_imuSensor;
  State currentState;
  unsigned long stateEntryTime;

  //EntangStateMachine entangStateMachine;

  struct StateFunctions {
    void (StateMachine::*onEntry)();
    void (StateMachine::*whileInState)();
  };

  static const StateFunctions stateFunctions[];
  static const StateTransition stateTransitions[];

  //void printState(State state);
};

#endif  // STATEMACHINE_H