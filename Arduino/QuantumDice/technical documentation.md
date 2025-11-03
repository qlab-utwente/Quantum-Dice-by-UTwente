# QuantumDice Technical Documentation

Created by claude.ai *create technical documentation of the sketch*

## Overview

The QuantumDice is an ESP32-based electronic dice system that supports both classical and quantum-entangled modes of operation. Three dice units (labeled A, B1, and B2) can wirelessly communicate using ESP-NOW protocol to achieve quantum-entangled behavior, where paired dice always sum to 7 when measured along the same axis.

**Version:** 1.0.0
**Hardware Platform:** ESP32 (Arduino Nano ESP32 or ESP32S3 Dev Module)
**Compiler Requirements:** ESP32 version 3.3.2, Pin Numbering By GPIO (legacy)

---

## System Architecture

### Hardware Components

1. **Microcontroller:** ESP32 (Nano or DevKit variant)
2. **IMU Sensor:** BNO055 9-axis sensor for motion detection and orientation
3. **Displays:** 6x GC9A01A circular TFT displays (240x240px) - one per die face
4. **Wireless:** ESP-NOW for low-latency inter-dice communication
5. **Random Source:** ATECCX08A hardware random number generator
6. **Power:** Battery-powered with voltage monitoring and deep sleep mode
7. **User Input:** Physical button for mode switching

### File Structure

```
QuantumDice/
├── QuantumDice.ino          # Main sketch entry point
├── Version.h                # Version control
├── defines.h                # Global constants and macros
├── Globals.h                # Global state variables
├── StateMachine.h/cpp       # Core state machine implementation
├── IMUhelpers.h/cpp         # IMU sensor abstraction layer
├── handyHelpers.h/cpp       # Utility functions and EEPROM management
├── EspNowSensor.h           # ESP-NOW communication template
├── ScreenStateDefs.h/cpp    # Screen state definitions and truth tables
├── Screenfunctions.h/cpp    # Display rendering functions
├── ScreenDeterminator.h     # Display update logic
├── Queue.h                  # Generic queue data structure
└── ImageLibrary/            # Image assets for displays
    ├── ImageLibrary.h
    ├── God_does_not_play_dice.h
    ├── UTwente_logo.h
    ├── QRCode.h
    ├── circle.h
    ├── cross.h
    ├── entangled.h
    ├── crossCircle.h
    ├── lowBattery.h
    ├── new_die.h
    └── quantum_labs_twente_RGB.h
```

---

## Core Components

### 1. Main Program Flow (QuantumDice.ino)

#### Setup Sequence

The setup function follows a critical initialization order:

```
1. Power Management    - Keep regulator on (REGULATOR_PIN)
2. Serial Init         - Debug output (115200 baud)
3. EEPROM Init         - Initialize 512-byte EEPROM
4. Config Load         - Load dice configuration from EEPROM
5. Display Init        - Initialize 6 TFT displays
6. Startup Logo        - Show Quantum Lab logo
7. IMU Init            - Initialize BNO055 sensor with calibration
8. Button Init         - Configure button handlers
9. Random Init         - Initialize ATECCX08A RNG
10. State Machine      - Initialize ESP-NOW and state machine
```

**Critical Configuration:** The device cannot operate without valid EEPROM configuration. If configuration loading fails, the device halts with an error message.

#### Main Loop

The loop runs at 50ms intervals (`UPDATE_INTERVAL`):

```cpp
void loop() {
  if (currentTime - lastUpdateTime >= UPDATE_INTERVAL) {
    button.loop();           // Process button events
    stateMachine.update();   // Execute state machine logic
  }
}
```

### 2. State Machine (StateMachine.h/cpp)

The state machine implements the core dice behavior using a table-driven finite state machine pattern.

#### States

| State | Description |
|-------|-------------|
| `IDLE` | Initial boot state, transitions to CLASSIC_STATE after timeout |
| `CLASSIC_STATE` | Traditional dice mode, no quantum features |
| `INITSINGLE` | Initialize single (non-entangled) quantum mode |
| `INITENTANGLED_AB1` | Initialize entanglement between dice A and B1 |
| `INITENTANGLED_AB2` | Initialize entanglement between dice A and B2 |
| `INITSINGLE_AFTER_ENT` | Return to single mode after entanglement ends |
| `WAITFORTHROW` | Ready state, waiting for dice to be thrown |
| `THROWING` | Dice is in motion |
| `INITMEASURED` | Process measurement after dice settles |
| `LOWBATTERY` | Battery critically low |

#### State Transitions

Each state transition is triggered by specific events:

| Trigger | Description |
|---------|-------------|
| `onthemove` | Dice is moving |
| `nonMoving` | Dice has stopped moving |
| `startRolling` | Tumbling motion detected |
| `buttonPressed` | Long button press detected |
| `measureXYZ` | Axis measurement complete |
| `measurementFail` | Measurement failed, retry needed |
| `closeByAB1` | Device B1 is nearby (RSSI > threshold) |
| `closeByAB2` | Device B2 is nearby (RSSI > threshold) |
| `entanglementSucces` | Entanglement established |
| `entanglementFail` | Entanglement failed |
| `entangleStopReceived` | Partner ended entanglement |
| `lowbattery` | Battery voltage below minimum |
| `timed` | Timeout elapsed |

#### Roles

The system supports three dice roles determined by MAC address:

- **ROLE_A:** Primary dice, initiates entanglement requests
- **ROLE_B1:** Secondary dice 1, can entangle with A
- **ROLE_B2:** Secondary dice 2, can entangle with A

Each dice maintains awareness of:
- `roleSelf` - Own role
- `roleSister` - Currently entangled partner
- `roleBrother` - Non-entangled third dice

### 3. IMU System (IMUhelpers.h/cpp)

The IMU system provides motion detection and orientation sensing using the BNO055 sensor.

#### IMUSensor Base Class

Abstract interface for IMU functionality:

```cpp
class IMUSensor {
public:
  virtual void init();
  virtual void update();

  // Motion detection
  bool tumbled(float minRotation);
  bool isMoving();
  bool isNotMoving();

  // Orientation
  float getXGravity();
  float getYGravity();
  float getZGravity();

  void reset();
};
```

#### BNO055IMUSensor Implementation

Implements the IMUSensor interface using the Adafruit BNO055 library.

**Key Features:**
- Automatic calibration data loading from EEPROM (addresses 0x0000-0x001F)
- Tumble detection using quaternion rotation tracking
- Gravity vector measurement for die face detection
- Movement detection with configurable threshold (0.7 m/s²)

**Tumble Detection Algorithm:**

1. Track initial "up" vector at rest
2. Continuously update "up" vector using gyroscope rotation data
3. Calculate rotation angle between initial and current up vectors
4. Trigger tumble when rotation exceeds threshold (configurable, default ~0.5 revolutions)

**Movement Detection:**

Uses linear acceleration magnitude with hysteresis:
- Moving: magnitude > 0.7 m/s²
- Stable: magnitude < 0.7 m/s² for > 200ms

### 4. ESP-NOW Communication (EspNowSensor.h)

Template-based ESP-NOW wrapper providing type-safe messaging.

#### Message Types

```cpp
typedef enum {
  MESSAGE_TYPE_WATCH_DOG,        // Periodic status broadcast
  MESSAGE_TYPE_MEASUREMENT,      // Die result after throw
  MESSAGE_TYPE_ENTANGLE_REQUEST, // Request entanglement
  MESSAGE_TYPE_ENTANGLE_CONFIRM, // Accept entanglement
  MESSAGE_TYPE_ENTANGLE_STOP     // End entanglement
} message_type;
```

#### Message Structure

```cpp
typedef struct {
  message_type type;
  Roles senderRole;
  union {
    struct { State state; } watchDog;
    struct {
      State state;
      DiceStates diceState;
      MeasuredAxises measureAxis;
      DiceNumbers diceNumber;
      UpSide upSide;
    } measurement;
  } data;
} message;
```

#### RSSI-Based Proximity Detection

Entanglement only occurs when dice are physically close:
- Uses promiscuous WiFi mode to capture RSSI
- Configurable threshold (stored in EEPROM, typically -50 dBm)
- Prevents accidental entanglement from a distance

### 5. Display System (Screenfunctions.h/cpp)

#### Display Hardware

Six GC9A01A circular displays (240x240px) are mapped to die faces:

| Screen | Axis Face |
|--------|-----------|
| 0 | X0 (left) |
| 1 | X1 (right) |
| 2 | Y0 (front) |
| 3 | Y1 (back) |
| 4 | Z0 (bottom) |
| 5 | Z1 (top) |

**Note:** Physical mapping differs between SMD and HDR variants, configured in EEPROM.

#### Screen States

Displays can show:
- **Numbers:** 1-6 rendered with dots
- **Symbols:** Circle, Cross, Circle+Cross
- **Animations:** Mixed 1-6 animation (waiting state)
- **Status:** Low battery, entanglement indicators
- **Branding:** University logos, QR codes, Einstein quote

#### Background Colors

Each axis has configurable background colors (RGB565):
- X-axis: Configurable (EEPROM)
- Y-axis: Configurable (EEPROM)
- Z-axis: Configurable (EEPROM)
- Entangled AB1: Configurable (EEPROM)
- Entangled AB2: Configurable (EEPROM)

### 6. Configuration System (handyHelpers.h/cpp)

#### EEPROM Memory Layout

```
Address Range | Size | Content
--------------|------|--------
0x0000-0x0003 | 4B   | BNO055 Sensor ID
0x0004-0x001F | 28B  | BNO055 Calibration Data
0x0020-0x00XX | XXB  | Dice Configuration
```

#### DiceConfig Structure

```cpp
struct DiceConfig {
  char diceId[16];              // "TEST1", "BART1", etc.
  uint8_t deviceA_mac[6];       // MAC of device A
  uint8_t deviceB1_mac[6];      // MAC of device B1
  uint8_t deviceB2_mac[6];      // MAC of device B2
  uint16_t x_background;        // Display colors (RGB565)
  uint16_t y_background;
  uint16_t z_background;
  uint16_t entang_ab1_color;
  uint16_t entang_ab2_color;
  int8_t rssiLimit;             // RSSI threshold (dBm)
  bool isSMD;                   // true=SMD, false=HDR
  bool isNano;                  // true=NANO, false=DEVKIT
  bool alwaysSeven;             // Force sum-to-7 mode
  uint8_t randomSwitchPoint;    // RNG threshold (0-100)
  float tumbleConstant;         // Tumble detection sensitivity
  uint32_t deepSleepTimeout;    // Power-off delay (ms)
  uint8_t checksum;             // Validation checksum
};
```

**Configuration is mandatory** - device will not boot without valid EEPROM data. Use the `QuantumDiceInitTool` sketch to program configuration.

#### HardwarePins Structure

Dynamically configured based on board type:

```cpp
struct HardwarePins {
  uint8_t tft_cs, tft_rst, tft_dc;     // TFT control pins
  uint8_t screen_cs[6];                // Screen chip selects
  uint8_t screenAddress[16];           // Screen multiplexer mapping
  uint8_t adc_pin;                     // Battery voltage sense
};
```

### 7. Random Number Generation

#### Hardware RNG (Primary)

Uses ATECCX08A crypto chip:
- `generateDiceRoll()` - Simple modulo method
- `generateDiceRollRejection()` - Rejection sampling for uniform distribution

**Rejection Sampling:** Requests random bytes until value < 252 (6×42), ensuring no bias in modulo operation.

#### Fallback RNG

If ATECCX08A unavailable, falls back to pseudo-random generator seeded from ADC noise.

---

## Quantum Entanglement Logic

### The "Secret Sauce"

The quantum behavior is implemented in `StateMachine::enterINITMEASURED()` at [StateMachine.cpp:632-813](Arduino/QuantumDice/StateMachine.cpp#L632-L813).

#### Dice Number Selection Algorithm

```
IF diceState == SINGLE:
    Generate random 1-6

ELSE IF diceState == MEASURED:
    IF measureAxis != previousMeasureAxis:
        Generate random 1-6  (different axis = new random)
    ELSE:
        Keep previous number  (same axis = deterministic)

ELSE IF diceState == ENTANGLED_AB1 or ENTANGLED_AB2:
    IF alwaysSeven mode enabled:
        IF partner has measured:
            Select opposite to sum to 7
        ELSE:
            Generate random 1-6
    ELSE:
        IF measureAxis == partnerMeasureAxis:
            Select opposite to sum to 7  (same axis = entangled)
        ELSE:
            Generate random 1-6  (different axis = independent)

ELSE IF diceState == MEASURED_AFTER_ENT:
    IF partner has dice number:
        Use partner's number  (maintain correlation)
    ELSE:
        Generate random 1-6
```

#### Opposite Number Mapping

The function `selectOppositeOneToSix()` maps dice values to ensure sum = 7:

```
1 ↔ 6
2 ↔ 5
3 ↔ 4
```

### Axis Detection

The die uses gravity vector to determine which face is up and which axis is measured.

#### NANO Configuration (IMU on X+ side)

```cpp
if (abs(xGravity) near 9.8):
    measureAxis = Z_AXIS
    upSide = Z0 (if xGravity < 0) else Z1

else if (abs(yGravity) near 9.8):
    measureAxis = X_AXIS
    upSide = X0 (if yGravity < 0) else X1

else if (abs(zGravity) near 9.8):
    measureAxis = Y_AXIS
    upSide = Y0 (if zGravity < 0) else Y1
```

#### DEVKIT Configuration (IMU on Y- side)

Different gravity-to-axis mappings account for IMU mounting orientation.

**Acceptance Range:** Gravity magnitude must be within 9.0-10.5 m/s² for valid measurement.

### Entanglement Establishment Protocol

#### Sequence for A-B1 Entanglement

```
1. A enters WAITFORTHROW state
2. A periodically broadcasts ENTANGLE_REQUEST to B1 (500ms interval)
3. B1 receives request
4. B1 checks RSSI with IsCloseBy()
5. IF RSSI > threshold:
   - B1 sends ENTANGLE_CONFIRM to A
   - B1 transitions to INITENTANGLED_AB1
6. A receives ENTANGLE_CONFIRM
7. A transitions to INITENTANGLED_AB1
8. Both dice exchange initial state via MEASUREMENT messages
9. If A was previously entangled with B2:
   - A sends ENTANGLE_STOP to B2
   - B2 transitions to INITSINGLE_AFTER_ENT
```

**Timeout:** If no throw occurs within 120 seconds (`MAXENTANGLEDWAITTIME`), entanglement reverts to INITSINGLE.

---

## Power Management

### Battery Monitoring

- ADC measures voltage through 50% voltage divider
- Thresholds defined in [defines.h](Arduino/QuantumDice/defines.h#L15-L16):
  - Maximum: 4.00V (nominal 4.2V under no load)
  - Minimum: 3.40V

```cpp
voltage = analogReadMilliVolts(adc_pin) / 1000.0 * 2.0
```

### Deep Sleep

Automatic power-off after inactivity:

1. IMU monitors movement
2. Timer starts when movement ceases
3. After timeout (configurable, stored in EEPROM):
   - Set `REGULATOR_PIN` HIGH
   - Power regulator shuts down

**Wake-up:** Pressing power switch re-enables regulator, device boots.

---

## Communication Protocols

### Watchdog Messages

Broadcast every state transition to all peers:

```cpp
void StateMachine::sendWatchDog() {
  message watchDog;
  watchDog.senderRole = roleSelf;
  watchDog.type = MESSAGE_TYPE_WATCH_DOG;
  watchDog.data.watchDog.state = stateSelf;
  Send(watchDog, roleBrother);
  Send(watchDog, roleSister);
}
```

Allows each dice to track partner states in real-time.

### Measurement Messages

Sent after die measurement in entangled mode:

```cpp
struct measurement {
  State state;
  DiceStates diceState;
  MeasuredAxises measureAxis;
  DiceNumbers diceNumber;
  UpSide upSide;
}
```

Partner uses this information to determine its own result based on quantum correlation rules.

---

## Debugging and Serial Output

### Debug Macros

Defined in [defines.h](Arduino/QuantumDice/defines.h#L7-L13):

```cpp
#define DEBUG 1
#if DEBUG == 1
  #define debug(x) Serial.print(x)
  #define debugln(x) Serial.println(x)
#else
  #define debug(x)
  #define debugln(x)
#endif
```

Set `DEBUG 0` to disable serial output in production.

### Serial Output

Baud rate: 115200

**Boot Sequence Output:**
- Firmware version
- Dice ID
- Board type (NANO/DEVKIT)
- Configuration details
- MAC address
- IMU calibration status
- Hardware pin assignments

**Runtime Output:**
- State transitions
- Entanglement events
- IMU measurements
- ESP-NOW send/receive status

---

## Timing Constants

| Constant | Value | Purpose |
|----------|-------|---------|
| `UPDATE_INTERVAL` | 50ms | Main loop execution rate |
| `FSM_UPDATE_INTERVAL` | 0ms | State machine update rate |
| `IDLETIME` | 3000ms | IDLE → CLASSIC_STATE timeout |
| `SHOWNEWSTATETIME` | 1000ms | Display new state duration |
| `MAXENTANGLEDWAITTIME` | 120000ms | Entanglement timeout |
| `STABTIME` | 800ms | Measurement stabilization delay |
| `BATTERYSTABTIME` | 1000ms | Battery voltage settling time |

---

## Pin Definitions

### Common Pins

```cpp
#define REGULATOR_PIN GPIO_NUM_18  // Power hold (D9)
#define BUTTON_PIN    GPIO_NUM_14  // User button
```

### Board-Specific Pins

#### NANO Configuration

| Function | GPIO |
|----------|------|
| TFT CS   | 21   |
| TFT RST  | 4    |
| TFT DC   | 2    |
| ADC      | 1    |
| Screen 0-5 CS | 5, 6, 7, 8, 9, 10 |

#### DEVKIT Configuration

| Function | GPIO |
|----------|------|
| TFT CS   | 10   |
| TFT RST  | 48   |
| TFT DC   | 47   |
| ADC      | 2    |
| Screen 0-5 CS | 4, 5, 6, 7, 15, 16 |

---

## Dependencies

### Arduino Libraries

- **Adafruit_Sensor** - Unified sensor interface
- **Adafruit_BNO055** - 9-axis IMU driver
- **Adafruit_GFX** - Graphics primitives
- **Adafruit_GC9A01A** - Display driver
- **SparkFun_ATECCX08a_Arduino_Library** - Hardware RNG
- **Button2** - Button event handling
- **EEPROM** - Non-volatile storage
- **WiFi** - ESP32 WiFi stack
- **esp_now** - ESP-NOW protocol
- **esp_wifi** - Low-level WiFi control

### External Resources

Fonts:
- FreeSans18pt7b
- FreeSansBold18pt7b
- FreeSansOblique12pt7b

---

## Error Handling

### Fatal Errors

The system halts execution on:

1. **Missing Configuration**
   ```cpp
   if (!loadConfigFromEEPROM()) {
     while(1) { delay(1000); }  // Infinite loop
   }
   ```

2. **Invalid Role Assignment**
   ```cpp
   assert(roleSelf != Roles::NONE);
   ```

### Recoverable Errors

- **IMU Read Failure:** Continues with stale data
- **ESP-NOW Send Failure:** Logged to serial, message dropped
- **Random Chip Unavailable:** Falls back to pseudo-random
- **Low Battery:** Enters LOWBATTERY state, displays warning

---

## Known Behaviors

### Measurement Edge Cases

1. **No Clear Axis:** If gravity vector doesn't align with X/Y/Z within bounds, triggers `measurementFail` and returns to THROWING state

2. **Same Axis Remeasurement:** In MEASURED state, measuring the same axis preserves the previous number (deterministic behavior)

3. **Different Axis in Entangled Mode:** If entangled dice measure different axes, each generates independent random numbers (no correlation)

### Entanglement Limitations

- Only one pair can be entangled at a time (A-B1 or A-B2, not both)
- Switching entanglement partner sends STOP message to previous partner
- RSSI-based proximity prevents long-range entanglement
- 2-minute timeout prevents indefinite waiting state

### Display Update Strategy

Screens refresh on state transitions, not on every loop iteration. This conserves power and reduces SPI bus traffic.

---

## Configuration Tool

The companion sketch `QuantumDiceInitTool` is used to program EEPROM configuration:

**Location:** `Arduino/QuantumDiceInitTool/QuantumDiceInitTool.ino`

**Purpose:**
- Write dice ID and MAC addresses
- Set display colors
- Configure hardware variant (NANO/DEVKIT, SMD/HDR)
- Set operational parameters (RSSI, tumble threshold, timeouts)
- Program IMU calibration data

**Usage:** Flash this tool once per device during initial setup, then flash the main QuantumDice sketch.

---

## Future Enhancement Opportunities

Based on code structure:

1. **Multi-Die Networks:** Template-based ESP-NOW supports arbitrary message types, enabling >3 dice systems

2. **Advanced Entanglement Modes:** Current `alwaysSeven` flag hints at multiple correlation algorithms

3. **Persistent State:** EEPROM has ~300 bytes unused, sufficient for usage statistics or throw history

4. **OTA Updates:** ESP32 platform supports over-the-air firmware updates

5. **Web Interface:** WiFi stack could serve configuration webpage

---

## Code Quality Notes

### Strengths

- Clear separation of concerns (IMU, Display, State Machine)
- Template-based communication for type safety
- Comprehensive debug output
- Flexible configuration system
- Graceful hardware variant handling

### Areas for Consideration

- State machine uses function pointers, increasing flash usage but improving maintainability
- Multiple delay() calls in state transitions (generally avoided in embedded systems)
- Global variables used extensively (typical for Arduino, but limits testability)
- Magic numbers in some calculations (e.g., 252 in rejection sampling)

---

## Version History

| Version | Changes |
|---------|---------|
| 1.0.0   | Release version with EEPROM configuration system |
| 075     | Config file in EEPROM |
| 071     | Bugfixing ESP-NOW with ESP32 core 3.3.0 |
| 070     | New ESP-NOW implementation with core 3.2.1 |
| 065     | Merged NANO and DEVKIT variants |
| 062     | Speed improvements and RNG refactor |
| 061     | Low voltage screen repair |
| 060     | IMU class implementation |

See [Version.h](Arduino/QuantumDice/Version.h) for complete history.

---

## License and Attribution

**Institution:** University of Twente - Quantum Lab
**Project:** Quantum Dice Demonstrator

**Purpose:** Educational demonstration of quantum entanglement principles using classical hardware and wireless communication.

**Quote Reference:** "God Does Not Play Dice" - Einstein's famous objection to quantum mechanics, displayed during boot sequence.

---

## Technical Support

For configuration issues, consult:
- Serial debug output at 115200 baud
- EEPROM memory map printout during boot
- Hardware pin configuration display
- IMU calibration status messages

**Critical First Steps:**
1. Verify EEPROM contains valid configuration
2. Check MAC addresses match between all three dice
3. Ensure IMU calibration data is loaded
4. Confirm battery voltage > 3.4V
5. Test ESP-NOW connectivity with watchdog messages

---

**Document Version:** 1.0
**Last Updated:** 2025-01-21
**Generated from Firmware Version:** 1.0.0

