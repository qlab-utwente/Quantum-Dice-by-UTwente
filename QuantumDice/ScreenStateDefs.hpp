#include <cstdint>
#ifndef SCREENSTATEDEFS_H_
#define SCREENSTATEDEFS_H_

#include "StateMachine.hpp"

// Global state variables for screen display
extern State          stateSelf, stateSister;
extern uint16_t       entanglement_color_self; // RGB565 color for current entanglement
extern bool           showColors;              // Toggle to show/hide entanglement colors
extern bool           flashColor;              // Whether to flash color briefly
extern unsigned long  flashColorStartTime;     // When the color flash started

// Dice numbers (1-6)
enum class DiceNumbers : uint8_t {
    NONE,
    ONE,
    TWO,
    THREE,
    FOUR,
    FIVE,
    SIX
};
extern DiceNumbers diceNumberSelf, diceNumberSister;

enum class MeasuredAxises : uint8_t {
    UNDEFINED,
    XAXIS,
    YAXIS,
    ZAXIS,
    ALL,
    NA // not applicable
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

enum class BlinkStates : uint8_t {
    OFF,
    ON
};
extern BlinkStates blinkState;

// Function prototypes
auto findValues(State state, DiceNumbers diceNumber, UpSide upSide, ScreenStates &x0ScreenState,
                ScreenStates &x1ScreenState, ScreenStates &y0ScreenState,
                ScreenStates &y1ScreenState, ScreenStates &z0ScreenState,
                ScreenStates &z1ScreenState) -> bool;

// void printValues(ScreenStates x, ScreenStates y, ScreenStates z);

// const char *toString(ScreenStates value);

void callFunction(ScreenStates result);

void checkAndCallFunctions(ScreenStates x0, ScreenStates x1, ScreenStates y0, ScreenStates y1,
                           ScreenStates z0, ScreenStates z1);
void refreshScreens();
auto selectOneToSix() -> DiceNumbers;
auto selectOppositeOneToSix(DiceNumbers diceNumberTop) -> DiceNumbers;
void printDiceStateName(const char *objectName, DiceStates diceState);

#endif /* SCREENSTATEDEFS_H_ */
