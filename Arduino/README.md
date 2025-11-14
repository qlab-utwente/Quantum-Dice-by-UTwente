# 🎲 QuantumDice Software Flasher

Web-based firmware flasher for Quantum Dice ESP32 devices. Flash your ESP32 directly from your browser - no drivers or Python installation required!

If you want to upload the Arduino sketches yourself, please follow [these instructions](/Arduino/QuantumDice/README.md).

## 🌐 Access the Flasher

**Live Tool:** [https://qlab-utwente.github.io/Quantum-Dice-by-UTwente/]( https://qlab-utwente.github.io/Quantum-Dice-by-UTwente/)

## 📋 Requirements

- **Browser:** Chrome, Edge, or Opera (WebSerial API support required)
- **Hardware:** Quantum Dice with top and bottom displays removed. The USB-C connector is on the lower side of the dice
- **USB-C Cable:** USB-C cable with data lines (not charge-only)

## 🔧 Available Firmware

QuantumDice has **two separate programs** that need to be flashed depending on your needs:

### 1. 📝 Initialization Tool (`QuantumDiceInitTool.vX.X.X.bin`)

**Purpose:** First-time setup and sensor calibration

**Use this firmware to:**

- Configure your Quantum Dice ESP32 for the first time
- Calibrate quantum sensors
- Change parameters
- See [Initialisation manual](/Arduino/QuantumDiceInitTool/README.md) for detailled instructions


### 2. 🎲 Main Program (`QuantumDice.vX.X.X.bin`)

**Purpose:** Production firmware for the Quantum Dice

**Use this firmware for:**

- Regular QuantumDice operation

---

## 📖 Step-by-Step Flashing Instructions

### Step 1: Connect Your ESP32

1. Connect your Quantum Dice to your computer via USB
2. Wait for the device to be recognized
3. Open the [QuantumDice Flasher](https://qlab-utwente.github.io/Quantum-Dice-by-UTwente/) in Chrome/Edge/Opera

### Step 2: Connect to Serial Port

1. Click **"Connect Serial Port"**
2. Select your ESP32 from the popup (look for "USB Serial" or "CP210x")
3. Wait for "Connected successfully!" message
4. The macAddress is retrieved and presented. Use the `Copy` button to copy the macAddress to clipboard
4. The serial monitor will start automatically

### Step 3: Select Firmware

You have two options for selecting firmware:

#### Option A: Local File (Recommended for Downloaded Files)

1. Click the **"📁 Local File"** tab
2. Click **"Choose .bin file"**
3. Select your firmware file:
   - `QuantumDiceInitTool.vX.X.X.bin` (for initialization), OR
   - `QuantumDice.vX.X.X.bin` (for main program)

#### Option B: GitHub Release

1. Click the **"🚀 GitHub Release"** tab
2. The repository should be pre-filled: `qlab-utwente/Quantum-Dice-by-UTwente`
3. Click **"Fetch Releases"**
4. Select your desired release version
5. Select the .bin file you need
6. If download fails, click the provided link to download manually, then use Option A

### Step 5: Flash Firmware

1. Click **"Flash Firmware"**. The Serial monitor stops automatically.
2. Wait for the flashing process to complete
   - You'll see progress: "Writing at 0x10000... (0% - 100%)"
   - This typically takes 5-10 seconds
3. Look for **"Flash Complete"** message

**Note:** You may see an MD5 warning - this is usually harmless if the progress reached 100%.

### Step 6: Start Monitoring & Test

1. Click **"Start Monitor"** to see ESP32 output
2. Click **"Reset ESP32"** or press the physical reset button on your board
3. Check the serial monitor output to verify your firmware is running

---

## 🔄 Typical Usage Workflow

### First-Time Setup

```text
1. Flash → QuantumDiceInitTool.vX.X.X.bin
2. Follow on-screen calibration and configuration instructions
3. Save configuration
4. Flash → QuantumDice.vX.X.X.bin
5. Start using your QuantumDice!
```

See [Quantum Dice Init Tool](/Arduino/QuantumDiceInitTool/README.md) for detailled instructions.

### Firmware Update

```text
1. Download new QuantumDice.vX.X.X.bin
2. Flash the new version
3. Reset and test
```

### Recalibration or reconfiguration

```text
1. Flash → QuantumDiceInitTool.vX.X.X.bin
2. Perform recalibration
3. Flash → QuantumDice.vX.X.X.bin
4. Resume normal operation
```

See [Quantum Dice Init Tool](/Arduino/QuantumDiceInitTool/README.md) for detailled instructions.

---

## 🔍 Serial Monitor Usage

### Sending Commands

The serial monitor supports bidirectional communication:

1. Type your command in the input field at the bottom
2. Press **Enter** or click **"Send"**
3. The command is sent with both CR and LF (same as Arduino IDE)

**Tip:** You can press Enter without typing anything to send an empty line.

### Monitor Controls

- **Start Monitor** - Begin receiving data from ESP32
- **Stop Monitor** - Pause receiving (required before flashing)
- **Clear Output** - Clear the terminal window
- **Reset ESP32** - Send reset signal to restart the device

---

## ❓ Troubleshooting

### "WebSerial API is not supported"

- ✅ Use Chrome, Edge, or Opera browser
- ✅ Update your browser to the latest version
- ❌ Safari and Firefox do not support WebSerial

### "Cannot connect to serial port"

- ✅ Make sure no other program is using the serial port (close Arduino IDE, PuTTY, etc.)
- ✅ Try a different USB port
- ✅ Try a different USB cable (must support data transfer)
- ✅ Check Device Manager (Windows) or `ls /dev/tty.*` (Mac/Linux) to verify the device is detected

### "MD5 of file does not match data in flash"

- ✅ This warning is **common and usually harmless**
- ✅ If progress reached 100%, the flash likely succeeded
- ✅ Test your device - if it works, you can ignore this warning
- ❌ If the device doesn't work, try flashing again with a slower baud rate

### "Flash failed" or incomplete flash

- ✅ Make sure you stopped the serial monitor before flashing
- ✅ Try disconnecting and reconnecting the ESP32
- ✅ Check your USB cable quality
- ✅ Try a different USB port (preferably USB 2.0)

### Device not booting after flash

- ✅ Press the reset button on your ESP32
- ✅ Check that you flashed the correct .bin file
- ✅ Try erasing the flash and flashing again
- ✅ Verify power supply is adequate (USB ports can sometimes be insufficient)

---

## 📦 Getting Firmware Files

### From GitHub Releases

Visit: [https://github.com/qlab-utwente/Quantum-Dice-by-UTwente/releases/](https://github.com/qlab-utwente/Quantum-Dice-by-UTwente/releases)

Each release contains:

- `QuantumDiceInitTool.vX.X.X.bin` - Initialization & calibration tool
- `QuantumDice.vX.X.X.bin` - Main program

Download the version you need, then use the "Local File" tab in the flasher.

---

## 🛠️ Technical Details

- **Baudrate:** 115200
- **Flash Address:** 0x10000 (standard Arduino app partition)
- **Compression:** Enabled
- **Flash Mode:** Keep existing settings

---
