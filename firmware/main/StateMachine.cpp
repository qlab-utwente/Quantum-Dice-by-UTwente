#include "StateMachine.hpp"

#include "defines.hpp"
#include "DiceConfigManager.hpp"
#include "EspNowSensor.hpp"
#include "IMUhelpers.hpp"
#include "Screenfunctions.hpp"
#include "ScreenStateDefs.hpp"

#include <map>
#include <driver/rtc_io.h>
#include "esp_random.h"

#include "Battery.hpp"
#include "button.h"
#include "power.h"

static uint8_t generateDiceRoll() {
	// Get a random 32-bit integer from the crypto chip
	uint32_t randomNumber = esp_random();

	// Check if we got a valid random number (0 indicates error)
	if (randomNumber == 0) {
		Serial.println("ERROR: Failed to get random number");
		return 1;  // Default value in case of error
	}

	// Map to 1-6 range using modulo
	return (randomNumber % 6) + 1;
}

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
	watchDog.type = message_type::MESSAGE_TYPE_WATCH_DOG;
	watchDog.data.watchDog.state = this->currentState;
	uint8_t target[MAC_ADDRESS_LENGTH];
	memset((void *)target, 0xFF, MAC_ADDRESS_LENGTH);
	EspNowSensor<message>::Send(watchDog, target);
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

    // Trigger color flash
    this->colorFlash = true;
    this->colorFlashStartTime = millis();
    debugln("Triggering color flash (accepting entanglement)");

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

auto StateMachine::getStateTransition(State state, Trigger trigger) -> StateTransition {
	for (const StateTransition &transition : StateMachine::stateTransitions) {
		bool modeMatch = !transition.currentMode.has_value() || transition.currentMode.value() == state.mode;
		bool throwStateMatch = !transition.currentThrowState.has_value() || transition.currentThrowState.value() == state.throwState;
		bool entanglementStateMatch = !transition.currentEntanglementState.has_value() || transition.currentEntanglementState.value() == state.entanglementState;

		if (modeMatch && throwStateMatch && entanglementStateMatch && transition.trigger == trigger) {
			return transition;
		}
	}
	throw std::runtime_error("No valid state transition found");
}

// declaration of instance
StateMachine::StateMachine()
	: _imuSensor(nullptr),
	  currentState{
		.mode = Mode::CLASSIC,
		.throwState = ThrowState::IDLE,
		.entanglementState = EntanglementState::PURE
	  },
	  current_peer{ 0, 0, 0, 0, 0, 0 },
	  next_peer{ 0, 0, 0, 0, 0, 0 },
	  new_peer{ 0, 0, 0, 0, 0, 0 },
	  new_peer_rssi(INT32_MIN),
	  stateEntryTime(0),
	  selfDiceNumber(DiceNumbers::NONE),
	  selfMeasurementAxis(MeasuredAxises::UNDEFINED),
	  selfUpSide(UpSide::NONE),
	  partnerMeasurementAxis(MeasuredAxises::UNDEFINED),
	  partnerDiceNumber(DiceNumbers::NONE),
	  teleportedMeasurementAxis(MeasuredAxises::UNDEFINED),
	  teleportedDiceNumber(DiceNumbers::NONE),
	  entanglement_color(0xFFE0),
	  lastRollBasis(MeasuredAxises::UNDEFINED),
	  lastRollNumber(DiceNumbers::NONE),
	  colorFlash(false),
	  colorFlashStartTime(0) {}

void StateMachine::begin() {
    // Initialize ESP-NOW with device A MAC from config
    EspNowSensor<message>::Init();

    infoln("ESP-NOW initialized successfully!");

    EspNowSensor<message>::PrintMacAddress();

    infoln("StateMachine Begin: Calling onEntry for initial state");
    printStateName("StateMachine", this->currentState);

    sleep(3);
    refreshScreens();

    infoln("StateMachine Begin: Setting initial state");
    // Call the onEntry function for the initial state
    auto it = stateFunctions.find(this->currentState);
    if (it != stateFunctions.end()) {
        (this->*it->second.onEntry)();
    } else {
        errorln("ERROR: No state function found for initial state!");
    }
}

void StateMachine::changeState(Trigger trigger) {
    // Get the state transition for the current state and trigger
    try {
        StateTransition transition = getStateTransition(this->currentState, trigger);

        // Create new state based on transition
        State newState = this->currentState; // Start with current state

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
        if (newState != this->currentState) {
            this->currentState = newState;
            printStateName("stateMachine", this->currentState);

            // Call onEntry function for new state
            auto it = stateFunctions.find(this->currentState);
            if (it != stateFunctions.end()) {
                (this->*it->second.onEntry)();
            } else {
                errorf("ERROR: No state function found for state: %s\n", getStateName(this->currentState));
            }
        }
    } catch (const std::runtime_error &e) {
        errorf("State transition error: %s\n", e.what());
        debugf("Current state: %s, Trigger: %d\n", getStateName(this->currentState), static_cast<int>(trigger));
    }
}

void StateMachine::update() {
	static unsigned long lastWatchdogTime = 0;
	unsigned long currentTime = millis();

	// If the button is held for a long time, then we must shut down.
	if (button_poll_press_long()) {
		power_shutdown();
	}

	// Check whether the dice has been inactive for long enough to go to sleep.
	this->checkTimeForDeepSleep();

	// Check the battery state.
	this->checkBattery();

	// Poll the received messages from the ESP-NOW and update the IMU sensor.
	this->updateEspNow();
	_imuSensor->update();

	// Periodically send watchdog to broadcast presence to nearby dice
	if (this->currentState.mode != Mode::CLASSIC
		&& (currentTime - lastWatchdogTime >= 500)) { // Send every 500ms
		// Not sending in CLASSIC mode ensures we don't get contacted about entanglement
		// and reduces power consumption and network traffic
		sendWatchDog();
		lastWatchdogTime = currentTime;
	}

	// State-independent: Handle color flash timeout
	if (this->colorFlash && (currentTime - this->colorFlashStartTime >= currentConfig.colorFlashTimeout)) {
		debugln("Color flash timeout - refreshing screens to show white");
		this->colorFlash = false;
		refreshScreens(); // Update display to show white instead of color
	}

	// Call whileInState function for current state
	auto it = stateFunctions.find(this->currentState);
	if (it != stateFunctions.end()) {
		(this->*it->second.whileInState)();
	} else {
		errorf("ERROR: No state function found for state: %s\n", getStateName(this->currentState));
	}

	// Check for any nearby dice to entangle with.
	this->checkCloseBy();
}

void StateMachine::updateEspNow() {
	message data;
	uint8_t source[MAC_ADDRESS_LENGTH];
	int32_t current_rssi = INT32_MIN;

	while (EspNowSensor<message>::Poll(&data, (unsigned char *)source, &current_rssi)) {
		// Update RSSI and source for ALL messages to track nearby dice
		if (current_rssi > this->new_peer_rssi) {
			this->new_peer_rssi = current_rssi;
			memcpy((void *)this->new_peer, (void *)source, MAC_ADDRESS_LENGTH);
		}

		switch (data.type) {
            case message_type::MESSAGE_TYPE_MEASUREMENT: // send by 2 entangled dices to each other.
                                                         // Store the data in the sisterStates
                if (memcmp((void *)source, (void *)this->current_peer, MAC_ADDRESS_LENGTH) == 0) {
                    debugln("Measurement received from partner - processing immediately");
                    this->partnerDiceNumber = data.data.measurement.diceNumber;
                    this->partnerMeasurementAxis = data.data.measurement.measureAxis;

                    // Clear current_peer so we can accept new entanglement requests
                    memset(this->current_peer, 0xFF, MAC_ADDRESS_LENGTH);

                    // Trigger state transition
                    changeState(Trigger::MEASUREMENT_RECEIVED);
                }
                break;

            case message_type::MESSAGE_TYPE_ENTANGLE_REQUEST:
                debugln("Entanglement request received - processing immediately");

                // Check if we're in CLASSIC mode - deny entanglement
                if (this->currentState.mode == Mode::CLASSIC) {
                    debugln("CLASSIC mode - denying entanglement request");
                    sendEntangleDenied(source);
                    break;
                }

                // Check if we're already waiting for confirmation - deny to prevent race condition
                if (this->currentState.entanglementState == EntanglementState::ENTANGLE_REQUESTED) {
                    debugln("Already in ENTANGLE_REQUESTED - denying to prevent symmetric entanglement");
                    sendEntangleDenied(source);
                    break;
                }

                // Check if we're already ENTANGLED - this means teleportation is being initiated
                if (this->currentState.entanglementState == EntanglementState::ENTANGLED) {
                    debugln("Already ENTANGLED - initiating TELEPORTATION protocol");
                    debugln(
                      "Teleport: Dice M (source) wants to teleport via us (A) to our partner (B)");

                    // Send TELEPORT_REQUEST to M with B's address
                    sendTeleportRequest(source, this->current_peer);

                    // Store M's address in next_peer for later reference
                    memcpy((void *)this->next_peer, (void *)source, MAC_ADDRESS_LENGTH);

                    // We'll transition to PURE after receiving TELEPORT_CONFIRM
                    // Don't change state yet - wait for confirmation
                } else {
                    // Normal entanglement request
                    // Another dice wants to entangle with us
                    // Store their MAC and send confirmation with our chosen color
                    memcpy((void *)this->current_peer, (void *)source, MAC_ADDRESS_LENGTH);
                    debugf("Adding peer (current_peer): %02X:%02X:%02X:%02X:%02X:%02X\n",
                           this->current_peer[0], this->current_peer[1], this->current_peer[2],
                           this->current_peer[3], this->current_peer[4], this->current_peer[5]);

                    sendEntanglementConfirm((uint8_t *)source);
                    // Reset local measurement state for new entanglement
                    this->selfDiceNumber = DiceNumbers::NONE;
                    this->selfUpSide = UpSide::NONE;
                    this->selfMeasurementAxis = MeasuredAxises::UNDEFINED;
                    changeState(Trigger::ENTANGLE_REQUEST); // PURE/POST_ENTANGLEMENT -> ENTANGLED
                }
                break;

            case message_type::MESSAGE_TYPE_ENTANGLE_CONFIRM: // device A receives confirmation
                                                              // entangle request
                debugln("Entanglement confirmation received - processing immediately");
                // We sent a request and got confirmation - finalize entanglement
                if (this->currentState.entanglementState == EntanglementState::ENTANGLE_REQUESTED) {
                    memcpy((void *)this->current_peer, (void *)this->next_peer, MAC_ADDRESS_LENGTH);
                    memset((void *)this->next_peer, 0xFF, MAC_ADDRESS_LENGTH);

                    // Store the entanglement color from the confirming dice
                    this->entanglement_color = data.data.entangleConfirm.color;
                    debugf("Received entanglement color: 0x%04X\n", this->entanglement_color);

                    // Trigger color flash
                    this->colorFlash = true;
                    this->colorFlashStartTime = millis();
                    debugln("Triggering color flash (receiving entanglement)");

                    // Reset local measurement state for new entanglement
                    this->selfDiceNumber = DiceNumbers::NONE;
                    this->selfUpSide = UpSide::NONE;
                    this->selfMeasurementAxis = MeasuredAxises::UNDEFINED;
                    changeState(Trigger::ENTANGLE_CONFIRM); // ENTANGLE_REQUESTED -> ENTANGLED
                }
                break;

            case message_type::MESSAGE_TYPE_ENTANGLE_DENIED:
                // Another dice denied our entanglement request (e.g., they're in CLASSIC mode)
                debugln("Entanglement denied - returning to PURE state");

                // Clear the peer we tried to entangle with
                memset(this->next_peer, 0xFF, MAC_ADDRESS_LENGTH);

                // If we're in ENTANGLE_REQUESTED state, go back to PURE
                if (this->currentState.entanglementState == EntanglementState::ENTANGLE_REQUESTED) {
                    changeState(
                      Trigger::ENTANGLE_STOP); // Use ENTANGLE_STOP trigger to return to PURE
                }
                break;

            case message_type::MESSAGE_TYPE_TELEPORT_REQUEST:
                // Dice M receives this from dice A with B's address
                debugln("Teleport request received - M processing teleportation");
                debugln("Teleport: Sending our state to target dice B");

                {
                    uint8_t target_b[MAC_ADDRESS_LENGTH];
                    memcpy((void *)target_b, (void *)data.data.teleportRequest.target_dice, MAC_ADDRESS_LENGTH);

                    // If M is entangled to N, inform N that its new partner is B
                    if (this->currentState.entanglementState == EntanglementState::ENTANGLED) {
                        uint8_t empty[MAC_ADDRESS_LENGTH];
                        memset(empty, 0xFF, MAC_ADDRESS_LENGTH);
                        if (memcmp((void *)this->current_peer, (void *)empty, MAC_ADDRESS_LENGTH) != 0) {
                            debugln("M is entangled to N - informing N of new partner B");
                            sendTeleportPartner(this->current_peer, target_b);
                        }
                    }

                    // Send TELEPORT_PAYLOAD to B with our current state
                    sendTeleportPayload(target_b, this->currentState, this->selfDiceNumber, this->selfUpSide,
                                        this->selfMeasurementAxis, this->current_peer,
                                        this->entanglement_color);
                    sendTeleportConfirm(source);

                    // Clear our entanglement if we had one
                    if (this->currentState.entanglementState == EntanglementState::ENTANGLED) {
                        // Remove old peer
                        memset(this->current_peer, 0xFF, MAC_ADDRESS_LENGTH);
                    }

                    // M goes to quantum idle state (full superposition) after teleportation
                    // Clear measurement state and memoization
                    this->selfDiceNumber = DiceNumbers::NONE;
                    this->selfUpSide = UpSide::NONE;
                    this->selfMeasurementAxis = MeasuredAxises::UNDEFINED;
                    lastRollBasis   = MeasuredAxises::UNDEFINED;
                    lastRollNumber  = DiceNumbers::NONE;

                    changeState(Trigger::TELEPORT_INITIATED);
                }
                break;

            case message_type::MESSAGE_TYPE_TELEPORT_CONFIRM:
                // Dice A receives confirmation from M that teleportation is complete
                debugln("Teleport confirm received - A ending entanglement with B");

                // Clear entanglement
                memset(this->current_peer, 0xFF, MAC_ADDRESS_LENGTH);
                memset(this->next_peer, 0xFF, MAC_ADDRESS_LENGTH);

                // A goes to quantum idle state (full superposition)
                // Clear measurement state and memoization
                this->selfDiceNumber = DiceNumbers::NONE;
                this->selfUpSide = UpSide::NONE;
                this->selfMeasurementAxis = MeasuredAxises::UNDEFINED;
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
                    memset(this->current_peer, 0xFF, MAC_ADDRESS_LENGTH);

                    // Check what state M was in
                    if (teleported_state.entanglementState == EntanglementState::ENTANGLED) {
                        // M was entangled to N - B now becomes entangled to N
                        debugln("Teleported state is ENTANGLED - B now entangled to N");
                        memcpy((void *)this->current_peer,
                               (void *)data.data.teleportPayload.entangled_peer, MAC_ADDRESS_LENGTH);

                        // Store the teleported entanglement color
                        this->entanglement_color = teleported_color;
                        debugf("Inherited entanglement color: 0x%04X\n", this->entanglement_color);

                        // Trigger color flash
                        this->colorFlash = true;
                        this->colorFlashStartTime = millis();
                        debugln("Triggering color flash (receiving teleportation)");

                        this->selfDiceNumber = DiceNumbers::NONE;
                        this->selfUpSide = UpSide::NONE;
                        this->selfMeasurementAxis = MeasuredAxises::UNDEFINED;

                        // Transition to ENTANGLED (use ENTANGLE_REQUEST trigger for this)
                        this->currentState.entanglementState = EntanglementState::ENTANGLED;
                        refreshScreens();

                    } else if (teleported_state.throwState == ThrowState::OBSERVED) {
                        // M was in observed state - B receives teleported measurement
                        debugln("Teleported state is OBSERVED - B enters TELEPORTED state");

                        // Store the teleported measurement
                        teleportedMeasurementAxis = data.data.teleportPayload.measureAxis;
                        teleportedDiceNumber      = data.data.teleportPayload.diceNumber;

                        // Transition from current state to TELEPORTED
                        this->currentState.entanglementState = EntanglementState::TELEPORTED;
                        refreshScreens();

                    } else {
                        // M was in PURE state - B also goes to PURE
                        debugln("Teleported state is PURE - B enters PURE state");
                        changeState(Trigger::TELEPORT_RECEIVED); // ENTANGLED/POST_ENTANGLEMENT -> PURE
                    }
                }
                break;

            case message_type::MESSAGE_TYPE_TELEPORT_PARTNER:
                // Dice N receives notification that its partner changed from M to B
                debugln("Teleport partner update received - N updating partner from M to B");

                {
                    uint8_t new_partner_b[MAC_ADDRESS_LENGTH];
                    memcpy((void *)new_partner_b, (void *)data.data.teleportPartner.new_partner, MAC_ADDRESS_LENGTH);

                    debugf("New partner: %02X:%02X:%02X:%02X:%02X:%02X\n", new_partner_b[0],
                           new_partner_b[1], new_partner_b[2], new_partner_b[3], new_partner_b[4],
                           new_partner_b[5]);

                    // Update current_peer to B
                    memcpy((void *)this->current_peer, (void *)new_partner_b, MAC_ADDRESS_LENGTH);

                    // N stays in ENTANGLED state, just with a different partner
                    // No state transition needed
                    debugln("N remains ENTANGLED, now with B instead of M");
                }
                break;
        }
    }
}

void StateMachine::checkBattery() {
	static uint64_t lastBatteryWarning = 0;
	uint64_t currentTime = millis();

	// Get the state of charge, check whether it is too low.
	float stateOfCharge = Battery.getStateOfCharge();
	bool tooLowStateOfCharge = stateOfCharge < StateMachine::BATTERY_MINIMUM_CHARGE;
	bool tooLongSinceWarning = (currentTime - lastBatteryWarning) >= BATTERY_WARNING_INTERVAL;

	// If the state of charge is too low and it has been a while since the last warning, then we
	// should issue another warning.
	if (tooLowStateOfCharge && tooLongSinceWarning) {
		infoln("Low battery detected!");
		lastBatteryWarning = currentTime;

		voltageIndicator(ALL);
		sleep(3);
		refreshScreens();
	}
}

void StateMachine::checkTimeForDeepSleep() {
	static bool isMoving = false;
	static unsigned long lastMovementTime = 0;

	if (this->_imuSensor->stable()) {
		if (isMoving) {
			lastMovementTime = millis();
			isMoving = false;
		}
	} else {
		isMoving = true;
	}

	// Use the timeout from configuration
	if (!isMoving && !button_is_pressed() && (millis() - lastMovementTime > currentConfig.deepSleepTimeout)) {
		power_shutdown();
	}
}

void StateMachine::checkCloseBy() {
	if (this->currentState.entanglementState != EntanglementState::PURE
		&& this->currentState.entanglementState != EntanglementState::POST_ENTANGLEMENT
		&& this->currentState.entanglementState != EntanglementState::TELEPORTED
		&& this->currentState.entanglementState != EntanglementState::ENTANGLED) {
		return;
	}

	if (this->new_peer_rssi < currentConfig.rssiLimit) {
		return;
	}

	if (this->new_peer_rssi > -1) {
		return;
	}

	if (memcmp((void *)this->new_peer, (void *)this->current_peer, MAC_ADDRESS_LENGTH) == 0) {
		return;
	}

	if (memcmp((void *)this->new_peer, (void *)this->next_peer, MAC_ADDRESS_LENGTH) == 0) {
		return;
	}

	debugln("Nearby dice detected - sending entanglement request and returning to IDLE.");
	memcpy((void *)this->next_peer, (void *)this->new_peer, MAC_ADDRESS_LENGTH);
	debugf(
		"Adding peer (next_peer): %02X:%02X:%02X:%02X:%02X:%02X\n",
		this->next_peer[0],
		this->next_peer[1],
		this->next_peer[2],
		this->next_peer[3],
		this->next_peer[4],
		this->next_peer[5]
	);

	this->new_peer_rssi = INT32_MIN;

	if (this->currentState.entanglementState == EntanglementState::ENTANGLED) {
		sendTeleportRequest(this->new_peer, this->current_peer);
	} else {
		sendEntangleRequest(this->new_peer);
		changeState(Trigger::CLOSE_BY);
	}
}

// ============================================================================
// STATE HANDLER IMPLEMENTATIONS
// ============================================================================

// === CLASSIC MODE ===

void StateMachine::enterClassicIdle() {
	debugln("=== Entering CLASSIC MODE ===");
	stateEntryTime = millis();

	// Initialize display state
	this->selfDiceNumber = DiceNumbers::NONE;
	this->selfUpSide = UpSide::NONE;
	this->selfMeasurementAxis = MeasuredAxises::UNDEFINED;

	sendWatchDog();
	refreshScreens();
}

void StateMachine::whileClassicIdle() {
	// Check for button press to switch to quantum mode
	if (button_poll_press_short()) {
		debugln("Button pressed - switching to QUANTUM mode");
		changeState(Trigger::BUTTON_PRESSED);
	}
}

// === QUANTUM MODE - IDLE ===

void StateMachine::enterQuantumIdle() {
	debugln("=== Entering QUANTUM IDLE ===");
	stateEntryTime = millis();

	// Reset tumble detection for next throw
	_imuSensor->resetTumbleDetection();

	sendWatchDog();
	refreshScreens();
}

void StateMachine::whileQuantumIdle() {
	// Check if dice is being thrown
	if (_imuSensor->tumbled()) {
		debugln("Tumble detected - starting throw");
		changeState(Trigger::START_ROLLING);
		return;
	}

	// Waiting for partner confirmation.
	// This state just waits and can timeout.
	if (this->currentState.entanglementState == EntanglementState::ENTANGLE_REQUESTED) {
		if (millis() - stateEntryTime > MAXENTANGLEDWAITTIME) {
			debugln("Entanglement request timeout - returning to PURE state");
			changeState(Trigger::TIMED);
			return;
		}
	}
}

// === QUANTUM MODE - THROWING ===

void StateMachine::enterThrowing() {
	debugln("=== Dice is THROWING ===");
	stateEntryTime = millis();

	refreshScreens();
	sendWatchDog();
}

void StateMachine::whileThrowing() {
	// Check if dice has landed and is stable
	if (_imuSensor->stable() && _imuSensor->on_table()) {
		debugln("Dice stable and on table - moving to OBSERVED");
		changeState(Trigger::STOP_ROLLING);
		return;
	}
}

// === QUANTUM MODE - OBSERVED (MEASUREMENT) ===

void StateMachine::enterObserved() {
	debugln("=== Dice OBSERVED - Processing measurement ===");
	stateEntryTime = millis();

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
			this->selfMeasurementAxis = MeasuredAxises::ZAXIS;
			this->selfUpSide = UpSide::Z0;
			debugln("Measured: Z+ axis");
			break;

		case IMU_Orientation::ORIENTATION_Z_DOWN:
			this->selfMeasurementAxis = MeasuredAxises::ZAXIS;
			this->selfUpSide = UpSide::Z1;
			debugln("Measured: Z- axis");
			break;

		case IMU_Orientation::ORIENTATION_X_UP:
			this->selfMeasurementAxis = MeasuredAxises::XAXIS;
			this->selfUpSide = UpSide::X1; // Inverted: X_UP maps to X1
			debugln("Measured: X+ axis");
			break;

		case IMU_Orientation::ORIENTATION_X_DOWN:
			this->selfMeasurementAxis = MeasuredAxises::XAXIS;
			this->selfUpSide = UpSide::X0; // Inverted: X_DOWN maps to X0
			debugln("Measured: X- axis");
			break;

		case IMU_Orientation::ORIENTATION_Y_UP:
			this->selfMeasurementAxis = MeasuredAxises::YAXIS;
			this->selfUpSide = UpSide::Y0;
			debugln("Measured: Y+ axis");
			break;

		case IMU_Orientation::ORIENTATION_Y_DOWN:
			this->selfMeasurementAxis = MeasuredAxises::YAXIS;
			this->selfUpSide = UpSide::Y1;
			debugln("Measured: Y- axis");
			break;

		case IMU_Orientation::ORIENTATION_TILTED: [[fallthrough]]
		case IMU_Orientation::ORIENTATION_UNKNOWN:
			debugln("No clear axis - measurement failed");
			changeState(Trigger::MEASURE_FAIL);
			return;
    }

	// Determine the dice number based on entanglement state
	switch (this->currentState.entanglementState) {
		case EntanglementState::PURE:
			// Check if we're measuring in the same basis as the last roll
			if (this->selfMeasurementAxis == lastRollBasis && lastRollNumber != DiceNumbers::NONE) {
				// Same basis - return the memoized value
				debugln("PURE state: same basis as last roll, using memoized value");
				this->selfDiceNumber = lastRollNumber;
			} else {
				// Different basis or first roll - generate new random number
				debugln("PURE state: generating random number");
				this->selfDiceNumber = static_cast<DiceNumbers>(generateDiceRoll());

				// Update memorization
				this->lastRollBasis  = this->selfMeasurementAxis;
				this->lastRollNumber = this->selfDiceNumber;
			}
			break;

		case EntanglementState::ENTANGLED:
			// We measured first - generate random and send to partner
			debugln("ENTANGLED state: we measured first");
			this->selfDiceNumber = static_cast<DiceNumbers>(generateDiceRoll());

			// Send our measurement to partner
			sendMeasurements(
				this->current_peer,
				this->currentState,
				this->selfDiceNumber,
				this->selfUpSide,
				this->selfMeasurementAxis
			);

			// Update memoization
			this->lastRollBasis  = this->selfMeasurementAxis;
			this->lastRollNumber = this->selfDiceNumber;

			// Clear entanglement
			this->currentState.entanglementState = EntanglementState::PURE;
			memset(this->current_peer, 0xFF, MAC_ADDRESS_LENGTH);
			break;

		case EntanglementState::POST_ENTANGLEMENT:
			// Partner measured first - check if same axis
			debugln("POST_ENTANGLEMENT state: partner measured first");

			if (this->selfMeasurementAxis == partnerMeasurementAxis) {
				// Same measurement basis - show opposite value (sum = 7)
				debugln("Same axis as partner - showing opposite value");
				this->selfDiceNumber = selectOppositeOneToSix(partnerDiceNumber);
			} else {
				// Different measurement basis - random value
				debugln("Different axis from partner - random value");
				this->selfDiceNumber = static_cast<DiceNumbers>(generateDiceRoll());
			}

			// Update memoization
			this->lastRollBasis = this->selfMeasurementAxis;
			this->lastRollNumber = this->selfDiceNumber;

			// Clear partner info and entanglement
			this->partnerMeasurementAxis = MeasuredAxises::UNDEFINED;
			this->partnerDiceNumber = DiceNumbers::NONE;
			this->currentState.entanglementState = EntanglementState::PURE;
			break;

		case EntanglementState::TELEPORTED:
			// Received teleported state - check if same axis as teleported measurement
			debugln("TELEPORTED state: checking measurement axis");

  			if (this->selfMeasurementAxis == teleportedMeasurementAxis) {
				// Same measurement basis - show teleported value
				debugln("Same axis as teleported state - showing teleported value");
				this->selfDiceNumber = teleportedDiceNumber;
			} else {
				// Different measurement basis - random value (collapses teleported state)
				debugln("Different axis from teleported state - random value");
				this->selfDiceNumber = static_cast<DiceNumbers>(generateDiceRoll());
			}

			// Update memoization
			this->lastRollBasis  = this->selfMeasurementAxis;
			this->lastRollNumber = this->selfDiceNumber;

			// Clear teleported info
			this->teleportedMeasurementAxis = MeasuredAxises::UNDEFINED;
			this->teleportedDiceNumber = DiceNumbers::NONE;
			this->currentState.entanglementState = EntanglementState::PURE;
			break;

		case EntanglementState::ENTANGLE_REQUESTED:
			// Shouldn't happen, but treat as PURE
			this->selfDiceNumber = static_cast<DiceNumbers>(generateDiceRoll());
			break;
	}

	// Reset tumble detection so we're ready for the next throw
	_imuSensor->resetTumbleDetection();

	refreshScreens();
	sendWatchDog();
}

void StateMachine::whileObserved() {
	// Check if dice is being thrown again
	if (_imuSensor->tumbled()) {
		debugln("Tumble detected - starting new throw");
		changeState(Trigger::START_ROLLING);
		return;
	}
}

// === LOW BATTERY ===

void StateMachine::enterLowBattery() {
	debugln("=== LOW BATTERY STATE ===");
	this->stateEntryTime = millis();

	this->selfDiceNumber = DiceNumbers::NONE;
	this->selfUpSide = UpSide::NONE;
	this->selfMeasurementAxis = MeasuredAxises::UNDEFINED;

	sendWatchDog();
	refreshScreens();
}

void StateMachine::whileLowBattery() {
	// Display battery indicator
	voltageIndicator(XX);
}
