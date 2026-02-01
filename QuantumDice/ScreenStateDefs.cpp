#include "ScreenStateDefs.hpp"

#include "defines.hpp"
#include "handyHelpers.hpp"
#include "ScreenDeterminator.hpp" // Use new dynamic screen determination
#include "Screenfunctions.hpp"

#include <Arduino.h>

State          stateSelf, stateSister;
MeasuredAxises measureAxisSelf, prevMeasureAxisSelf, measureAxisSister;
DiceNumbers    diceNumberSelf, diceNumberSister;
UpSide         upSideSelf, prevUpSideSelf, upSideSister;
uint16_t       entanglement_color_self = 0xFFE0; // Default yellow
uint16_t       prev_entanglement_color = 0xFFE0; // Track previous color for change detection
bool           showColors              = true;   // Toggle for showing entanglement colors
bool           prev_showColors         = true;   // Track previous showColors state
bool           flashColor              = false;  // Whether to flash color briefly
bool           prev_flashColor         = false;  // Track previous flashColor state
unsigned long  flashColorStartTime     = 0;      // When the color flash started
ScreenStates   x0ReqScreenState, x1ReqScreenState, y0ReqScreenState, y1ReqScreenState,
  z0ReqScreenState, z1ReqScreenState;

BlinkStates blinkState;

constexpr uint8_t DICE_MIN = 1;
constexpr uint8_t DICE_MAX = 6;

auto findValues(State state, DiceNumbers diceNumber, UpSide upSide, ScreenStates &x0ScreenState,
                ScreenStates &x1ScreenState, ScreenStates &y0ScreenState,
                ScreenStates &y1ScreenState, ScreenStates &z0ScreenState,
                ScreenStates &z1ScreenState) -> bool {
    // Use new dynamic screen determination instead of truth table
    return determineScreenStates(stateSelf, diceNumberSelf, upSideSelf, x0ScreenState,
                                 x1ScreenState, y0ScreenState, y1ScreenState, z0ScreenState,
                                 z1ScreenState);
}

static void callFunction(ScreenStates result, screenselections screens) {
    switch (result) {
        case ScreenStates::GODDICE:
            debugln("Entering GODDICE case");
            displayEinstein(screens);
            return;
        case ScreenStates::WELCOME:
            debugln("Entering WELCOME case");
            welcomeInfo(screens);
            return;
        case ScreenStates::N1:
            debugln("Entering N1 case");
            displayN1(screens);
            return;
        case ScreenStates::N2:
            debugln("Entering N2 case");
            displayN2(screens);
            return;
        case ScreenStates::N3:
            debugln("Entering N3 case");
            displayN3(screens);
            return;
        case ScreenStates::N4:
            debugln("Entering N4 case");
            displayN4(screens);
            return;
        case ScreenStates::N5:
            debugln("Entering N5 case");
            displayN5(screens);
            return;
        case ScreenStates::N6:
            debugln("Entering N6 case");
            displayN6(screens);
            return;
        case ScreenStates::MIX1TO6:
            debugln("Entering MIX1TO6 case");
            displayMix1to6(screens);
            return;
        case ScreenStates::MIX1TO6_ENTANGLED:
            debugln("Entering MIX1TO6_ENTANGLED case");
            displayMix1to6_entangled(screens);
            return;
        case ScreenStates::LOWBATTERY:
            debugln("Entering LOWBATTERY case");
            displayLowBattery(screens);
            return;
        case ScreenStates::BLANC:
            debugln("Entering BLANC case");
            blankScreen(screens);
            return;
        case ScreenStates::DIAGNOSE:
            debugln("Entering DIAGNOSE case");
            voltageIndicator(screens);
            return;
        case ScreenStates::RESET:
            debugln("Entering RESET case");
            displayNewDie(screens);
            return;
        case ScreenStates::QLAB_LOGO:
            debugln("Entering QLAB_LOGO case");
            displayQLab(screens);
            return;
        case ScreenStates::UT_LOGO:
            debugln("Entering UT_LOGO case");
            displayUTlogo(screens);
            return;
        case ScreenStates::QRCODE:
            debugln("Entering QRCODE case");
            displayQRcode(screens);
            return;
        default: warnln("No valid screen state selected"); return;
    }
}

void checkAndCallFunctions(ScreenStates x0, ScreenStates x1, ScreenStates y0, ScreenStates y1,
                           ScreenStates z0, ScreenStates z1) {
    static ScreenStates prevX0 = ScreenStates::BLANC;
    static ScreenStates prevY0 = ScreenStates::BLANC;
    static ScreenStates prevZ0 = ScreenStates::BLANC;
    static ScreenStates prevX1 = ScreenStates::BLANC;
    static ScreenStates prevY1 = ScreenStates::BLANC;
    static ScreenStates prevZ1 = ScreenStates::BLANC;

    // Check if color changed - if so, force refresh of all entangled screens
    bool colorChanged      = (entanglement_color_self != prev_entanglement_color);
    bool showColorsChanged = (showColors != prev_showColors);
    bool flashColorChanged = (flashColor != prev_flashColor);

    if (colorChanged || showColorsChanged || flashColorChanged) {
        if (showColorsChanged) {
            debugf("Show colors toggled from %s to %s - forcing screen refresh\n",
                   prev_showColors ? "ON" : "OFF", showColors ? "ON" : "OFF");
            prev_showColors = showColors;
        }
        if (colorChanged) {
            debugf("Entanglement color changed from 0x%04X to 0x%04X - forcing screen refresh\n",
                   prev_entanglement_color, entanglement_color_self);
            prev_entanglement_color = entanglement_color_self;
        }
        // Force refresh of any entangled screens by resetting their prev state
        if (x0 == ScreenStates::MIX1TO6_ENTANGLED) {
            prevX0 = ScreenStates::BLANC;
        }
        if (y0 == ScreenStates::MIX1TO6_ENTANGLED) {
            prevY0 = ScreenStates::BLANC;
        }
        if (z0 == ScreenStates::MIX1TO6_ENTANGLED) {
            prevZ0 = ScreenStates::BLANC;
        }
        if (x1 == ScreenStates::MIX1TO6_ENTANGLED) {
            prevX1 = ScreenStates::BLANC;
        }
        if (y1 == ScreenStates::MIX1TO6_ENTANGLED) {
            prevY1 = ScreenStates::BLANC;
        }
        if (z1 == ScreenStates::MIX1TO6_ENTANGLED) {
            prevZ1 = ScreenStates::BLANC;
        }

        if (flashColorChanged) {
            debugf("Flash color toggled from %s to %s - forcing screen refresh\n",
                   prev_flashColor ? "ON" : "OFF", flashColor ? "ON" : "OFF");
            prev_flashColor = flashColor;

            if (x0 == ScreenStates::MIX1TO6_ENTANGLED) {
                prevX0 = ScreenStates::BLANC;
            }
            if (y0 == ScreenStates::MIX1TO6_ENTANGLED) {
                prevY0 = ScreenStates::BLANC;
            }
            if (z0 == ScreenStates::MIX1TO6_ENTANGLED) {
                prevZ0 = ScreenStates::BLANC;
            }
            if (x1 == ScreenStates::MIX1TO6_ENTANGLED) {
                prevX1 = ScreenStates::BLANC;
            }
            if (y1 == ScreenStates::MIX1TO6_ENTANGLED) {
                prevY1 = ScreenStates::BLANC;
            }
            if (z1 == ScreenStates::MIX1TO6_ENTANGLED) {
                prevZ1 = ScreenStates::BLANC;
            }

            if (!flashColor) { // Flash ended, refresh to normal color
                debugln("Flash ended - refreshing entangled screens to normal color");
                x0 = ScreenStates::MIX1TO6;
                y0 = ScreenStates::MIX1TO6;
                z0 = ScreenStates::MIX1TO6;
                x1 = ScreenStates::MIX1TO6;
                y1 = ScreenStates::MIX1TO6;
                z1 = ScreenStates::MIX1TO6;
            }
        }
    }

    if (x0 != prevX0) {
        // debug("X0:");
        callFunction(x0, screenselections::X0);
        prevX0 = x0;
    }
    if (y0 != prevY0) {
        // debug("Y0:");
        callFunction(y0, screenselections::Y0);
        prevY0 = y0;
    }
    if (z0 != prevZ0) {
        // debug("Z0:");
        callFunction(z0, screenselections::Z0);
        prevZ0 = z0;
    }
    if (x1 != prevX1) {
        // debug("X1:");
        callFunction(x1, screenselections::X1);
        prevX1 = x1;
    }
    if (y1 != prevY1) {
        // debug("Y1:");
        callFunction(y1, screenselections::Y1);
        prevY1 = y1;
    }
    if (z1 != prevZ1) {
        // debug("Z1:");
        callFunction(z1, screenselections::Z1);
        prevZ1 = z1;
    }
}

void refreshScreens() {
    // set screens, depending of states
    if (findValues(stateSelf, diceNumberSelf, upSideSelf, x0ReqScreenState, x1ReqScreenState,
                   y0ReqScreenState, y1ReqScreenState, z0ReqScreenState, z1ReqScreenState)) {
        checkAndCallFunctions(x0ReqScreenState, x1ReqScreenState, y0ReqScreenState,
                              y1ReqScreenState, z0ReqScreenState, z1ReqScreenState);
    } else {
        debugln("no match found");
        // No match found, handle accordingly
    }
}

auto selectOneToSix() -> DiceNumbers {
    uint8_t randomNumber = generateDiceRoll();
    debugf("Select one to six. Randomnumber: %d\n", randomNumber);
    if (randomNumber >= DICE_MIN && randomNumber <= DICE_MAX) {
        return static_cast<DiceNumbers>(randomNumber);
    }
    warnln("Random number out of range, returning ONE");
    return DiceNumbers::ONE;
}

auto selectOppositeOneToSix(DiceNumbers diceNumberTop) -> DiceNumbers {
    if (diceNumberTop == DiceNumbers::NONE) {
        warnln("selectOppositeOneToSix called with NONE, returning ONE");
        return DiceNumbers::ONE;
    }

    if (diceNumberTop < DiceNumbers::ONE || diceNumberTop > DiceNumbers::SIX) {
        warnln("selectOppositeOneToSix called with invalid number, returning ONE");
        return DiceNumbers::ONE;
    }

    const int oppositeSum    = 7; // In a standard die, opposite sides sum to 7
    const int oppositeNumber = oppositeSum - static_cast<int>(diceNumberTop);

    debugf("Opposite number of %d is %d\n", static_cast<int>(diceNumberTop), oppositeNumber);
    return static_cast<DiceNumbers>(oppositeNumber);
}
