#include "StateMachine.hpp"

#include "defines.hpp"
#include "DiceConfigManager.hpp"
#include "EspNowSensor.hpp"
#include "handyHelpers.hpp"
#include "IMUhelpers.hpp"
#include "Screenfunctions.hpp"
#include "ScreenStateDefs.hpp"

#include <map>

using message_type = enum message_type : uint8_t {
    MESSAGE_TYPE_WATCH_DOG,
    MESSAGE_TYPE_MEASUREMENT,
    MESSAGE_TYPE_ENTANGLE_REQUEST,
    MESSAGE_TYPE_ENTANGLE_CONFIRM,
    MESSAGE_TYPE_ENTANGLE_DENIED,
    MESSAGE_TYPE_TELEPORT_REQUEST,
    MESSAGE_TYPE_TELEPORT_CONFIRM,
    MESSAGE_TYPE_TELEPORT_PAYLOAD,
    MESSAGE_TYPE_TELEPORT_PARTNER
};

using message = struct message {
    message_type type;

    union _data {
        struct _watchDogData {
            State state;
        } watchDog;

        struct _measurementData {
            State          state;
            MeasuredAxises measureAxis;
            DiceNumbers    diceNumber;
            UpSide         upSide;
        } measurement;

        struct _entangleConfirmData {
            uint16_t color; // RGB565 color for this entanglement
        } entangleConfirm;

        struct _teleportRequestData {
            uint8_t target_dice[6]; // MAC address of dice B (the target for teleportation)
        } teleportRequest;

        struct _teleportPayloadData {
            State          state;             // State of dice M to be transferred to B
            MeasuredAxises measureAxis;       // Measurement axis if observed
            DiceNumbers    diceNumber;        // Dice number if observed
            UpSide         upSide;            // Up side if observed
            uint8_t        entangled_peer[6]; // MAC of N if M is entangled to N
            uint16_t       color;             // Entanglement color (RGB565)
        } teleportPayload;

        struct _teleportPartnerData {
            uint8_t new_partner[6]; // MAC address of B (the new partner for N)
        } teleportPartner;
    } data;
};

static uint8_t last_source[6];
static int32_t last_rssi = INT32_MIN;

// State function mappings for the quantum dice system
// Maps each state combination to its enter and while functions
const std::map<State, StateMachine::StateFunction> StateMachine::stateFunctions = {
  // === CLASSIC MODE ===
  {State{Mode::CLASSIC, ThrowState::IDLE, EntanglementState::PURE},
   {&StateMachine::enterClassicIdle, &StateMachine::whileClassicIdle}},

  // === QUANTUM MODE - IDLE ===
  // Pure quantum state (no entanglement)
  {State{Mode::QUANTUM, ThrowState::IDLE, EntanglementState::PURE},
   {&StateMachine::enterQuantumIdle, &StateMachine::whileQuantumIdle}},

  // Entanglement requested (waiting for partner confirmation)
  {State{Mode::QUANTUM, ThrowState::IDLE, EntanglementState::ENTANGLE_REQUESTED},
   {&StateMachine::enterQuantumIdle, &StateMachine::whileQuantumIdle}},

  // Entangled with partner
  {State{Mode::QUANTUM, ThrowState::IDLE, EntanglementState::ENTANGLED},
   {&StateMachine::enterQuantumIdle, &StateMachine::whileQuantumIdle}},

  // Post-entanglement (partner has measured, waiting for our measurement)
  {State{Mode::QUANTUM, ThrowState::IDLE, EntanglementState::POST_ENTANGLEMENT},
   {&StateMachine::enterQuantumIdle, &StateMachine::whileQuantumIdle}},

  // Teleported state (received teleported measurement, waiting for our measurement)
  {State{Mode::QUANTUM, ThrowState::IDLE, EntanglementState::TELEPORTED},
   {&StateMachine::enterQuantumIdle, &StateMachine::whileQuantumIdle}},

  // === QUANTUM MODE - THROWING ===
  {State{Mode::QUANTUM, ThrowState::THROWING, EntanglementState::PURE},
   {&StateMachine::enterThrowing, &StateMachine::whileThrowing}      },
  {State{Mode::QUANTUM, ThrowState::THROWING, EntanglementState::ENTANGLE_REQUESTED},
   {&StateMachine::enterThrowing, &StateMachine::whileThrowing}      },
  {State{Mode::QUANTUM, ThrowState::THROWING, EntanglementState::ENTANGLED},
   {&StateMachine::enterThrowing, &StateMachine::whileThrowing}      },
  {State{Mode::QUANTUM, ThrowState::THROWING, EntanglementState::POST_ENTANGLEMENT},
   {&StateMachine::enterThrowing, &StateMachine::whileThrowing}      },
  {State{Mode::QUANTUM, ThrowState::THROWING, EntanglementState::TELEPORTED},
   {&StateMachine::enterThrowing, &StateMachine::whileThrowing}      },

  // === QUANTUM MODE - OBSERVED ===
  {State{Mode::QUANTUM, ThrowState::OBSERVED, EntanglementState::PURE},
   {&StateMachine::enterObserved, &StateMachine::whileObserved}      },
  {State{Mode::QUANTUM, ThrowState::OBSERVED, EntanglementState::ENTANGLE_REQUESTED},
   {&StateMachine::enterObserved, &StateMachine::whileObserved}      },
  {State{Mode::QUANTUM, ThrowState::OBSERVED, EntanglementState::ENTANGLED},
   {&StateMachine::enterObserved, &StateMachine::whileObserved}      },
  {State{Mode::QUANTUM, ThrowState::OBSERVED, EntanglementState::POST_ENTANGLEMENT},
   {&StateMachine::enterObserved, &StateMachine::whileObserved}      },
  {State{Mode::QUANTUM, ThrowState::OBSERVED, EntanglementState::TELEPORTED},
   {&StateMachine::enterObserved, &StateMachine::whileObserved}      },
};

namespace {
    inline auto getModeName(Mode mode) -> char * {
        switch (mode) {
            case Mode::CLASSIC: return "CLASSIC";
            case Mode::QUANTUM: return "QUANTUM";
            default:            return "UNKNOWN";
        }
    }

    inline auto getThrowStateName(ThrowState throwState) -> char * {
        switch (throwState) {
            case ThrowState::IDLE:     return "IDLE";
            case ThrowState::THROWING: return "THROWING";
            case ThrowState::OBSERVED: return "OBSERVED";
            default:                   return "UNKNOWN";
        }
    }

    inline auto getEntanglementStateName(EntanglementState entanglementState) -> char * {
        switch (entanglementState) {
            case EntanglementState::PURE:               return "PURE";
            case EntanglementState::ENTANGLE_REQUESTED: return "ENTANGLE_REQUESTED";
            case EntanglementState::ENTANGLED:          return "ENTANGLED";
            case EntanglementState::POST_ENTANGLEMENT:  return "POST_ENTANGLEMENT";
            case EntanglementState::TELEPORTED:         return "TELEPORTED";
            default:                                    return "UNKNOWN";
        }
    }

    inline auto getStateName(State state) -> char * {
        static char stateName[100];
        (void)snprintf((char *)stateName, sizeof(stateName), "%s | %s | %s",
                       getModeName(state.mode), getThrowStateName(state.throwState),
                       getEntanglementStateName(state.entanglementState));
        return (char *)stateName;
    }
}

void printStateName(const char *objectName, State state) {
    debugf("%s: %s\n", objectName, getStateName(state));
}

void StateMachine::sendWatchDog() {
    message watchDog;
    watchDog.type                = message_type::MESSAGE_TYPE_WATCH_DOG;
    watchDog.data.watchDog.state = stateSelf;
    uint8_t target[6];
    memset((uint8_t *)target, 0xFF, 6);
    EspNowSensor<message>::Send(watchDog, (uint8_t *)target);
}

void StateMachine::sendMeasurements(uint8_t *target, State state, DiceNumbers diceNumber,
                                    UpSide upSide, MeasuredAxises measureAxis) {
    EspNowSensor<message>::AddPeer(target);
    message myData;
    debugln("Send Measurements message initated");
    myData.type                         = message_type::MESSAGE_TYPE_MEASUREMENT;
    myData.data.measurement.state       = state;
    myData.data.measurement.measureAxis = measureAxis;
    myData.data.measurement.diceNumber  = diceNumber;
    myData.data.measurement.upSide      = upSide;
    EspNowSensor<message>::Send(myData, target);
}

void StateMachine::sendEntangleRequest(uint8_t *target) {
    EspNowSensor<message>::AddPeer(target);
    message myData;
    myData.type = message_type::MESSAGE_TYPE_ENTANGLE_REQUEST;
    EspNowSensor<message>::Send(myData, target);
}

void StateMachine::sendEntanglementConfirm(uint8_t *target) {
    EspNowSensor<message>::AddPeer(target);
    debugln("Send entanglement confirm");
    message myData;
    myData.type = message_type::MESSAGE_TYPE_ENTANGLE_CONFIRM;

    // Pick a random color from available colors
    if (currentConfig.entang_colors_count > 0) {
        uint8_t colorIndex                = random(0, currentConfig.entang_colors_count);
        myData.data.entangleConfirm.color = currentConfig.entang_colors[colorIndex];
        debugf("Selected entanglement color: 0x%04X (index %d of %d)\n",
               myData.data.entangleConfirm.color, colorIndex, currentConfig.entang_colors_count);
    } else {
        myData.data.entangleConfirm.color = 0xFFE0; // Default yellow if no colors configured
        debugln("No colors configured, using default yellow");
    }

    this->entanglement_color = myData.data.entangleConfirm.color;
    entanglement_color_self  = this->entanglement_color; // Update global

    // Trigger color flash if showColors is disabled
    if (!showColors) {
        flashColor          = true;
        flashColorStartTime = millis();
        debugln("Triggering color flash (accepting entanglement)");
    }

    EspNowSensor<message>::Send(myData, target);
}

void StateMachine::sendEntangleDenied(uint8_t *target) {
    EspNowSensor<message>::AddPeer(target);
    debugln("Send entangle denied");
    message myData;
    myData.type = message_type::MESSAGE_TYPE_ENTANGLE_DENIED;
    EspNowSensor<message>::Send(myData, target);
}

void StateMachine::sendTeleportRequest(uint8_t *target_m, uint8_t *target_b) {
    EspNowSensor<message>::AddPeer(target_m);
    debugln("Send teleport request");
    message myData;
    myData.type = message_type::MESSAGE_TYPE_TELEPORT_REQUEST;
    memcpy((void *)myData.data.teleportRequest.target_dice, (void *)target_b, 6);
    EspNowSensor<message>::Send(myData, target_m);
}

void StateMachine::sendTeleportConfirm(uint8_t *target) {
    EspNowSensor<message>::AddPeer(target);
    debugln("Send teleport confirm");
    message myData;
    myData.type = message_type::MESSAGE_TYPE_TELEPORT_CONFIRM;
    EspNowSensor<message>::Send(myData, target);
}

void StateMachine::sendTeleportPayload(uint8_t *target, State state, DiceNumbers diceNumber,
                                       UpSide upSide, MeasuredAxises measureAxis,
                                       uint8_t *entangled_peer, uint16_t color) {
    EspNowSensor<message>::AddPeer(target);
    debugf("Send teleport payload with colour 0x%04X\n", color);
    message myData;
    myData.type                             = message_type::MESSAGE_TYPE_TELEPORT_PAYLOAD;
    myData.data.teleportPayload.state       = state;
    myData.data.teleportPayload.measureAxis = measureAxis;
    myData.data.teleportPayload.diceNumber  = diceNumber;
    myData.data.teleportPayload.upSide      = upSide;
    myData.data.teleportPayload.color       = color;
    memcpy((void *)myData.data.teleportPayload.entangled_peer, (void *)entangled_peer, 6);
    EspNowSensor<message>::Send(myData, target);
}

void StateMachine::sendTeleportPartner(uint8_t *target_n, uint8_t *new_partner_b) {
    EspNowSensor<message>::AddPeer(target_n);
    debugln("Send teleport partner update");
    message myData;
    myData.type = message_type::MESSAGE_TYPE_TELEPORT_PARTNER;
    memcpy((void *)myData.data.teleportPartner.new_partner, (void *)new_partner_b, 6);
    EspNowSensor<message>::Send(myData, target_n);
}

void setInitialState() {
    // Initialize measurement-related state
    measureAxisSelf     = MeasuredAxises::UNDEFINED;
    prevMeasureAxisSelf = MeasuredAxises::UNDEFINED;
    diceNumberSelf      = DiceNumbers::NONE;
    upSideSelf          = UpSide::NONE;
    prevUpSideSelf      = UpSide::NONE;
}

// State transitions for the quantum dice system
// Note: This must be accessible by getStateTransition function
const std::array<StateTransition, 37> StateMachine::stateTransitions = {
  {// Order: currentMode, nextMode, currentThrowState, nextThrowState, currentEntanglementState,
   // nextEntanglementState, trigger

   // === CLASSIC MODE TRANSITIONS ===
   StateTransition{Mode::CLASSIC, Mode::QUANTUM, ThrowState::IDLE, ThrowState::IDLE, std::nullopt,
                   EntanglementState::PURE, Trigger::BUTTON_PRESSED},

   // === QUANTUM MODE - IDLE TRANSITIONS ===
   StateTransition{Mode::QUANTUM, std::nullopt, ThrowState::IDLE, ThrowState::THROWING,
                   EntanglementState::PURE, std::nullopt, Trigger::START_ROLLING},
   StateTransition{Mode::QUANTUM, Mode::CLASSIC, std::nullopt, ThrowState::IDLE,
                   EntanglementState::PURE, EntanglementState::PURE, Trigger::BUTTON_PRESSED},
   StateTransition{Mode::QUANTUM, Mode::CLASSIC, std::nullopt, ThrowState::IDLE,
                   EntanglementState::POST_ENTANGLEMENT, EntanglementState::PURE,
                   Trigger::BUTTON_PRESSED},
   StateTransition{Mode::QUANTUM, Mode::CLASSIC, std::nullopt, ThrowState::IDLE,
                   EntanglementState::TELEPORTED, EntanglementState::PURE, Trigger::BUTTON_PRESSED},
   StateTransition{Mode::QUANTUM, std::nullopt, ThrowState::IDLE, std::nullopt,
                   EntanglementState::PURE, EntanglementState::ENTANGLE_REQUESTED,
                   Trigger::CLOSE_BY},
   StateTransition{Mode::QUANTUM, std::nullopt, ThrowState::IDLE, std::nullopt,
                   EntanglementState::POST_ENTANGLEMENT, EntanglementState::ENTANGLE_REQUESTED,
                   Trigger::CLOSE_BY},
   StateTransition{Mode::QUANTUM, std::nullopt, ThrowState::IDLE, std::nullopt,
                   EntanglementState::PURE, EntanglementState::ENTANGLED,
                   Trigger::ENTANGLE_REQUEST},
   StateTransition{Mode::QUANTUM, std::nullopt, ThrowState::IDLE, std::nullopt,
                   EntanglementState::ENTANGLE_REQUESTED, EntanglementState::ENTANGLED,
                   Trigger::ENTANGLE_CONFIRM},
   StateTransition{Mode::QUANTUM, std::nullopt, ThrowState::IDLE, std::nullopt,
                   EntanglementState::ENTANGLE_REQUESTED, EntanglementState::PURE,
                   Trigger::ENTANGLE_STOP},
   StateTransition{Mode::QUANTUM, std::nullopt, ThrowState::IDLE, std::nullopt,
                   EntanglementState::ENTANGLE_REQUESTED, EntanglementState::PURE, Trigger::TIMED},
   StateTransition{Mode::QUANTUM, std::nullopt, ThrowState::IDLE, std::nullopt,
                   EntanglementState::ENTANGLED, EntanglementState::PURE, Trigger::ENTANGLE_STOP},
   StateTransition{Mode::QUANTUM, std::nullopt, ThrowState::IDLE, std::nullopt,
                   EntanglementState::ENTANGLED, EntanglementState::PURE, Trigger::TIMED},
   StateTransition{Mode::QUANTUM, std::nullopt, ThrowState::IDLE, ThrowState::THROWING,
                   EntanglementState::ENTANGLED, std::nullopt, Trigger::START_ROLLING},
   StateTransition{Mode::QUANTUM, std::nullopt, ThrowState::IDLE, ThrowState::THROWING,
                   EntanglementState::POST_ENTANGLEMENT, std::nullopt, Trigger::START_ROLLING},
   StateTransition{Mode::QUANTUM, std::nullopt, ThrowState::IDLE, std::nullopt,
                   EntanglementState::ENTANGLED, EntanglementState::POST_ENTANGLEMENT,
                   Trigger::MEASUREMENT_RECEIVED},
   StateTransition{Mode::QUANTUM, std::nullopt, ThrowState::IDLE, std::nullopt,
                   EntanglementState::POST_ENTANGLEMENT, EntanglementState::ENTANGLED,
                   Trigger::ENTANGLE_REQUEST},
   StateTransition{Mode::QUANTUM, std::nullopt, ThrowState::IDLE, ThrowState::THROWING,
                   EntanglementState::TELEPORTED, std::nullopt, Trigger::START_ROLLING},

   // === TELEPORTATION TRANSITIONS ===
   // M initiates teleport (any state -> PURE after sending payload)
   StateTransition{Mode::QUANTUM, std::nullopt, std::nullopt, ThrowState::IDLE, std::nullopt,
                   EntanglementState::PURE, Trigger::TELEPORT_INITIATED},
   // A confirms teleport (ENTANGLED -> PURE after confirming)
   StateTransition{Mode::QUANTUM, std::nullopt, std::nullopt, ThrowState::IDLE,
                   EntanglementState::ENTANGLED, EntanglementState::PURE,
                   Trigger::TELEPORT_CONFIRMED},
   // B receives teleport from M that was PURE
   StateTransition{Mode::QUANTUM, std::nullopt, std::nullopt, std::nullopt,
                   EntanglementState::ENTANGLED, EntanglementState::PURE,
                   Trigger::TELEPORT_RECEIVED},
   // B receives teleport from M that was ENTANGLED covered by message handler

   // === QUANTUM MODE - THROWING TRANSITIONS ===
   StateTransition{Mode::QUANTUM, std::nullopt, ThrowState::THROWING, ThrowState::OBSERVED,
                   std::nullopt, std::nullopt, Trigger::STOP_ROLLING},
   StateTransition{Mode::QUANTUM, std::nullopt, ThrowState::THROWING, ThrowState::IDLE,
                   EntanglementState::PURE, EntanglementState::ENTANGLE_REQUESTED,
                   Trigger::CLOSE_BY},
   StateTransition{Mode::QUANTUM, std::nullopt, ThrowState::THROWING, ThrowState::IDLE,
                   EntanglementState::PURE, EntanglementState::ENTANGLED,
                   Trigger::ENTANGLE_REQUEST},
   StateTransition{Mode::QUANTUM, std::nullopt, ThrowState::THROWING, ThrowState::IDLE,
                   EntanglementState::ENTANGLE_REQUESTED, EntanglementState::ENTANGLED,
                   Trigger::ENTANGLE_CONFIRM},
   StateTransition{Mode::QUANTUM, std::nullopt, ThrowState::THROWING, std::nullopt,
                   EntanglementState::ENTANGLED, EntanglementState::POST_ENTANGLEMENT,
                   Trigger::MEASUREMENT_RECEIVED},

   // === QUANTUM MODE - OBSERVED TRANSITIONS ===
   StateTransition{Mode::QUANTUM, std::nullopt, ThrowState::OBSERVED, ThrowState::THROWING,
                   EntanglementState::PURE, std::nullopt, Trigger::START_ROLLING},
   StateTransition{Mode::QUANTUM, std::nullopt, ThrowState::OBSERVED, ThrowState::IDLE,
                   EntanglementState::PURE, EntanglementState::ENTANGLE_REQUESTED,
                   Trigger::CLOSE_BY},
   StateTransition{Mode::QUANTUM, std::nullopt, ThrowState::OBSERVED, ThrowState::IDLE,
                   EntanglementState::PURE, EntanglementState::ENTANGLED,
                   Trigger::ENTANGLE_REQUEST},
   StateTransition{Mode::QUANTUM, std::nullopt, ThrowState::OBSERVED, ThrowState::IDLE,
                   EntanglementState::ENTANGLE_REQUESTED, EntanglementState::ENTANGLED,
                   Trigger::ENTANGLE_CONFIRM},
   StateTransition{Mode::QUANTUM, std::nullopt, ThrowState::OBSERVED, ThrowState::THROWING,
                   EntanglementState::ENTANGLED, std::nullopt, Trigger::START_ROLLING},
   StateTransition{Mode::QUANTUM, std::nullopt, ThrowState::OBSERVED, ThrowState::THROWING,
                   EntanglementState::POST_ENTANGLEMENT, EntanglementState::PURE,
                   Trigger::START_ROLLING},
   StateTransition{Mode::QUANTUM, std::nullopt, ThrowState::OBSERVED, ThrowState::THROWING,
                   std::nullopt, std::nullopt, Trigger::MEASURE_FAIL},
   StateTransition{Mode::QUANTUM, std::nullopt, ThrowState::OBSERVED, std::nullopt,
                   EntanglementState::ENTANGLED, EntanglementState::POST_ENTANGLEMENT,
                   Trigger::MEASUREMENT_RECEIVED},
  }
};

auto StateMachine::getStateTransition(State currentState, Trigger trigger) -> StateTransition {
    for (const StateTransition &transition : StateMachine::stateTransitions) {
        bool modeMatch = !transition.currentMode.has_value()
                         || transition.currentMode.value() == currentState.mode;
        bool throwStateMatch = !transition.currentThrowState.has_value()
                               || transition.currentThrowState.value() == currentState.throwState;
        bool entanglementStateMatch
          = !transition.currentEntanglementState.has_value()
            || transition.currentEntanglementState.value() == currentState.entanglementState;

        if (modeMatch && throwStateMatch && entanglementStateMatch
            && transition.trigger == trigger) {
            return transition;
        }
    }
    throw std::runtime_error("No valid state transition found");
}

// declaration of instance
StateMachine::StateMachine()
  : currentState{.mode              = Mode::CLASSIC,
                 .throwState        = ThrowState::IDLE,
                 .entanglementState = EntanglementState::PURE},
    stateEntryTime(0), partnerMeasurementAxis(MeasuredAxises::UNDEFINED),
    partnerDiceNumber(DiceNumbers::NONE), teleportedMeasurementAxis(MeasuredAxises::UNDEFINED),
    teleportedDiceNumber(DiceNumbers::NONE), lastRollBasis(MeasuredAxises::UNDEFINED),
    lastRollNumber(DiceNumbers::NONE) {
    // Constructor does not call onEntry. That's done in StateMachine::begin()
    memset((void *)this->current_peer, 0xFF, 6);
    memset((void *)this->next_peer, 0xFF, 6);
    memset((void *)last_source, 0xFF, 6);
    last_rssi = INT32_MIN;
}

void StateMachine::begin() {
    // Initialize ESP-NOW with device A MAC from config
    EspNowSensor<message>::Init();

    infoln("ESP-NOW initialized successfully!");

    EspNowSensor<message>::PrintMacAddress();

    infoln("StateMachine Begin: Calling onEntry for initial state");
    printStateName("StateMachine", currentState);

    infoln("Displaying voltage indicator for 3 seconds on startup");
    voltageIndicator(ALL);
    sleep(3);
    refreshScreens();

    infoln("StateMachine Begin: Setting initial state");
    // Call the onEntry function for the initial state
    auto it = stateFunctions.find(currentState);
    if (it != stateFunctions.end()) {
        (this->*it->second.onEntry)();
    } else {
        errorln("ERROR: No state function found for initial state!");
    }
}

void StateMachine::changeState(Trigger trigger) {
    // Get the state transition for the current state and trigger
    try {
        StateTransition transition = getStateTransition(currentState, trigger);

        // Create new state based on transition
        State newState = currentState; // Start with current state

        // Apply transitions if specified
        if (transition.nextMode.has_value()) {
            newState.mode = transition.nextMode.value();
        }
        if (transition.nextThrowState.has_value()) {
            newState.throwState = transition.nextThrowState.value();
        }
        if (transition.nextEntanglementState.has_value()) {
            newState.entanglementState = transition.nextEntanglementState.value();
        }

        // Only change if the state actually changed
        if (!(newState == currentState)) {
            currentState = newState;
            printStateName("stateMachine", currentState);

            // Call onEntry function for new state
            auto it = stateFunctions.find(currentState);
            if (it != stateFunctions.end()) {
                (this->*it->second.onEntry)();
            } else {
                errorf("ERROR: No state function found for state: %s\n",
                       getStateName(currentState));
            }
        }
    } catch (const std::runtime_error &e) {
        errorf("State transition error: %s\n", e.what());
        debugf("Current state: %s, Trigger: %d\n", getStateName(currentState),
               static_cast<int>(trigger));
    }
}

void StateMachine::update() {
    static unsigned long lastUpdateTime     = 0;
    static unsigned long lastWatchdogTime   = 0;
    static unsigned long lastBatteryWarning = 0;

    message data;
    uint8_t source[6];
    int32_t current_rssi;

    while (EspNowSensor<message>::Poll(&data, (unsigned char *)source, &current_rssi)) {
        // Update RSSI and source for ALL messages to track nearby dice
        last_rssi = current_rssi;
        memcpy((void *)last_source, (void *)source, 6);

        switch (data.type) {
            case message_type::MESSAGE_TYPE_WATCH_DOG: // watch dog, send by all dices
                if (memcmp((void *)source, (void *)this->current_peer, 6) == 0) {
                    stateSister = data.data.watchDog.state;
                }
                break;

            case message_type::MESSAGE_TYPE_MEASUREMENT: // send by 2 entangled dices to each other.
                                                         // Store the data in the sisterStates
                if (memcmp((void *)source, (void *)this->current_peer, 6) == 0) {
                    debugln("Measurement received from partner - processing immediately");
                    stateSister       = data.data.measurement.state;
                    diceNumberSister  = data.data.measurement.diceNumber;
                    measureAxisSister = data.data.measurement.measureAxis;

                    // Store partner's measurement information
                    partnerMeasurementAxis = measureAxisSister;
                    partnerDiceNumber      = diceNumberSister;

                    // Clear current_peer so we can accept new entanglement requests
                    memset(this->current_peer, 0xFF, 6);

                    // Trigger state transition
                    changeState(Trigger::MEASUREMENT_RECEIVED);
                }
                break;

            case message_type::MESSAGE_TYPE_ENTANGLE_REQUEST:
                debugln("Entanglement request received - processing immediately");

                // Check if we're in CLASSIC mode - deny entanglement
                if (currentState.mode == Mode::CLASSIC) {
                    debugln("CLASSIC mode - denying entanglement request");
                    sendEntangleDenied((uint8_t *)source);
                    break;
                }

                // Check if we're already waiting for confirmation - deny to prevent race condition
                if (currentState.entanglementState == EntanglementState::ENTANGLE_REQUESTED) {
                    debugln(
                      "Already in ENTANGLE_REQUESTED - denying to prevent symmetric entanglement");
                    sendEntangleDenied((uint8_t *)source);
                    break;
                }

                // Check if we're already ENTANGLED - this means teleportation is being initiated
                if (currentState.entanglementState == EntanglementState::ENTANGLED) {
                    debugln("Already ENTANGLED - initiating TELEPORTATION protocol");
                    debugln(
                      "Teleport: Dice M (source) wants to teleport via us (A) to our partner (B)");

                    // Send TELEPORT_REQUEST to M with B's address
                    sendTeleportRequest((uint8_t *)source, this->current_peer);

                    // Store M's address in next_peer for later reference
                    memcpy((void *)this->next_peer, (void *)source, 6);

                    // We'll transition to PURE after receiving TELEPORT_CONFIRM
                    // Don't change state yet - wait for confirmation
                } else {
                    // Normal entanglement request
                    // Another dice wants to entangle with us
                    // Store their MAC and send confirmation with our chosen color
                    memcpy((void *)this->current_peer, (void *)source, 6);
                    debugf("Adding peer (current_peer): %02X:%02X:%02X:%02X:%02X:%02X\n",
                           this->current_peer[0], this->current_peer[1], this->current_peer[2],
                           this->current_peer[3], this->current_peer[4], this->current_peer[5]);

                    sendEntanglementConfirm((uint8_t *)source);
                    // Reset local measurement state for new entanglement
                    diceNumberSelf  = DiceNumbers::NONE;
                    upSideSelf      = UpSide::NONE;
                    measureAxisSelf = MeasuredAxises::UNDEFINED;
                    changeState(Trigger::ENTANGLE_REQUEST); // PURE/POST_ENTANGLEMENT -> ENTANGLED
                }
                break;

            case message_type::MESSAGE_TYPE_ENTANGLE_CONFIRM: // device A receives confirmation
                                                              // entangle request
                debugln("Entanglement confirmation received - processing immediately");
                // We sent a request and got confirmation - finalize entanglement
                if (currentState.entanglementState == EntanglementState::ENTANGLE_REQUESTED) {
                    memcpy((void *)this->current_peer, (void *)this->next_peer, 6);
                    memset((void *)this->next_peer, 0xFF, 6);

                    // Store the entanglement color from the confirming dice
                    this->entanglement_color = data.data.entangleConfirm.color;
                    entanglement_color_self  = this->entanglement_color; // Update global
                    debugf("Received entanglement color: 0x%04X\n", this->entanglement_color);

                    // Trigger color flash if showColors is disabled
                    if (!showColors) {
                        flashColor          = true;
                        flashColorStartTime = millis();
                        debugln("Triggering color flash (receiving entanglement)");
                    }

                    // Reset local measurement state for new entanglement
                    diceNumberSelf  = DiceNumbers::NONE;
                    upSideSelf      = UpSide::NONE;
                    measureAxisSelf = MeasuredAxises::UNDEFINED;
                    changeState(Trigger::ENTANGLE_CONFIRM); // ENTANGLE_REQUESTED -> ENTANGLED
                }
                break;

            case message_type::MESSAGE_TYPE_ENTANGLE_DENIED:
                // Another dice denied our entanglement request (e.g., they're in CLASSIC mode)
                debugln("Entanglement denied - returning to PURE state");

                // Clear the peer we tried to entangle with
                memset(this->next_peer, 0xFF, 6);

                // If we're in ENTANGLE_REQUESTED state, go back to PURE
                if (currentState.entanglementState == EntanglementState::ENTANGLE_REQUESTED) {
                    changeState(
                      Trigger::ENTANGLE_STOP); // Use ENTANGLE_STOP trigger to return to PURE
                }
                break;

            case message_type::MESSAGE_TYPE_TELEPORT_REQUEST:
                // Dice M receives this from dice A with B's address
                debugln("Teleport request received - M processing teleportation");
                debugln("Teleport: Sending our state to target dice B");

                {
                    uint8_t target_b[6];
                    memcpy((void *)target_b, (void *)data.data.teleportRequest.target_dice, 6);

                    // If M is entangled to N, inform N that its new partner is B
                    if (currentState.entanglementState == EntanglementState::ENTANGLED) {
                        uint8_t empty[6];
                        memset(empty, 0xFF, 6);
                        if (memcmp((void *)this->current_peer, (void *)empty, 6) != 0) {
                            debugln("M is entangled to N - informing N of new partner B");
                            sendTeleportPartner(this->current_peer, target_b);
                        }
                    }

                    // Send TELEPORT_PAYLOAD to B with our current state
                    sendTeleportPayload(target_b, stateSelf, diceNumberSelf, upSideSelf,
                                        measureAxisSelf, this->current_peer,
                                        this->entanglement_color);
                    sendTeleportConfirm((uint8_t *)source);

                    // Clear our entanglement if we had one
                    if (currentState.entanglementState == EntanglementState::ENTANGLED) {
                        // Remove old peer
                        memset(this->current_peer, 0xFF, 6);
                    }

                    // M goes to quantum idle state (full superposition) after teleportation
                    // Clear measurement state and memoization
                    diceNumberSelf  = DiceNumbers::NONE;
                    upSideSelf      = UpSide::NONE;
                    measureAxisSelf = MeasuredAxises::UNDEFINED;
                    lastRollBasis   = MeasuredAxises::UNDEFINED;
                    lastRollNumber  = DiceNumbers::NONE;

                    changeState(Trigger::TELEPORT_INITIATED);
                }
                break;

            case message_type::MESSAGE_TYPE_TELEPORT_CONFIRM:
                // Dice A receives confirmation from M that teleportation is complete
                debugln("Teleport confirm received - A ending entanglement with B");

                // Clear entanglement
                memset(this->current_peer, 0xFF, 6);
                memset(this->next_peer, 0xFF, 6);

                // A goes to quantum idle state (full superposition)
                // Clear measurement state and memoization
                diceNumberSelf  = DiceNumbers::NONE;
                upSideSelf      = UpSide::NONE;
                measureAxisSelf = MeasuredAxises::UNDEFINED;
                lastRollBasis   = MeasuredAxises::UNDEFINED;
                lastRollNumber  = DiceNumbers::NONE;

                changeState(Trigger::TELEPORT_CONFIRMED);
                break;

            case message_type::MESSAGE_TYPE_TELEPORT_PAYLOAD:
                // Dice B receives the teleported state from M
                debugln("Teleport payload received - B receiving M's state");

                {
                    State    teleported_state = data.data.teleportPayload.state;
                    uint16_t teleported_color = data.data.teleportPayload.color;

                    debugf("Received teleportation with color: 0x%04X\n", teleported_color);

                    // Remove old peer (A) from peer list
                    memset(this->current_peer, 0xFF, 6);

                    // Check what state M was in
                    if (teleported_state.entanglementState == EntanglementState::ENTANGLED) {
                        // M was entangled to N - B now becomes entangled to N
                        debugln("Teleported state is ENTANGLED - B now entangled to N");
                        memcpy((void *)this->current_peer,
                               (void *)data.data.teleportPayload.entangled_peer, 6);

                        // Store the teleported entanglement color
                        this->entanglement_color = teleported_color;
                        entanglement_color_self  = this->entanglement_color; // Update global
                        debugf("Inherited entanglement color: 0x%04X\n", this->entanglement_color);

                        // Trigger color flash if showColors is disabled
                        if (!showColors) {
                            flashColor          = true;
                            flashColorStartTime = millis();
                            debugln("Triggering color flash (receiving teleportation)");
                        }

                        diceNumberSelf  = DiceNumbers::NONE;
                        upSideSelf      = UpSide::NONE;
                        measureAxisSelf = MeasuredAxises::UNDEFINED;

                        // Transition to ENTANGLED (use ENTANGLE_REQUEST trigger for this)
                        currentState.entanglementState = EntanglementState::ENTANGLED;
                        stateSelf.entanglementState    = EntanglementState::ENTANGLED;
                        refreshScreens();

                    } else if (teleported_state.throwState == ThrowState::OBSERVED) {
                        // M was in observed state - B receives teleported measurement
                        debugln("Teleported state is OBSERVED - B enters TELEPORTED state");

                        // Store the teleported measurement
                        teleportedMeasurementAxis = data.data.teleportPayload.measureAxis;
                        teleportedDiceNumber      = data.data.teleportPayload.diceNumber;

                        // Transition from current state to TELEPORTED
                        currentState.entanglementState = EntanglementState::TELEPORTED;
                        stateSelf.entanglementState    = EntanglementState::TELEPORTED;
                        refreshScreens();

                    } else {
                        // M was in PURE state - B also goes to PURE
                        debugln("Teleported state is PURE - B enters PURE state");

                        changeState(
                          Trigger::TELEPORT_RECEIVED); // ENTANGLED/POST_ENTANGLEMENT -> PURE
                    }
                }
                break;

            case message_type::MESSAGE_TYPE_TELEPORT_PARTNER:
                // Dice N receives notification that its partner changed from M to B
                debugln("Teleport partner update received - N updating partner from M to B");

                {
                    uint8_t new_partner_b[6];
                    memcpy((void *)new_partner_b, (void *)data.data.teleportPartner.new_partner, 6);

                    debugf("New partner: %02X:%02X:%02X:%02X:%02X:%02X\n", new_partner_b[0],
                           new_partner_b[1], new_partner_b[2], new_partner_b[3], new_partner_b[4],
                           new_partner_b[5]);

                    // Update current_peer to B
                    memcpy((void *)this->current_peer, (void *)new_partner_b, 6);

                    // N stays in ENTANGLED state, just with a different partner
                    // No state transition needed
                    debugln("N remains ENTANGLED, now with B instead of M");
                }
                break;
        }
    }

    _imuSensor->update();
    unsigned long currentTime = millis();

    // Battery monitoring
    if (checkMinimumVoltage()) {
        if (currentTime - lastBatteryWarning >= BATTERY_WARNING_INTERVAL) {
            debugln("Low battery detected!");
            lastBatteryWarning = currentTime;

            voltageIndicator(ALL);
            sleep(3);
            refreshScreens();
        }
    }

    // State-independent: Handle short click to toggle color display (only in QUANTUM mode)
    if (clicked) {
        clicked = false;
        if (currentState.mode == Mode::QUANTUM) {
            showColors = !showColors;
            debugf("Color display toggled: %s\n", showColors ? "ON" : "OFF");
            refreshScreens(); // Refresh to show the change immediately
        } else {
            debugln("Short click ignored in CLASSIC mode");
        }
    }

    // Periodically send watchdog to broadcast presence to nearby dice
    if (currentState.mode != Mode::CLASSIC
        && (currentTime - lastWatchdogTime >= 500)) { // Send every 500ms
        // Not sending in CLASSIC mode ensures we don't get contacted about entanglement
        // and reduces power consumption and network traffic
        sendWatchDog();
        lastWatchdogTime = currentTime;
    }

    // State-independent: Handle color flash timeout
    if (flashColor && (currentTime - flashColorStartTime >= currentConfig.colorFlashTimeout)) {
        debugln("Color flash timeout - refreshing screens to show white");
        flashColor = false;
        refreshScreens(); // Update display to show white instead of color
    }

    if (currentTime - lastUpdateTime >= FSM_UPDATE_INTERVAL) {
        // add functions called at state update
        //  printDiceStateName("DiceState", diceStateSelf);
        lastUpdateTime = currentTime;

        // Call whileInState function for current state
        auto it = stateFunctions.find(currentState);
        if (it != stateFunctions.end()) {
            (this->*it->second.whileInState)();
        } else {
            errorf("ERROR: No state function found for state: %s\n", getStateName(currentState));
        }
    }

    checkTimeForDeepSleep(_imuSensor);
}

// ============================================================================
// STATE HANDLER IMPLEMENTATIONS
// ============================================================================

// === CLASSIC MODE ===

void StateMachine::enterClassicIdle() {
    debugln("=== Entering CLASSIC MODE ===");
    stateEntryTime = millis();
    stateSelf      = currentState;

    // Initialize display state
    diceNumberSelf  = DiceNumbers::NONE;
    upSideSelf      = UpSide::NONE;
    measureAxisSelf = MeasuredAxises::UNDEFINED;

    sendWatchDog();
    refreshScreens();
}

void StateMachine::whileClassicIdle() {
    // Check for button press to switch to quantum mode
    if (longclicked) {
        longclicked = false;
        debugln("Button pressed - switching to QUANTUM mode");
        changeState(Trigger::BUTTON_PRESSED);
    }
}

// === QUANTUM MODE - IDLE ===

void StateMachine::enterQuantumIdle() {
    debugln("=== Entering QUANTUM IDLE ===");
    stateEntryTime = millis();
    stateSelf      = currentState;

    // Display state depends only on current State (mode, throwState, entanglementState)
    // No need for separate diceStateSelf variable

    // Reset tumble detection for next throw
    _imuSensor->resetTumbleDetection();

    // Reset button flag
    longclicked = false;

    sendWatchDog();
    refreshScreens();
}

void StateMachine::whileQuantumIdle() {
    // Check for button press to switch back to classic mode
    // Allow switching from PURE, POST_ENTANGLEMENT, or TELEPORTED states (not when entangled)
    if (longclicked
        && (currentState.entanglementState == EntanglementState::PURE
            || currentState.entanglementState == EntanglementState::POST_ENTANGLEMENT
            || currentState.entanglementState == EntanglementState::TELEPORTED)) {
        longclicked = false;
        debugln("Button pressed - switching to CLASSIC mode");
        changeState(Trigger::BUTTON_PRESSED);
        return;
    }

    // Check if dice is being thrown
    if (_imuSensor->tumbled()) {
        debugln("Tumble detected - starting throw");
        changeState(Trigger::START_ROLLING);
        return;
    }

    // Handle entanglement logic
    switch (currentState.entanglementState) {
        case EntanglementState::PURE:
        case EntanglementState::POST_ENTANGLEMENT:
        case EntanglementState::TELEPORTED:
            // Check for nearby dice to initiate entanglement
            // Don't re-entangle with current or pending partner
            if (last_rssi > currentConfig.rssiLimit && last_rssi < -1
                && memcmp((void *)last_source, (void *)this->current_peer, 6) != 0
                && memcmp((void *)last_source, (void *)this->next_peer, 6) != 0) {
                debugln("Nearby dice detected - sending entanglement request");
                memcpy((void *)this->next_peer, (void *)last_source, 6);
                debugf("Adding peer (next_peer): %02X:%02X:%02X:%02X:%02X:%02X\n",
                       this->next_peer[0], this->next_peer[1], this->next_peer[2],
                       this->next_peer[3], this->next_peer[4], this->next_peer[5]);
                sendEntangleRequest((uint8_t *)last_source);
                last_rssi = INT32_MIN;
                changeState(Trigger::CLOSE_BY); // PURE/TELEPORTED -> ENTANGLE_REQUESTED
                return;
            }
            break;

        case EntanglementState::ENTANGLED:
            // ENTANGLED dice initiates TELEPORTATION when detecting nearby dice
            // Send TELEPORT_REQUEST directly (not ENTANGLE_REQUEST) to prevent phantom entanglement
            // Don't try to teleport to the dice we're already entangled with
            if (last_rssi > currentConfig.rssiLimit && last_rssi < -1
                && memcmp((void *)last_source, (void *)this->current_peer, 6) != 0
                && memcmp((void *)last_source, (void *)this->next_peer, 6) != 0) {
                debugln("Nearby dice detected while ENTANGLED - sending TELEPORT_REQUEST directly");
                memcpy((void *)this->next_peer, (void *)last_source, 6);
                debugf("Initiating teleport to M (next_peer): %02X:%02X:%02X:%02X:%02X:%02X\n",
                       this->next_peer[0], this->next_peer[1], this->next_peer[2],
                       this->next_peer[3], this->next_peer[4], this->next_peer[5]);

                // Send TELEPORT_REQUEST directly with current_peer (B) as target
                sendTeleportRequest((uint8_t *)last_source, this->current_peer);

                last_rssi = INT32_MIN;
                // Don't change state yet - wait for TELEPORT_CONFIRM
                return;
            }
            break;

        case EntanglementState::ENTANGLE_REQUESTED:
            // Waiting for partner confirmation
            // The confirmation is now handled directly in update() method
            // This state just waits and can timeout
            if (millis() - stateEntryTime > MAXENTANGLEDWAITTIME) {
                debugln("Entanglement request timeout - returning to PURE state");
                changeState(Trigger::TIMED);
                return;
            }
            break;
    }
}

// === QUANTUM MODE - THROWING ===

void StateMachine::enterThrowing() {
    debugln("=== Dice is THROWING ===");
    stateEntryTime = millis();
    stateSelf      = currentState;

    refreshScreens();
    sendWatchDog();
}

void StateMachine::whileThrowing() {
    // Check for button press to switch back to classic mode
    // Allow switching from PURE, POST_ENTANGLEMENT, or TELEPORTED states (not when entangled)
    if (longclicked
        && (currentState.entanglementState == EntanglementState::PURE
            || currentState.entanglementState == EntanglementState::POST_ENTANGLEMENT
            || currentState.entanglementState == EntanglementState::TELEPORTED)) {
        longclicked = false;
        debugln("Button pressed - switching to CLASSIC mode");
        changeState(Trigger::BUTTON_PRESSED);
        return;
    }

    // Check if dice has landed and is stable
    if (_imuSensor->stable() && _imuSensor->on_table()) {
        debugln("Dice stable and on table - moving to OBSERVED");
        changeState(Trigger::STOP_ROLLING);
        return;
    }

    // Check for nearby dice to initiate entanglement (only when PURE)
    // This will transition to IDLE + ENTANGLE_REQUESTED state
    if (currentState.entanglementState == EntanglementState::PURE) {
        // Check for nearby dice to initiate entanglement
        // Don't re-entangle with current or pending partner
        if (last_rssi > currentConfig.rssiLimit && last_rssi < -1
            && memcmp((void *)last_source, (void *)this->current_peer, 6) != 0
            && memcmp((void *)last_source, (void *)this->next_peer, 6) != 0) {
            debugln(
              "Nearby dice detected in THROWING - sending entanglement request and returning to "
              "IDLE");
            memcpy((void *)this->next_peer, (void *)last_source, 6);
            debugf("Adding peer (next_peer): %02X:%02X:%02X:%02X:%02X:%02X\n", this->next_peer[0],
                   this->next_peer[1], this->next_peer[2], this->next_peer[3], this->next_peer[4],
                   this->next_peer[5]);
            sendEntangleRequest((uint8_t *)last_source);
            last_rssi = INT32_MIN;
            changeState(Trigger::CLOSE_BY); // Will transition to IDLE + ENTANGLE_REQUESTED
            return;
        }
    }

    // Partner measurement is handled directly in update() method via MEASUREMENT_RECEIVED trigger
}

// === QUANTUM MODE - OBSERVED (MEASUREMENT) ===

void StateMachine::enterObserved() {
    debugln("=== Dice OBSERVED - Processing measurement ===");
    stateEntryTime = millis();
    stateSelf      = currentState;

    // Check if dice is still moving (measurement failure)
    if (_imuSensor->moving()) {
        debugln("Dice still moving - measurement failed");
        changeState(Trigger::MEASURE_FAIL);
        return;
    }

    // Determine which axis is facing up
    IMU_Orientation orient = _imuSensor->orientation();

    switch (orient) {
        case IMU_Orientation::ORIENTATION_Z_UP:
            measureAxisSelf = MeasuredAxises::ZAXIS;
            upSideSelf      = UpSide::Z0;
            debugln("Measured: Z+ axis");
            break;
        case IMU_Orientation::ORIENTATION_Z_DOWN:
            measureAxisSelf = MeasuredAxises::ZAXIS;
            upSideSelf      = UpSide::Z1;
            debugln("Measured: Z- axis");
            break;
        case IMU_Orientation::ORIENTATION_X_UP:
            measureAxisSelf = MeasuredAxises::XAXIS;
            upSideSelf      = UpSide::X1; // Inverted: X_UP maps to X1
            debugln("Measured: X+ axis");
            break;
        case IMU_Orientation::ORIENTATION_X_DOWN:
            measureAxisSelf = MeasuredAxises::XAXIS;
            upSideSelf      = UpSide::X0; // Inverted: X_DOWN maps to X0
            debugln("Measured: X- axis");
            break;
        case IMU_Orientation::ORIENTATION_Y_UP:
            measureAxisSelf = MeasuredAxises::YAXIS;
            upSideSelf      = UpSide::Y0;
            debugln("Measured: Y+ axis");
            break;
        case IMU_Orientation::ORIENTATION_Y_DOWN:
            measureAxisSelf = MeasuredAxises::YAXIS;
            upSideSelf      = UpSide::Y1;
            debugln("Measured: Y- axis");
            break;
        case IMU_Orientation::ORIENTATION_TILTED:
        case IMU_Orientation::ORIENTATION_UNKNOWN:
            debugln("No clear axis - measurement failed");
            changeState(Trigger::MEASURE_FAIL);
            return;
    }

    // Determine the dice number based on entanglement state
    switch (currentState.entanglementState) {
        case EntanglementState::PURE:
            // Check if we're measuring in the same basis as the last roll
            if (measureAxisSelf == lastRollBasis && lastRollNumber != DiceNumbers::NONE) {
                // Same basis - return the memoized value
                debugln("PURE state: same basis as last roll, using memoized value");
                diceNumberSelf = lastRollNumber;
            } else {
                // Different basis or first roll - generate new random number
                debugln("PURE state: generating random number");
                diceNumberSelf = selectOneToSix();
                // Update memoization
                lastRollBasis  = measureAxisSelf;
                lastRollNumber = diceNumberSelf;
            }
            break;

        case EntanglementState::ENTANGLED:
            // We measured first - generate random and send to partner
            debugln("ENTANGLED state: we measured first");
            diceNumberSelf = selectOneToSix();

            // Send our measurement to partner
            sendMeasurements(this->current_peer, stateSelf, diceNumberSelf, upSideSelf,
                             measureAxisSelf);

            // Update memoization
            lastRollBasis  = measureAxisSelf;
            lastRollNumber = diceNumberSelf;

            // Clear entanglement
            currentState.entanglementState = EntanglementState::PURE;
            stateSelf.entanglementState    = EntanglementState::PURE;
            memset(this->current_peer, 0xFF, 6);
            break;

        case EntanglementState::POST_ENTANGLEMENT:
            // Partner measured first - check if same axis
            debugln("POST_ENTANGLEMENT state: partner measured first");

            if (measureAxisSelf == partnerMeasurementAxis) {
                // Same measurement basis - show opposite value (sum = 7)
                debugln("Same axis as partner - showing opposite value");
                diceNumberSelf = selectOppositeOneToSix(partnerDiceNumber);
            } else {
                // Different measurement basis - random value
                debugln("Different axis from partner - random value");
                diceNumberSelf = selectOneToSix();
            }

            // Update memoization
            lastRollBasis  = measureAxisSelf;
            lastRollNumber = diceNumberSelf;

            // Clear partner info and entanglement
            partnerMeasurementAxis         = MeasuredAxises::UNDEFINED;
            partnerDiceNumber              = DiceNumbers::NONE;
            currentState.entanglementState = EntanglementState::PURE;
            stateSelf.entanglementState    = EntanglementState::PURE;
            break;

        case EntanglementState::TELEPORTED:
            // Received teleported state - check if same axis as teleported measurement
            debugln("TELEPORTED state: checking measurement axis");

            if (measureAxisSelf == teleportedMeasurementAxis) {
                // Same measurement basis - show teleported value
                debugln("Same axis as teleported state - showing teleported value");
                diceNumberSelf = teleportedDiceNumber;
            } else {
                // Different measurement basis - random value (collapses teleported state)
                debugln("Different axis from teleported state - random value");
                diceNumberSelf = selectOneToSix();
            }

            // Update memoization
            lastRollBasis  = measureAxisSelf;
            lastRollNumber = diceNumberSelf;

            // Clear teleported info
            teleportedMeasurementAxis      = MeasuredAxises::UNDEFINED;
            teleportedDiceNumber           = DiceNumbers::NONE;
            currentState.entanglementState = EntanglementState::PURE;
            stateSelf.entanglementState    = EntanglementState::PURE;
            break;

        case EntanglementState::ENTANGLE_REQUESTED:
            // Shouldn't happen, but treat as PURE
            diceNumberSelf = selectOneToSix();
            break;
    }

    // Store previous states
    prevMeasureAxisSelf = measureAxisSelf;
    prevUpSideSelf      = upSideSelf;

    // Reset tumble detection so we're ready for the next throw
    _imuSensor->resetTumbleDetection();

    refreshScreens();
    sendWatchDog();
}

void StateMachine::whileObserved() {
    // Check for button press to switch back to classic mode
    // Allow switching from PURE, POST_ENTANGLEMENT, or TELEPORTED states (not when entangled)
    if (longclicked
        && (currentState.entanglementState == EntanglementState::PURE
            || currentState.entanglementState == EntanglementState::POST_ENTANGLEMENT
            || currentState.entanglementState == EntanglementState::TELEPORTED)) {
        longclicked = false;
        debugln("Button pressed - switching to CLASSIC mode");
        changeState(Trigger::BUTTON_PRESSED);
        return;
    }

    // Check if dice is being thrown again
    if (_imuSensor->tumbled()) {
        debugln("Tumble detected - starting new throw");
        changeState(Trigger::START_ROLLING);
        return;
    }

    // Check for nearby dice to initiate entanglement (only when PURE)
    // This will transition to IDLE + ENTANGLE_REQUESTED state
    if (currentState.entanglementState == EntanglementState::PURE) {
        // Check for nearby dice to initiate entanglement
        // Don't re-entangle with current or pending partner
        if (last_rssi > currentConfig.rssiLimit && last_rssi < -1
            && memcmp((void *)last_source, (void *)this->current_peer, 6) != 0
            && memcmp((void *)last_source, (void *)this->next_peer, 6) != 0) {
            debugln(
              "Nearby dice detected in OBSERVED - sending entanglement request and returning to "
              "IDLE");
            memcpy((void *)this->next_peer, (void *)last_source, 6);
            debugf("Adding peer (next_peer): %02X:%02X:%02X:%02X:%02X:%02X\n", this->next_peer[0],
                   this->next_peer[1], this->next_peer[2], this->next_peer[3], this->next_peer[4],
                   this->next_peer[5]);
            sendEntangleRequest((uint8_t *)last_source);
            last_rssi = INT32_MIN;
            changeState(Trigger::CLOSE_BY); // Will transition to IDLE + ENTANGLE_REQUESTED
            return;
        }
    }

    // Stay in OBSERVED state showing the measured value
    // Wait for user to roll again
}

// === LOW BATTERY ===

void StateMachine::enterLowBattery() {
    debugln("=== LOW BATTERY STATE ===");
    stateEntryTime = millis();
    stateSelf      = currentState;

    diceNumberSelf  = DiceNumbers::NONE;
    upSideSelf      = UpSide::NONE;
    measureAxisSelf = MeasuredAxises::UNDEFINED;

    sendWatchDog();
    refreshScreens();
}

void StateMachine::whileLowBattery() {
    // Display battery indicator
    voltageIndicator(XX);
}
