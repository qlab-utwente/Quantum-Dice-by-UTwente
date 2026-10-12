#ifndef SCREENSTATEDEFS_H_
#define SCREENSTATEDEFS_H_

#include "StateMachine.hpp"
#include <cstdint>

enum class ScreenStates : uint8_t {
    // Splash screens
    GODDICE,
    WELCOME,
    QLAB_LOGO,
    QRCODE,
    UT_LOGO,

    // Number displays (1-6)
    N1,
    N2,
    N3,
    N4,
    N5,
    N6,

    // Quantum superposition states
    MIX1TO6,           // Normal quantum superposition
    MIX1TO6_ENTANGLED, // Entangled superposition (different color)

    // Special states
    LOWBATTERY,
    BLANC, // Blank screen
    DIAGNOSE,
    RESET
};
extern ScreenStates x0ReqScreenState, x1ReqScreenState, y0ReqScreenState, y1ReqScreenState,
  z0ReqScreenState, z1ReqScreenState;

void callFunction(ScreenStates result);
void checkAndCallFunctions(ScreenStates x0, ScreenStates x1, ScreenStates y0, ScreenStates y1,
                           ScreenStates z0, ScreenStates z1);
void refreshScreens();
auto selectOppositeOneToSix(DiceNumbers diceNumberTop) -> DiceNumbers;
void printDiceStateName(const char *objectName, DiceStates diceState);

#endif /* SCREENSTATEDEFS_H_ */
