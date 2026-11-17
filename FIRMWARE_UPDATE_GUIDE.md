# Quantum Dice Firmware Update Guide

This guide explains how to install new software (firmware) on a Quantum Dice and how to change its settings. You do not need any experience with programming or firmware: everything is done in your web browser with the **[Quantum Dice Tool](https://qlab-utwente.github.io/Quantum-Dice-by-UTwente/)**. Updating one dice takes about five minutes.

The procedure is the same for every generation of the Quantum Dice. Only the way you connect the USB cable is different:

- **Generation 4:** the USB-C connector is on the outside of the dice. You do not need to open it.
- **Generation 2:** the USB-C connector is on the processor board inside the dice. You first remove one display to reach it.

## Contents

1. [What you need](#1-what-you-need)
2. [Which installation do I need?](#2-which-installation-do-i-need)
3. [Connect the dice](#3-connect-the-dice)
4. [Update the firmware](#4-update-the-firmware)
5. [Complete install (new or faulty board)](#5-complete-install-new-or-faulty-board)
6. [Change the settings](#6-change-the-settings)
7. [Check that the dice works](#7-check-that-the-dice-works)
8. [Troubleshooting](#8-troubleshooting)

---

## 1. What you need

- A **computer** (Windows, macOS, Linux or ChromeOS) with internet access.
- **Google Chrome** or **Microsoft Edge**. Safari, Firefox, phones and tablets cannot connect to the dice.
- A **USB-C cable that can transfer data**. Many cables that come with chargers can only charge. If the dice does not appear on the computer, try another cable first.
- The **Quantum Dice**, with a charged battery.
- For a **Generation 2** dice: a small flat screwdriver or a hobby knife to remove a display.

You do not need to download anything. The Quantum Dice Tool always offers the published firmware versions.

> **Several dice?** Update them one at a time. Only connect the dice you are working on.

---

## 2. Which installation do I need?

The Quantum Dice Tool offers two choices. In almost all cases you need **Update firmware**.

| Situation | Choose | What happens to the settings |
|---|---|---|
| A new version of the firmware is available | **Update firmware** | Kept |
| You want to install the same version again | **Update firmware** | Kept |
| A new board that has never been used | **Complete install** | Not applicable (there are none yet) |
| The tool says *"Choose Complete install instead"* | **Complete install** | Kept if *Keep my settings* is ticked |
| The dice does not work properly after an update | **Complete install** | Kept if *Keep my settings* is ticked |

**Update firmware** replaces only the program. The dice keeps its name, colours and other settings.

**Complete install** first erases the whole memory of the dice and then installs everything again: the program, the memory layout and the storage for the settings. A board that has never been used, or that has the wrong memory layout, needs this once. After that, an update is enough.

You do not have to work out which one is needed yourself. When the dice is connected, the tool checks it. If an update is not possible (for example because the board is new or the memory layout is different), the tool tells you to choose **Complete install** and does not let you start the update.

---

## 3. Connect the dice

### Generation 4

1. **Switch the dice on** by briefly pressing the button on one of the yellow sides. The displays light up.
2. Plug the USB-C cable into the connector on the **yellow side** of the dice and into your computer.

Continue with [Open the Quantum Dice Tool](#open-the-quantum-dice-tool).

### Generation 2

On a Generation 2 dice the USB-C connector is mounted directly on the processor board (ESP32) inside the dice. Remove one display to reach it:

1. Make sure the dice is **switched off**.
2. Find the display behind which the processor board with the USB-C connector sits (the **bottom** side of the dice).
3. Carefully remove this display from the frame. Lift it evenly and do not pull on the flat cable that connects it.
4. If the flat cable is too short to put the display aside, disconnect it from the board. Remember how it was connected: the dice also works for the update without this display.
5. **Switch the dice on.** The remaining displays light up.
6. Plug the USB-C cable into the connector on the processor board and into your computer.

> **After the update:** switch the dice off, reconnect the flat cable if you removed it, and put the display back in its place.

### Open the Quantum Dice Tool

1. Open **[qlab-utwente.github.io/Quantum-Dice-by-UTwente](https://qlab-utwente.github.io/Quantum-Dice-by-UTwente/)** in Chrome or Edge.
2. In **step 1, Connect your dice**, click **Connect**.
3. A window appears with a list of devices. Choose the dice; it is usually called **USB JTAG/serial debug unit**. Click **Connect**.
4. The tool now checks the dice. After a few seconds you see its details:
   - **Processor:** ESP32-S3
   - **Installed firmware:** for example *Gen4.1*
   - **Dice name:** the name from its settings file

> **The list is empty?** The dice is switched off, has gone to sleep, or the cable can only charge. See [Troubleshooting](#8-troubleshooting).

> **The dice goes to sleep** after about five minutes without movement. When it is asleep the computer cannot reach it. Briefly press the button to wake it, then click **Check dice again**.

---

## 4. Update the firmware

Use this to install a new version on a dice that already works. Your settings are kept.

1. Connect the dice as described in [section 3](#3-connect-the-dice).
2. Go to **step 2, Install firmware**.
3. Under **Firmware version**, choose the newest version. It is at the top of the list and is selected automatically.
4. Under **What do you want to do?**, choose **Update firmware**.
5. Read the **Safety checks**. They show, for example:
   - ✓ *Quantum Dice processor (ESP32-S3) found.*
   - ✓ *Memory layout matches this firmware.*
   - ✓ *Upgrade from Gen4.0 to Gen4.1.*

   A red ✕ means the update is not possible. The message tells you what to do; usually: *Choose "Complete install" instead*. Continue with [section 5](#5-complete-install-new-or-faulty-board).
6. Click **Install firmware**. A window shows a summary. If there are warnings (orange), read them and tick the box to confirm.
7. Click **Update now**.
8. **Do not unplug the dice and do not close the page** until the window says **Done!** This takes one to two minutes. The tool downloads the firmware, writes it and then checks that it was written correctly.
9. The dice restarts by itself. Click **Close**.

To update the next dice: unplug this dice, plug in the next one, click **Connect** again and repeat these steps.

---

## 5. Complete install (new or faulty board)

Use this for a board that has never been used, when the tool tells you to choose Complete install, or when the dice still does not work properly after an update.

1. Connect the dice as described in [section 3](#3-connect-the-dice).
2. Go to **step 2, Install firmware** and choose the newest **Firmware version**.
3. Choose **Complete install**.
4. Leave **Keep my settings** ticked. The tool then saves the settings first and puts them back after the installation. On a new board there are no settings yet; the box makes no difference there.
5. Read the **Safety checks** and click **Install firmware**.
6. If you unticked *Keep my settings*, tick **I understand that all settings on the dice will be lost.**
7. Click **Erase and install**.
8. **Do not unplug the dice and do not close the page** until the window says **Done!** This takes one to two minutes.
9. When the tool saved your settings, the last window offers to save a copy of them on your computer. This is a good idea.

> **What if something goes wrong?** If the cable comes loose or the computer goes to sleep during the installation, the dice cannot be damaged. Connect it again and repeat the Complete install. If your settings were saved before the interruption, the tool offers you a copy to keep. You can put it back with [section 6](#6-change-the-settings).

After a Complete install on a **new board** the dice is called *DEFAULT*. To give it its own name and colours, follow [section 6](#6-change-the-settings).

---

## 6. Change the settings

Each dice keeps its settings in a small text file on the dice, called `<name>_config.txt`, for example `ALICE_config.txt`. It contains the name of the dice, the background colours, the entanglement animation and a few advanced values. Each value is explained in the file itself.

Dice that are used together do not need the same settings.

### Save the settings to your computer

1. Connect the dice ([section 3](#3-connect-the-dice)).
2. In **step 3, Settings file**, click **Read settings from dice**. This takes up to a minute.
3. The tool shows the file and checks it. Click **Save to this computer**.

Keep this copy: you can always put it back.

### Change the settings

1. Open the saved file in a plain text editor: **Notepad** (Windows) or **TextEdit** (macOS, choose *Format → Make Plain Text*). Do not use Word.
2. Change only the values after the `=` sign. Lines that start with `#` are explanations and are ignored by the dice.
3. To change the name of the dice, change `diceId=` and give the file the same name: `diceId=ALICE` belongs in `ALICE_config.txt`.
4. Save the file as plain text. The file name must end in `_config.txt` and may only contain letters, digits, `-`, `_` and `.` (no spaces).

No settings file yet? In **step 3, Settings file**, click **Download the example settings file** and change it as described above.

### Put the settings file on the dice

1. Connect the dice ([section 3](#3-connect-the-dice)).
2. In **step 3, Settings file**, click **Choose file…** and select your `_config.txt` file.
3. The tool checks the file before anything is written:
   - **Red** messages are errors. The file is not written; correct the file and try again.
   - **Orange** messages are warnings, for example a value that is unusual or a setting the firmware does not know. Read them carefully.
   - If the file name is not allowed, the tool offers a correct name, for example **Use the name "ALICE_config.txt"**.
4. Click **Write to dice**, then **Write settings** to confirm. Any other settings file on the dice is replaced.
5. Wait until the window says **Settings written** (up to a minute). The dice restarts with the new settings.

---

## 7. Check that the dice works

1. Keep the dice connected and look at **step 4, Serial monitor**. It shows the messages the dice sends while it starts.
2. Click **Restart dice** to see the start messages from the beginning.
3. Check that you see:
   - `FW: Gen 4.1` (or the version you installed)
   - `Dice ID: ...` with the name of your dice
4. Unplug the dice. For a Generation 2 dice: switch it off and put the display back.
5. Switch the dice on and check that all displays light up.

If you need help, click **Save to file** in the serial monitor and send this file with your question.

---

## 8. Troubleshooting

### "This browser cannot connect to the Quantum Dice"

Open the page in **Google Chrome** or **Microsoft Edge** on a computer. Safari, Firefox, phones and tablets are not supported.

### The dice does not appear in the list after clicking Connect

- Is the dice **switched on and awake**? Press the button briefly; the displays must be on.
- Try **another USB-C cable**. Many cables can only charge.
- Try another USB port on your computer, directly on the computer instead of through a hub.
- Close other programs that may be using the dice, such as the Arduino IDE, ESPConnect or another browser tab with the Quantum Dice Tool.
- **Windows:** wait a minute after plugging in the first time; Windows installs a driver automatically.

### "The dice did not answer"

The dice went to sleep or was switched off. Press the button to wake it and try again. If it keeps happening, unplug the dice, plug it in again and click **Connect**.

### The update is blocked with a red ✕

Read the message. *"The memory layout on this dice is different"* and *"There is no firmware on this dice yet"* both mean: use **Complete install** ([section 5](#5-complete-install-new-or-faulty-board)) with **Keep my settings** ticked.

### "You are going back to an OLDER version"

The firmware you selected is older than the one on the dice. Choose the newest version at the top of the list, unless you were asked to install an older one.

### "Installation did not finish" or "Settings were not written"

Nothing is broken. Check the cable, make sure the dice is awake, and try again. If an update keeps failing, use **Complete install**.

### The dice has the name DEFAULT after the update

The dice could not find a valid settings file, so it uses its standard settings. Put your settings file back with [section 6](#6-change-the-settings). The tool also tells you why a file was not accepted.

### Still stuck?

Create an issue on [GitHub](https://github.com/qlab-utwente/Quantum-Dice-by-UTwente/issues) and include:

- the generation of your dice (Generation 2 or 4);
- what you were doing and the exact message you saw;
- the serial monitor output (**Save to file**) and, if possible, the **Technical log** from step 4 of the tool.
