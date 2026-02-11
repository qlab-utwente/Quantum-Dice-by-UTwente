# Quantum Dice User Manual

## Table of Contents

1. [Introduction](#introduction)
2. [Power On](#power-on)
3. [Charging the Battery](#charging-the-battery)
4. [Start Up Sequence](#start-up-sequence)
5. [Basic Quantum State](#basic-quantum-state)
6. [Using the Quantum Button](#using-the-quantum-button)
7. [Entanglement](#entanglement)
8. [Teleportation](#teleportation)

---

## Introduction

Once your Quantum Dice assembly is complete, you're ready to start using it! This manual covers:

- Powering the device on and off
- Understanding the startup sequence
- Using the Quantum button
- Entangling two quantum dice
- Charging the battery

**Device orientation:** The dice has 3 axes, color-coded as follows:

- **Blue** - top and bottom sides
- **Red** - left and right sides
- **Yellow** - front and rear sides
Watch the [instructional video](https://youtu.be/DUJu3AJgXRc?si=qqh9P63NLDmi5nQ0) for usage guidance

---

## Power On

Locate the power button on one of the yellow sides (marked with a power symbol).

**To power on/off:** Press the button briefly.

---

## Charging the Battery

**Battery specifications:**

- Capacity: 1800 mAh
- Charging current: 500 mA

**Charging instructions:**

1. Locate the USB-C connector on one of the yellow sides
2. **⚠️ Important:** This connector is **only** for battery charging and is not connected to the ESP32 processor
3. **Use a USB-A to USB-C cable** with a USB-A charger (USB-C chargers will not work)
4. Charging takes approximately **2-3 hours**

**Monitoring charging status:**

The charge indicator LEDs are located inside the dice and are not visible during charging. To monitor the charging process, you can use:

- A USB charger with a built-in current indicator, or
- A USB current monitor dongle such as [this one](https://www.amazon.com/Multi-function-Voltmeter-Electrical-Appliances-Detection/dp/B0CRTBWTQF?th=1)

---

## Start Up Sequence

After powering on, the dice initializes and displays the following:

1. **Qlab logo** followed by additional branding
2. **Device information** on the power button display:
   - Dice ID
   - Firmware version
   - Battery voltage

**⚠️ Low battery warning:** If the battery voltage is low, charge the device before use. See [Charging the Battery](#charging-the-battery)

The startup sequence completes in **Classic mode**, where each side displays a fixed number (like a traditional die). Long press the Quantum button to enter Quantum mode.

---

## Basic Quantum State

In Quantum mode, all six sides display a **superposition state**.

**How it works:**

- Roll the dice
- When it comes to rest, a random number appears on the top and bottom sides
- This represents the "measurement" of the quantum state

---

## Using the Quantum Button

The Quantum button is located on the opposite side from the power button.

**Short press:** Toggle entanglement colour display
**Long press:** Return to Classic mode

### Entanglement Colour Display

When two dice are entangled, the Quantum Dice will display colours on the superposition state to indicate entanglement status. Both dice will have the same colour. When the colour display is toggled off, the dice will show the entanglement colour, and shortly after revert to the standard superposition state display.

---

## Entanglement

**Requirements:** Both dice must be in Quantum mode.

**How to entangle:**

1. Ensure both dice are in Quantum mode (superposition state visible)
2. Bring the two dice close together
3. You may need to adjust their position slightly
4. **Success indicator:** The superposition state turns **coloured**

**After entanglement:**

- Roll both dice separately
- The measurement outcomes will be **correlated** between the two dice
- After measurement, each die returns to its previous state

**⚠️ Important:** After entanglement, separate the dice by **more than 1 meter** to prevent them from re-entangling after rolling.

---

## Teleportation

**Requirements:** Three dice in Quantum mode, here called A, B, and M.

**How to teleport:**

1. Entangle Dice A and Dice B
2. Bringt Dice M into the disired quantum state (either superposition, mearsured state, or entangled state with another die)
3. Place Dice M close to Dice A to perform the teleportation operation
4. **Success indicator:** Dice B will now reflect the quantum state of Dice M
    - If Dice M was in superposition, Dice B will now be in superposition
    - If Dice M was measured, Dice B will show the same measurement outcome **when rolled in the same basis**
    - If Dice M was entangled with another die, Dice B will now be entangled with that die

### Entangelemnt Colour Display with Teleportation

When teleportation is successful, Dice B will display the entanglement colour of Dice A and M. If the colour display is toggled off, Dice B will show the entanglement colour briefly before reverting to the standard superposition state display.

---

**Enjoy your Quantum Dice!**
