# Firmware State Machine

This document describes the state machine in [`main/StateMachine.cpp`](main/StateMachine.cpp) and [`main/StateMachine.hpp`](main/StateMachine.hpp).

The dice state is the product of three independent parts (`struct State`):

| Part | Values |
|------|--------|
| `Mode` | `CLASSIC`, `QUANTUM` |
| `ThrowState` | `IDLE`, `THROWING`, `OBSERVED` |
| `EntanglementState` | `PURE`, `ENTANGLE_REQUESTED`, `ENTANGLED`, `POST_ENTANGLEMENT`, `TELEPORTED` |

Most transitions go through `changeState(Trigger)`. It looks up the **first** matching entry in `stateTransitions`, where an empty (`std::nullopt`) current field matches any value and an empty next field leaves that value as it is. If the state changes, it calls the `onEntry` function of the new state. `update()` calls the `whileInState` function on every loop. A few transitions write `currentState.entanglementState` directly instead of using a trigger. These are drawn with dashed notes or marked *(direct)* below.

Because the full product has 16 valid states, the machine is shown as three diagrams: one for the mode and power, one for the throw state, and one for the entanglement state.

---

## 1. Mode and power

```mermaid
stateDiagram-v2
    direction LR

    [*] --> CLASSIC : boot / esp_restart()

    state "CLASSIC · IDLE · PURE" as CLASSIC
    state "QUANTUM" as QUANTUM
    state "Shutting down<br/>(tryingToSleep)" as SLEEP

    CLASSIC --> QUANTUM : BUTTON_PRESSED<br/>(short press in whileClassicIdle)
    QUANTUM --> CLASSIC : BUTTON_PRESSED<br/>(in table, but never fired)

    CLASSIC --> SLEEP : long press or<br/>inactivity > deepSleepTimeout
    QUANTUM --> SLEEP : long press or<br/>inactivity > deepSleepTimeout

    SLEEP --> [*] : esp_deep_sleep_try_to_start()
    SLEEP --> CLASSIC : USB or button press<br/>→ esp_restart()
```

- The dice always boots into `CLASSIC / IDLE / PURE`. This state is set in the constructor, and `begin()` calls its `onEntry`.
- Watchdog broadcasts (every 500 ms) are only sent in `QUANTUM` mode, so a `CLASSIC` dice is invisible to other dice.
- Shutdown is handled in `update()` and `checkTimeForDeepSleep()`. It is outside the `State` struct, so it is not a real `State`.

---

## 2. Throw state (within `QUANTUM`)

```mermaid
stateDiagram-v2
    direction LR

    [*] --> IDLE : enter QUANTUM

    IDLE --> THROWING : START_ROLLING<br/>(IMU.tumbled)
    THROWING --> OBSERVED : STOP_ROLLING<br/>(IMU.stable && IMU.onTable)
    OBSERVED --> THROWING : START_ROLLING<br/>(IMU.tumbled)
    OBSERVED --> THROWING : MEASURE_FAIL<br/>(still moving / tilted in enterObserved)

    THROWING --> IDLE : ENTANGLE_REQUEST /<br/>ENTANGLE_CONFIRM
    OBSERVED --> IDLE : ENTANGLE_REQUEST /<br/>ENTANGLE_CONFIRM

    THROWING --> IDLE : TELEPORT_INITIATED /<br/>TELEPORT_CONFIRMED
    OBSERVED --> IDLE : TELEPORT_INITIATED /<br/>TELEPORT_CONFIRMED

    note right of OBSERVED
        enterObserved() reads the up side, picks the number
        and then resets the entanglement state to PURE (direct)
        for ENTANGLED, POST_ENTANGLEMENT and TELEPORTED.
    end note
```

| State | `onEntry` | `whileInState` |
|-------|-----------|----------------|
| `IDLE` | `enterQuantumIdle` | `whileQuantumIdle`: checks for a tumble and the `ENTANGLE_REQUESTED` timeout |
| `THROWING` | `enterThrowing` | `whileThrowing`: checks whether the dice is stable and on the table |
| `OBSERVED` | `enterObserved`: measures and decides the number | `whileObserved`: checks for a tumble |

The same functions are used for every entanglement state.

---

## 3. Entanglement state (within `QUANTUM`)

```mermaid
stateDiagram-v2
    [*] --> PURE

    PURE --> ENTANGLE_REQUESTED : CLOSE_BY<br/>(we sent ENTANGLE_REQUEST)
    POST_ENTANGLEMENT --> ENTANGLE_REQUESTED : CLOSE_BY [IDLE]

    PURE --> ENTANGLED : ENTANGLE_REQUEST<br/>(received, we send confirm)
    POST_ENTANGLEMENT --> ENTANGLED : ENTANGLE_REQUEST [IDLE]
    ENTANGLE_REQUESTED --> ENTANGLED : ENTANGLE_CONFIRM

    ENTANGLE_REQUESTED --> PURE : ENTANGLE_STOP<br/>(ENTANGLE_DENIED received)
    ENTANGLE_REQUESTED --> PURE : TIMED [IDLE]<br/>(> MAXENTANGLEDWAITTIME, 120 s)

    ENTANGLED --> POST_ENTANGLEMENT : MEASUREMENT_RECEIVED<br/>(partner was measured first)
    ENTANGLED --> PURE : measured first in enterObserved<br/>(direct, sends MEASUREMENT)
    POST_ENTANGLEMENT --> PURE : measured in enterObserved<br/>(direct, opposite value if same axis)

    ENTANGLED --> PURE : TELEPORT_CONFIRMED<br/>(we are Alice)
    ENTANGLED --> PURE : TELEPORT_RECEIVED<br/>(we are Bob, M was PURE)
    ENTANGLED --> ENTANGLED : TELEPORT_PAYLOAD, M was ENTANGLED<br/>(direct, new partner N)
    ENTANGLED --> TELEPORTED : TELEPORT_PAYLOAD, M was OBSERVED<br/>(direct)
    TELEPORTED --> PURE : measured in enterObserved<br/>(direct, M's value if same axis)

    ENTANGLED --> PURE : ENTANGLE_STOP / TIMED<br/>(in table, never fired)

    note left of PURE
        TELEPORT_INITIATED (we are M, our ENTANGLE_REQUEST
        was answered with a teleport) sends any state
        to PURE and the throw state to IDLE.
    end note
```

### Teleportation roles

| Role | Situation | What happens |
|------|-----------|--------------|
| **M** (messenger) | Comes close to dice A, which is `ENTANGLED` with B | M sends its state to B and fires `TELEPORT_INITIATED` → `PURE` |
| **A** (Alice) | Was entangled with B, receives `TELEPORT_CONFIRM` | `TELEPORT_CONFIRMED` → `PURE` |
| **B** (Bob) | Was entangled with A, receives `TELEPORT_PAYLOAD` | Takes over M's state: `PURE` (trigger), `ENTANGLED` with N (direct) or `TELEPORTED` (direct) |

When a dice that is already `ENTANGLED` comes close to another dice, `checkCloseBy()` sends a teleport request instead of firing `CLOSE_BY`.

---

## 4. Triggers and where they are fired

| Trigger | Fired from | Condition |
|---------|-----------|-----------|
| `BUTTON_PRESSED` | `whileClassicIdle` | Short button press |
| `START_ROLLING` | `whileQuantumIdle`, `whileObserved` | `IMU.tumbled()` |
| `STOP_ROLLING` | `whileThrowing` | `IMU.stable() && IMU.onTable()` |
| `MEASURE_FAIL` | `enterObserved` | Still moving, or orientation `TILTED`/`UNKNOWN` |
| `CLOSE_BY` | `checkCloseBy` | Strongest nearby dice above the RSSI limit, not entangled |
| `ENTANGLE_REQUEST` | `updateEspNow` | `ENTANGLE_REQUEST` message received (not a teleport) |
| `ENTANGLE_CONFIRM` | `updateEspNow` | `ENTANGLE_CONFIRM` message received |
| `ENTANGLE_STOP` | `updateEspNow` | `ENTANGLE_DENIED` received while `ENTANGLE_REQUESTED` |
| `MEASUREMENT_RECEIVED` | `updateEspNow` | `MEASUREMENT` message from the current partner |
| `TELEPORT_INITIATED` | `updateEspNow` | Our entangle request was answered by an entangled dice |
| `TELEPORT_CONFIRMED` | `updateEspNow` | `TELEPORT_CONFIRM` message received |
| `TELEPORT_RECEIVED` | `updateEspNow` | `TELEPORT_PAYLOAD` received, and M was `PURE` |
| `TIMED` | `whileQuantumIdle` | `ENTANGLE_REQUESTED` for longer than `MAXENTANGLEDWAITTIME` |
| `MEASURE` | — | Declared, but not used |

---

## 5. Things found while mapping the code

These are observations from reading the code, not confirmed bugs:

1. **No way back to `CLASSIC`.** The `QUANTUM → CLASSIC` transitions exist in the table, but `BUTTON_PRESSED` is only polled in `whileClassicIdle`. The only way back to `CLASSIC` is a restart after sleep.
2. **`IDLE + ENTANGLE_REQUESTED` ignores a throw.** There is no `START_ROLLING` transition for this state, so a tumble logs `"No valid state transition found"` and the dice stays where it is until it is confirmed, denied or the 120 s timeout runs out.
3. **`OBSERVED + ENTANGLE_REQUESTED` can get stuck.** You get here through `CLOSE_BY` while `OBSERVED`, or through `STOP_ROLLING` from `THROWING + ENTANGLE_REQUESTED`. There is no `START_ROLLING` transition, and `TIMED` is only checked in `whileQuantumIdle`. The dice only leaves this state through `ENTANGLE_CONFIRM` or `ENTANGLE_STOP`.
4. **Unused transitions.** `ENTANGLE_STOP` and `TIMED` from `ENTANGLED`, and the `MEASURE` trigger, are never fired.
5. **Comment does not match the table.** The comment at the `TELEPORT_RECEIVED` call says *"ENTANGLED/POST_ENTANGLEMENT -> PURE"*, but the table only defines this transition from `ENTANGLED`.
