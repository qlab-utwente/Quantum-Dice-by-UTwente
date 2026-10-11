# Quantum Dice - Technical Documentation

**Version:** Gen 4.0
**Platform:** ESP32-S3 N16R8
**Framework:** Arduino

---

## Table of Contents

1. [System Overview](#1-system-overview)
2. [Hardware Architecture](#2-hardware-architecture)
3. [Software Architecture](#3-software-architecture)
4. [State Machine](#4-state-machine)
5. [Quantum Mechanics Implementation](#5-quantum-mechanics-implementation)
6. [Communication Protocol](#6-communication-protocol)
7. [Configuration System](#7-configuration-system)
8. [Motion Detection](#8-motion-detection)
9. [Display System](#9-display-system)
10. [Data Structures](#10-data-structures)
11. [Code Modules](#11-code-modules)

---

## 1. System Overview

The Quantum Dice is an interactive educational device that demonstrates quantum mechanical principles through a physical dice system. It operates in two modes:

- **Classic Mode**: Functions as a traditional random dice
- **Quantum Mode**: Demonstrates quantum entanglement, superposition, measurement, and teleportation

### Key Features

- Quantum entanglement between multiple dice
- Quantum teleportation between three dice (Alice, Bob, and Measurement)
- Measurement basis correlation
- State persistence (memorization)
- Visual feedback through six circular TFT displays
- Battery-powered with deep sleep capability
- ESP-NOW wireless communication

---

## 2. Hardware Architecture

### Core Components

| Component | Model/Type | Interface | Purpose |
|-----------|-----------|-----------|---------|
| **Microcontroller** | ESP32-S3 N16R8 | - | Main processor with 16MB Flash, 8MB PSRAM |
| **IMU Sensor** | Adafruit LSM6DS3 | I2C | IMU |
| **Displays (6x)** | GC9A01A Round TFT | SPI | Visual state representation |
| **Button** | - | GPIO_NUM_14 | Mode switching, UI control |
| **Power** | Li-Po Battery | MAX17048 | Battery voltage monitoring |
| **Communication** | ESP-NOW | WiFi (channel 6) | Peer-to-peer messaging |

### Pin Assignments

The system uses dynamic pin mapping loaded from configuration:

- **Button Input**: GPIO_NUM_14 (BUTTON_PIN) - user interaction
- **SPI Bus**: Shared among all six displays
- **Display CS Pins**: One pin for each display
- **I2C Bus**: LSM6DS3 and MAX17048 communication

### Display Configuration

Six displays arranged to represent the six faces of a dice:
- **X-axis**: X0, X1 (opposing sides)
- **Y-axis**: Y0, Y1 (opposing sides)
- **Z-axis**: Z0, Z1 (opposing sides)

---

## 3. Software Architecture

### Initialization Sequence

The `setup()` function follows a rough four-step initialization:

```
1. Power Management
   └─ Enable regulator pin immediately

2. Filesystem & Configuration
   ├─ Initialize LittleFS
   ├─ Auto-detect config file
   ├─ Load configuration
   └─ Validate checksum

3. Hardware Initialization
   ├─ Configure GPIO pins
   ├─ Initialize displays
   ├─ Configure button handler
   ├─ Initialize IMU sensor
   └─ Apply axis remapping (NANO variant)

4. State Machine Setup
   ├─ Initialize ESP-NOW
   └─ Enter initial state (CLASSIC/IDLE/PURE)
```

### Main Loop

The `loop()` function operates at **50ms intervals** (UPDATE_INTERVAL):

```cpp
void loop() {
    IMU.update();            // Process IMU measurements
    stateMachine.update();   // Execute state machine logic
}
```

---

## 4. State Machine

### State Representation

The system state is defined by three orthogonal dimensions:

```cpp
struct State {
    Mode              mode;              // CLASSIC, QUANTUM, LOW_BATTERY
    ThrowState        throwState;        // IDLE, THROWING, OBSERVED
    EntanglementState entanglementState; // PURE, ENTANGLE_REQUESTED,
                                         // ENTANGLED, POST_ENTANGLEMENT,
                                         // TELEPORTED
};
```

### Modes

| Mode | Description |
|------|-------------|
| `CLASSIC` | Traditional random dice, no quantum behavior |
| `QUANTUM` | Full quantum behavior enabled |
| `LOW_BATTERY` | Power-saving mode, prompts for charging |

### Throw States

| State | Description | Entry Condition |
|-------|-------------|-----------------|
| `IDLE` | Dice at rest, awaiting action | Dice stable on table |
| `THROWING` | Dice in motion | Motion detected (tumbling) |
| `OBSERVED` | Measurement complete | Dice stopped and on table |

### Entanglement States

| State | Description |
|-------|-------------|
| `PURE` | No entanglement, superposition |
| `ENTANGLE_REQUESTED` | Awaiting partner confirmation |
| `ENTANGLED` | Quantum correlation active |
| `POST_ENTANGLEMENT` | Partner measured, awaiting our measurement |
| `TELEPORTED` | Received teleported state |

### State Transitions

The system defines **37 state transitions** triggered by various events:

#### Trigger Types

| Category | Triggers |
|----------|----------|
| **User** | `BUTTON_PRESSED` |
| **Motion** | `START_ROLLING`, `STOP_ROLLING` |
| **Entanglement** | `CLOSE_BY`, `ENTANGLE_REQUEST`, `ENTANGLE_CONFIRM`, `ENTANGLE_STOP` |
| **Measurement** | `MEASURE`, `MEASURE_FAIL`, `MEASUREMENT_RECEIVED` |
| **Teleportation** | `TELEPORT_INITIATED`, `TELEPORT_CONFIRMED`, `TELEPORT_RECEIVED` |
| **System** | `TIMED`, `LOW_BATTERY` |

#### Key Transitions

**Classic ↔ Quantum Mode Switching:**
```
CLASSIC/IDLE/PURE + BUTTON_PRESSED → QUANTUM/IDLE/PURE
QUANTUM/IDLE/PURE + BUTTON_PRESSED → CLASSIC/IDLE/PURE
```

**Entanglement Establishment:**
```
QUANTUM/IDLE/PURE + CLOSE_BY → QUANTUM/IDLE/ENTANGLE_REQUESTED
QUANTUM/IDLE/ENTANGLE_REQUESTED + ENTANGLE_CONFIRM → QUANTUM/IDLE/ENTANGLED
```

**Measurement Flow:**
```
QUANTUM/IDLE/* + START_ROLLING → QUANTUM/THROWING/*
QUANTUM/THROWING/* + STOP_ROLLING → QUANTUM/OBSERVED/*
QUANTUM/OBSERVED/PURE + START_ROLLING → QUANTUM/THROWING/PURE
```

**Teleportation (3-dice system: Alice-Bob-Measurement):**
```
Alice (ENTANGLED) detects nearby M → sends TELEPORT_REQUEST
M receives request → sends TELEPORT_PAYLOAD to Bob → sends TELEPORT_CONFIRM to Alice
Bob receives payload → inherits M's state (may include entanglement to N)
```

### State Functions

Each valid state combination maps to two functions:

```cpp
struct StateFunction {
    void (StateMachine::*onEntry)();      // Called once on state entry
    void (StateMachine::*whileInState)(); // Called repeatedly while in state
};
```

---

## 5. Quantum Mechanics Implementation

### Superposition

When a quantum dice is unobserved (IDLE or THROWING in PURE state), it displays a superposition pattern on all six faces, representing that the outcome is undefined until measured.

### Measurement & Collapse

When the dice stops and becomes stable on a table:

1. **Axis Detection**: IMU determines which face is up (X0, X1, Y0, Y1, Z0, or Z1)
2. **Basis Selection**: The up-face determines the measurement basis (X, Y, or Z axis)
3. **Wave Function Collapse**: A random number (1-6) is generated
4. **State Display**: The measured face shows the number, others show superposition patterns

### Entanglement

When two dice are brought close together (RSSI > configured limit):

```
Dice A (PURE) detects Dice B → sends ENTANGLE_REQUEST
Dice B (PURE) receives request → sends ENTANGLE_CONFIRM with random color
Both dice → ENTANGLED state with shared color
```

**Entanglement Correlation:**

When entangled dice are measured:
- **Same Basis**: Results are **anti-correlated** (opposite faces: 1↔6, 2↔5, 3↔4)
- **Different Basis**: Results are **uncorrelated** (independent random)

**Implementation:**
```cpp
// First dice to measure
diceNumberSelf = selectOneToSix();
sendMeasurements(partner, ...);

// Partner dice measuring after
if (measureAxisSelf == partnerMeasurementAxis) {
    // Same basis - anti-correlated
    diceNumberSelf = selectOppositeOneToSix(partnerDiceNumber);
} else {
    // Different basis - uncorrelated
    diceNumberSelf = selectOneToSix();
}
```

### Memoization (Measurement Consistency)

To maintain quantum mechanical consistency, the system implements **measurement memoization**:

```cpp
// Store last measurement
lastRollBasis  = measureAxisSelf;   // Which axis was measured
lastRollNumber = diceNumberSelf;    // What value was observed

// On subsequent measurement in same basis
if (measureAxisSelf == lastRollBasis && lastRollNumber != NONE) {
    diceNumberSelf = lastRollNumber;  // Return same value
} else {
    diceNumberSelf = selectOneToSix(); // New random value
}
```

This ensures that repeatedly measuring the same face yields the same result, consistent with quantum mechanics.

### Quantum Teleportation

The system implements simplified quantum teleportation with three dice:

**Roles:**
- **Alice (A)**: Has entangled pair with Bob, initiates teleportation
- **Bob (B)**: Entangled with Alice, receives teleported state
- **Measurement (M)**: The dice to be teleported

**Protocol:**

1. **A and B are entangled** (share color)
2. **A detects M nearby** → sends `TELEPORT_REQUEST(target=B)` to M
3. **M sends its state** → `TELEPORT_PAYLOAD` to B, `TELEPORT_CONFIRM` to A
4. **State Transfer:**
   - If M was PURE: B becomes PURE
   - If M was OBSERVED: B receives teleported measurement (shown only in same basis)
   - If M was ENTANGLED to N: B becomes entangled to N (inherits color)
5. **Cleanup:** M returns to PURE, A-B entanglement ends

**Special Case - Entanglement Swapping:**

If M was entangled to another dice N:
```
Before: A-B entangled, M-N entangled
After:  B-N entangled, A and M are PURE
```

This is implemented via `TELEPORT_PARTNER` message to N.

---

## 6. Communication Protocol

### ESP-NOW Configuration

- **Channel**: 6 (ESPNOW_WIFI_CHANNEL)
- **Transport**: Peer-to-peer, WiFi-based
- **Encryption**: None (broadcast discovery)

### Message Structure

```cpp
struct message {
    message_type type;
    union data {
        watchDogData;        // Periodic presence broadcast
        measurementData;     // Measurement result sharing
        entangleConfirmData; // Entanglement color negotiation
        teleportRequestData; // Teleportation request (contains target MAC)
        teleportPayloadData; // Complete state transfer
        teleportPartnerData; // Entanglement partner update
    } data;
};
```

### Message Types

| Type | Direction | Purpose | Data Payload |
|------|-----------|---------|--------------|
| `WATCH_DOG` | Broadcast | Presence advertisement | Current state |
| `MEASUREMENT` | Peer → Peer | Share observation result | State, axis, number, upside |
| `ENTANGLE_REQUEST` | Initiator → Target | Request entanglement | None |
| `ENTANGLE_CONFIRM` | Target → Initiator | Accept entanglement | RGB565 color |
| `ENTANGLE_DENIED` | Target → Initiator | Reject entanglement | None |
| `TELEPORT_REQUEST` | Alice → M | Initiate teleportation | Bob's MAC address |
| `TELEPORT_CONFIRM` | M → Alice | Teleportation complete | None |
| `TELEPORT_PAYLOAD` | M → Bob | Transfer complete state | State, measurement, entanglement |
| `TELEPORT_PARTNER` | M → N | Update entanglement partner | New partner MAC |

### RSSI-Based Proximity Detection

```cpp
// Typical configuration
rssiLimit = config.rssiLimit // dBm threshold for "close by"

// Detection logic
if (last_rssi > rssiLimit && last_rssi < -1) {
    // Dice detected nearby
    sendEntangleRequest(last_source);
}
```

### Watchdog Mechanism

Every **500ms** in QUANTUM mode, each dice broadcasts its state:

```cpp
sendWatchDog();  // Broadcasts current State
```

This enables:
- Proximity detection for entanglement
- State synchronization
- Partner monitoring

---

## 7. Configuration System

### Configuration File Structure

Text-based configuration file (e.g., `EXAMPLE_config.txt`):

```ini
# Dice Identification
diceId=EXAMPLE

# Display Background Colors (RGB565 format)
x_background=0x001F    # Blue
y_background=0xF800    # Red
z_background=0xFFE0    # Yellow

# Entanglement Colors (up to 8)
entang_colors=0xFFE0,0x07E0,0x07FF,0xF81F

# Timing Configuration
colorFlashTimeout=250    # Color flash duration (ms)
deepSleepTimeout=300000   # Deep sleep after 5 min idle (ms)

# Communication
rssiLimit=-35             # RSSI threshold for proximity (dBm)
```

### DiceConfig Structure

```cpp
struct DiceConfig {
    String   diceId;                                    // Unique identifier
    uint16_t x_background, y_background, z_background;  // RGB565 colors
    std::array<uint16_t, 8> entang_colors;              // Palette
    uint8_t  entang_colors_count;                       // Active colors
    uint16_t colorFlashTimeout;                         // Flash duration
    int8_t   rssiLimit;                                 // Proximity threshold
    uint32_t deepSleepTimeout;                          // Power saving
    uint8_t  checksum;                                  // Validation
};
```

### Auto-Configuration Process

```cpp
bool ensureLittleFSAndConfig() {
    1. Mount LittleFS filesystem
    2. Auto-detect config file (*_config.txt)
    3. Load configuration
    4. Validate checksum
    5. If failed: create default config
}
```

### Hardware Pin Mapping

Pins are configured dynamically based on board type:

```cpp
struct HardwarePins {
    uint8_t tft_cs, tft_rst, tft_dc;    // Common TFT control
    uint8_t screen_cs[6];                // Individual display CS
    uint8_t screenAddress[16];           // Display mapping
    uint8_t adc_pin;                     // Battery monitoring
};
```

Variants:
- **NANO + SMD**: Specific pin layout for Arduino Nano ESP32 with SMD screens
- **NANO + HDR**: Header-based connections
- **DEVKIT + SMD/HDR**: ESP32-S3 DevKit variants

---

## 8. Motion Detection

### IMU Sensor

The system uses a singleton:

```cpp
IMU.moving();
IMU.stable();
IMU.onTable();
IMU.getOrientation();
IMU.resetTumbleDetection();
IMU.tumbled();
```

### Motion Detection Algorithm

**Parameters:**
- `motionThreshold`: 0.5 m/s² (significant acceleration change)
- `stableThreshold`: 0.15 m/s² (minimal movement)
- `stableCountRequired`: 10 consecutive stable samples

**Logic:**
```cpp
gyroMag = sqrtf(gyro.dotProduct(gyro));

if (gyroMag > motionThreshold) {
    isMoving = true;
    stableCounter = 0;
} else if (gyroMag < stableThreshold) {
    stableCounter++;
    if (stableCounter >= stableCountRequired) {
        isMoving = false;  // Confirmed stable
    }
}
```

### Orientation Detection

Determines which face is up by analyzing gravity vector:

```cpp
enum class IMUOrientation : uint8_t {
	/**
	 * The orientation is unknown.
	 */
	UNKNOWN,

	/**
	 * The die is tilted.
	 *
	 * No face of the die is pointing up.
	 */
	TILTED,

	/**
	 * The Z+ face of the die is pointing up.
	 */
	Z_POS,

	/**
	 * The Z- face of the die is pointing up.
	 */
	Z_NEG,

	/**
	 * The Y+ face of the die is pointing up.
	 */
	Y_POS,

	/**
	 * The Y- face of the die is pointing up.
	 */
	Y_NEG,

	/**
	 * The X+ face of the die is pointing up.
	 */
	X_POS,

	/**
	 * The X- face of the die is pointing up.
	 */
	X_NEG
};
```

**Detection Criteria:**
- Dominant axis must have 9.0–10.5 m/s² (gravity)
- Other axes must be < 2.0 m/s²
- Otherwise classified as TILTED or UNKNOWN

### Tumble Detection

Uses rotation matrix integration to track orientation change:

```cpp
void resetTumbleDetection() {
    // Capture current "up" direction
	this->_upStartX = -(this->_gravityX / _gravityMag);
	this->_upStartY = -(this->_gravityY / _gravityMag);
	this->_upStartZ = -(this->_gravityZ / _gravityMag);
}

void update() {
	...

    // Tumble detection logic.
	if (this->_tumbleReferenceSet) {
		// Calculate dot product between current and initial up vectors
		float upMag = sqrtf(this->_upX * this->_upX + this->_upY * this->_upY + this->_upZ * this->_upZ);
		float upStartMag = sqrtf(this->_upStartX * this->_upStartX + this->_upStartY * this->_upStartY + this->_upStartZ * this->_upStartZ);
		float dotProduct = (this->_upX * this->_upStartX + this->_upY * this->_upStartY + this->_upZ * this->_upStartZ) / (upMag * upStartMag);

		// Check if tumbled beyond threshold.
		if (dotProduct < this->_tumbleThreshold) {
			this->_tumbled = true;
		}
	}
}
```

**Advantages:**
- Rotation-axis independent (ignores spinning on table)
- Detects only actual rolling/tumbling motion
- Configurable threshold angle

---

## 9. Display System

### Hardware Configuration

Six **GC9A01A** round TFT displays (240×240 pixels each):

```cpp
// CS done separately.
Adafruit_GC9A01A tft0(-1, SCREEN_DC, SCREEN_CS);
```

### Screen Selection System

Screens are addressed via bitmask enum:

```cpp
enum screenselections : uint8_t {
    X0 = 0b00000100,
    X1 = 0b00010000,
    Y0 = 0b00001000,
    Y1 = 0b00000010,
    Z0 = 0b00100000,
    Z1 = 0b00000001,
    XX = X0 | X1,
    YY = Y0 | Y1,
    ZZ = Z0 | Z1,
    XXYY = XX | YY,
    XXZZ = XX | ZZ,
    YYZZ = YY | ZZ,
    ODD = X0 | Y0 | Z0,
    EVEN = X1 | Y1 | Z1,
    ALL = ODD | EVEN,
    NO_ONE = 0
};
```

### Display State Mapping

Each face shows a specific `ScreenState`:

```cpp
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
```

### Rendering Pipeline

```cpp
void refreshScreens() {
    // set screens, depending of states
    if (determineScreenStates(stateMachine.getCurrentState(), stateMachine.getDiceNumberSelf(), stateMachine.getUpSideSelf(), x0ReqScreenState, x1ReqScreenState,
                   y0ReqScreenState, y1ReqScreenState, z0ReqScreenState, z1ReqScreenState)) {
        checkAndCallFunctions(x0ReqScreenState, x1ReqScreenState, y0ReqScreenState,
                              y1ReqScreenState, z0ReqScreenState, z1ReqScreenState);
    } else {
        debugln("no match found");
        // No match found, handle accordingly
    }
}
```

### Visual Feedback

| State | Display Pattern | Color |
|-------|----------------|-------|
| Superposition (PURE) | Mixed dots all faces | White |
| Entangled (superposition) | Mixed dots all faces | Entanglement color |
| Observed (number) | Dots on measured face | White or color |
| Low Battery | Battery icon | Red |
| Entanglement flash | Brief color pulse | Entanglement color |

### Color System

- **RGB565 Format**: 16-bit color (5-bit red, 6-bit green, 5-bit blue)
- **Background Colors**: Configurable per axis (x_background, y_background, z_background)
- **Entanglement Colors**: Up to 8 colors, randomly selected when establishing entanglement
- **Color Flash**: Brief display on entanglement events (duration: `colorFlashTimeout`)

---

## 10. Data Structures

### Global State Variables

```cpp
// Current dice state
State stateSelf;           // Our current state
State stateSister;         // Partner's last known state

// Measurement values
DiceNumbers    diceNumberSelf;     // Our observed number (1-6 or NONE)
DiceNumbers    diceNumberSister;   // Partner's number
MeasuredAxises measureAxisSelf;    // Which axis we measured (X/Y/Z)
UpSide         upSideSelf;         // Which specific face is up

// Entanglement
uint16_t entanglement_color_self;  // RGB565 color for entanglement
bool     showColors;               // Toggle for color display
bool     flashColor;               // Temporary color flash active
```

### Enumerations

**DiceNumbers:**
```cpp
enum class DiceNumbers : uint8_t {
    NONE, ONE, TWO, THREE, FOUR, FIVE, SIX
};
```

**MeasuredAxises:**
```cpp
enum class MeasuredAxises : uint8_t {
    UNDEFINED,  // Not yet measured
    XAXIS,      // X-axis (X0 or X1 face up)
    YAXIS,      // Y-axis (Y0 or Y1 face up)
    ZAXIS,      // Z-axis (Z0 or Z1 face up)
    ALL,        // Special: all axes
    NA          // Not applicable
};
```

**UpSide:**
```cpp
enum class UpSide : uint8_t {
    NONE,  // Unknown
    X0, X1, Y0, Y1, Z0, Z1,  // Specific faces
    ANY,   // Any face
    NA     // Not applicable
};
```

### StateTransition Structure

```cpp
struct StateTransition {
    // Current state conditions
    std::optional<Mode>              currentMode;
    std::optional<ThrowState>        currentThrowState;
    std::optional<EntanglementState> currentEntanglementState;

    // Next state (if transition occurs)
    std::optional<Mode>              nextMode;
    std::optional<ThrowState>        nextThrowState;
    std::optional<EntanglementState> nextEntanglementState;

    // Trigger that causes transition
    Trigger trigger;
};
```

---

## 11. Code Modules

### Main.ino

**Purpose**: Main sketch entry point
**Key Functions**:
- `setup()`: Four-stage initialization (power, config, hardware, state machine)
- `loop()`: 50ms update cycle for button and state machine

### StateMachine.hpp / .cpp

**Purpose**: Core state machine implementation
**Key Components**:
- State representation and transitions (37 transitions)
- State handler functions (onEntry, whileInState)
- Message processing logic
- Entanglement and teleportation protocols
**Lines**: ~1246 lines

### DiceConfigManager.hpp / .cpp

**Purpose**: Configuration file management
**Key Functions**:
- `load()`: Parse text config file
- `ensureLittleFSAndConfig()`: Auto-detect and load config
- `createDefaultConfigFile()`: Generate default config
**Features**:
- Checksum validation
- Comment and whitespace handling
- Default value initialization

### IMU.hpp / .cpp

**Purpose**: IMU singleton
**Classes**:
- `IMUClass`: Singleton class
**Key Functions**:
- Motion detection (`moving()`, `stable()`)
- Orientation detection (`orientation()`, `on_table()`)
- Tumble tracking (`resetTumbleDetection()`, `tumbled()`)
- Gyroscope integration for rotation matrices

### EspNowSensor.hpp

**Purpose**: Template-based ESP-NOW communication wrapper
**Design Pattern**: Singleton with static interface
**Key Functions**:
- `Init()`: Initialize ESP-NOW and WiFi
- `Send()`: Transmit message to peer
- `Poll()`: Retrieve received messages from queue
- `AddPeer()`: Register communication partner
**Features**:
- RSSI extraction from packet headers
- Message queuing
- Callback handling

### Screenfunctions.hpp / .cpp

**Purpose**: Display management and rendering
**Key Functions**:
- `initDisplays()`: Initialize six GC9A01A displays
- `refreshScreens()`: Update all displays based on state
- `display1to6()`: Show dice numbers
- `displayMix1to6()`: Superposition pattern
- `displayEntangled()`: Entanglement indicator
**Utilities**:
- `blendColor()`: Alpha blending for smooth visuals
- `drawDot()`: Draw dice dots
- `selectScreens()`: Bitmask-based screen selection

### ScreenStateDefs.hpp / .cpp

**Purpose**: Screen state determination logic
**Key Functions**:
- `findValues()`: Map quantum state → screen states for each face
- `selectOneToSix()`: Random number generation
- `selectOppositeOneToSix()`: Anti-correlation for entangled dice

### Button.hpp / .cpp

**Purpose**: Button singleton for interacting with button.
**Key Functions**:
- `pollPressShort()`: Get whether a short press has been registered since last call.
- `pollPressDouble()`: Get whether a double press has been registered since last call.
- `pollPressLong()`: Get whether a long press has been registered since last call.

### Battery.hpp / .cpp

**Purpose**: Battery singleton for interacting with the button.
**Key Functions**:
- `getVoltage()`: Returns the voltage of the battery.
- `getStateOfCharge()`: Returns the state of charge of the battery.s

### Queue.hpp

**Purpose**: Generic FIFO queue implementation
**Template Class**: `Queue<T>`
**Features**:
- Dynamic resizing
- Circular buffer
- Type-safe

### defines.hpp

**Purpose**: System-wide constants and debug macros
**Contents**:
- Version string
- Debug/logging macros (debug, info, warn, error)
- Battery voltage thresholds
- Pin definitions

---

## Appendix: Key Algorithms

### Random Number Generation

```cpp
uint8_t generateDiceRoll() {
    // Use ESP32 hardware RNG
    uint32_t random_value = esp_random();
    return (random_value % 6) + 1;  // Returns 1-6
}
```

### Anti-Correlation Mapping

```cpp
DiceNumbers selectOppositeOneToSix(DiceNumbers input) {
    // Opposite faces on a dice sum to 7
    switch (input) {
        case ONE:   return SIX;
        case TWO:   return FIVE;
        case THREE: return FOUR;
        case FOUR:  return THREE;
        case FIVE:  return TWO;
        case SIX:   return ONE;
    }
}
```

---

## Performance Characteristics

| Metric | Value | Notes |
|--------|-------|-------|
| Main loop frequency | 20 Hz | 50ms update interval |
| Watchdog broadcast | 2 Hz | Every 500ms (quantum mode only) |
| State machine transitions | <1ms | Typical transition time |
| Display refresh | ~100ms | Full six-screen update |
| Motion detection latency | 50-250ms | Depends on stable count |
| Entanglement discovery | <1s | Proximity-based |
| Configuration load time | <100ms | LittleFS read |

---

## Dependencies

### Arduino Libraries

- **Adafruit BNO055**: IMU sensor driver
- **Adafruit LSM6DS3**: IMU sensor driver
- **Adafruit Unified Sensor**: Unified sensor abstraction
- **Adafruit GFX Library**: Graphics primitives
- **Adafruit_GC9A01A**: Round TFT display driver
- **WiFi**: ESP32 WiFi stack (for ESP-NOW)
- **esp_now**: Peer-to-peer communication
- **LittleFS**: Filesystem for configuration storage

### ESP32 Framework

- **ESP-IDF 5.x** (via Arduino-ESP32 3.3.2)
- **FreeRTOS**: Underlying task scheduler
- **ESP-NOW protocol**: WiFi direct communication

---

## Build Configuration

### Platform Settings

```
Board: ESP32S3 Dev Module
USB CDC On Boot: Enabled
Flash Size: 16MB (128Mb)
PSRAM: OPI PSRAM
Partition Scheme: Custom (partitions.csv)
```

### Custom Partitions

The `partitions.csv` file allocates space for:
- Application firmware
- LittleFS filesystem (for config storage)

---

## Debug Features

### Logging System

Five log levels with formatted output:

```cpp
debug("value: %d\n", x);     // [D] value: 42
info("status: OK\n");        // [L] status: OK
warn("low battery\n");       // [W] low battery
error("failed!\n");          // [E] failed!
```

### State Monitoring

```cpp
void printStateName(const char *objectName, State state) {
    Serial.printf("%s: %s | %s | %s\n",
                  objectName,
                  getModeName(state.mode),
                  getThrowStateName(state.throwState),
                  getEntanglementStateName(state.entanglementState));
}
```

Example output:
```
stateMachine: QUANTUM | IDLE | ENTANGLED
```

---

## Future Enhancement Opportunities

1. **Multiple Entanglement**: Support >2 dice entangled simultaneously
2. **Persistent Entanglement**: Save entanglement state across power cycles
3. **Calibration Wizard**: Interactive IMU calibration sequence
4. **OTA Updates**: Over-the-air firmware updates
5. **Advanced Teleportation**: Full 3-qubit teleportation with basis selection
6. **Data Logging**: Record measurement sequences for education/analysis
7. **Web Interface**: Configuration and monitoring via WiFi AP

---

**Document Version**: 1.1
**Last Updated**: October 2026
**Firmware Version**: Gen 4.0
