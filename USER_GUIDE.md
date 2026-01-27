# Quantum Dice User Manual

## Table of Contents

1. [Introduction](#introduction)
2. [Power On](#power-on)
3. [Start Up Sequence](#start-up-sequence)
4. [Basic Quantum State](#basic-quantum-state)
5. [Using the Quantum Button](#using-the-quantum-button)
6. [Entanglement](#entanglement)
7. [Charging the Battery](#charging-the-battery)

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

For programming instructions, please refer to the YouTube video tutorial.

---

## Power On

Locate the power button on one of the yellow sides (marked with a power symbol).

**To power on/off:** Press the button briefly.

---

## Start Up Sequence

After powering on, the dice initializes and displays the following:

1. **Qlab logo** followed by additional branding
2. **Device information** on the power button display:
   - Dice ID
   - Firmware version
   - Battery voltage

**⚠️ Low battery warning:** If the battery voltage is low, charge the device before use.

The startup sequence completes in **Classic mode**, where each side displays a fixed number (like a traditional die). Press the Quantum button to enter Quantum mode.

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

**Long press:** Clears the previous measurement and returns the dice to the superposition state.

---

## Entanglement

**Requirements:** Both dice must be in Quantum mode and have **matching IDs**.

**How to entangle:**

1. Ensure both dice are in Quantum mode (superposition state visible)
2. Bring the two dice close together
3. You may need to adjust their position slightly
4. **Success indicator:** The superposition state turns **yellow**

**After entanglement:**

- Roll both dice separately
- The measurement outcomes will be **correlated** between the two dice
- After measurement, each die returns to its previous state

**⚠️ Important:** After entanglement, separate the dice by **more than 1 meter** to prevent them from re-entangling after rolling.

---

## Charging the Battery

**Battery specifications:**

- Capacity: 1700 mAh
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

**Enjoy your Quantum Dice!**
