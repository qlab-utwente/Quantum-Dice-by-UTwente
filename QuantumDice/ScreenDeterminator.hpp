#ifndef SCREENDETERMINATOR_NEW_H_
#define SCREENDETERMINATOR_NEW_H_

#include "defines.hpp"
#include "ScreenStateDefs.hpp"
#include "StateMachine.hpp"

/**
 * Dynamic screen determination based on current state
 * Instead of a hardcoded truth table,
 * we
 * compute what to display
 */

// Helper struct to hold screen states for all 6 faces
struct ScreenConfiguration {
    ScreenStates x0, x1, y0, y1, z0, z1;
};

/**
 * Determine which screens to show based on current dice state
 */
inline auto determineScreens(State state, DiceNumbers diceNumber, UpSide upSide)
  -> ScreenConfiguration {
    ScreenConfiguration config;

    // === CLASSIC MODE ===
    if (state.mode == Mode::CLASSIC) {
        // Classic mode: show fixed numbers 1-6 on each face
        config.x0 = ScreenStates::N2;
        config.x1 = ScreenStates::N5;
        config.y0 = ScreenStates::N3;
        config.y1 = ScreenStates::N4;
        config.z0 = ScreenStates::N6;
        config.z1 = ScreenStates::N1;
        return config;
    }
    // === QUANTUM MODE ===

    // THROWING STATE - show superposition
    if (state.throwState == ThrowState::THROWING) {
        // Show superposition based on entanglement state
        ScreenStates throwingScreen;

        if (state.entanglementState == EntanglementState::ENTANGLED) {
            // Entangled: show special entangled superposition
            throwingScreen = ScreenStates::MIX1TO6_ENTANGLED;
        } else {
            // Pure superposition (including ENTANGLE_REQUESTED)
            throwingScreen = ScreenStates::MIX1TO6;
        }

        config.x0 = config.x1 = config.y0 = config.y1 = config.z0 = config.z1 = throwingScreen;
        return config;
    }

    // IDLE STATE - show superposition (waiting to throw)
    if (state.throwState == ThrowState::IDLE) {
        ScreenStates idleScreen;

        if (state.entanglementState == EntanglementState::ENTANGLED) {
            // Entangled: show entangled superposition
            idleScreen = ScreenStates::MIX1TO6_ENTANGLED;
        } else {
            // Pure quantum: show superposition (including ENTANGLE_REQUESTED)
            idleScreen = ScreenStates::MIX1TO6;
        }

        config.x0 = config.x1 = config.y0 = config.y1 = config.z0 = config.z1 = idleScreen;
        return config;
    }

    // OBSERVED STATE - measurement complete
    if (state.throwState == ThrowState::OBSERVED) {
        // Show the measured value on the top face, superposition on others

        // Determine which screen shows the measured number
        ScreenStates measuredScreen;
        switch (diceNumber) {
            case DiceNumbers::ONE:   measuredScreen = ScreenStates::N1; break;
            case DiceNumbers::TWO:   measuredScreen = ScreenStates::N2; break;
            case DiceNumbers::THREE: measuredScreen = ScreenStates::N3; break;
            case DiceNumbers::FOUR:  measuredScreen = ScreenStates::N4; break;
            case DiceNumbers::FIVE:  measuredScreen = ScreenStates::N5; break;
            case DiceNumbers::SIX:   measuredScreen = ScreenStates::N6; break;
            default:                 measuredScreen = ScreenStates::MIX1TO6; break;
        }

        // Default all to superposition
        ScreenStates defaultScreen = (state.entanglementState == EntanglementState::ENTANGLED)
                                       ? ScreenStates::MIX1TO6_ENTANGLED
                                       : ScreenStates::MIX1TO6;

        config.x0 = config.x1 = config.y0 = config.y1 = config.z0 = config.z1 = defaultScreen;

        // Set the measured face to show the number
        switch (upSide) {
            case UpSide::X0: config.x0 = measuredScreen; break;
            case UpSide::X1: config.x1 = measuredScreen; break;
            case UpSide::Y0: config.y0 = measuredScreen; break;
            case UpSide::Y1: config.y1 = measuredScreen; break;
            case UpSide::Z0: config.z0 = measuredScreen; break;
            case UpSide::Z1: config.z1 = measuredScreen; break;
            default:         break;
        }

        return config;
    }

    // FALLBACK - shouldn't reach here but default to superposition
    errorln("determineScreens: Unhandled state, defaulting to superposition");
    config.x0 = config.x1 = config.y0 = config.y1 = config.z0 = config.z1 = ScreenStates::MIX1TO6;
    return config;
}

/**
 * Main function to refresh all screens based on current state
 * This replaces the old
 *
 * findValues() function
 */
inline auto determineScreenStates(State state, DiceNumbers diceNumber, UpSide upSide,
                                  ScreenStates &x0ScreenState, ScreenStates &x1ScreenState,
                                  ScreenStates &y0ScreenState, ScreenStates &y1ScreenState,
                                  ScreenStates &z0ScreenState, ScreenStates &z1ScreenState)
  -> bool {
    ScreenConfiguration config = determineScreens(state, diceNumber, upSide);

    x0ScreenState = config.x0;
    x1ScreenState = config.x1;
    y0ScreenState = config.y0;
    y1ScreenState = config.y1;
    z0ScreenState = config.z0;
    z1ScreenState = config.z1;

    return true; // Always succeeds
}

#endif // SCREENDETERMINATOR_NEW_H_
