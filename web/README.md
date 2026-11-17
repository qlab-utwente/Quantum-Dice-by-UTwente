# Quantum Dice Tool (web page)

A web page, hosted on GitHub Pages, that installs firmware on a Quantum Dice,
reads and writes its settings file, and shows its serial output. It runs
entirely in the browser (Chrome or Edge on a computer) using Web Serial.

Plain HTML, CSS and JavaScript modules: there is no build step and no npm.

## Folder layout

| Path | What it is |
|---|---|
| `index.html`, `css/`, `js/`, `assets/` | The web page |
| `js/dice.js` | USB/serial connection, programming mode, flash read/write with MD5 check |
| `js/firmware.js` | Firmware list, downloads (SHA-256 checked) and the safety checks |
| `js/config.js` | Reading and checking `*_config.txt` the same way the firmware does |
| `js/storage.js` | Opening and changing the LittleFS settings storage |
| `vendor/esptool/` | [tasmota-webserial-esptool](https://github.com/Jason2866/WebSerial_ESPTool) 7.3.10 (MIT), the flashing library used by ESPConnect |
| `vendor/littlefs/` | LittleFS WebAssembly module from [ESPConnect](https://github.com/thelastoutpostworkshop/ESPConnect) (MIT); `index.js` is patched to accept preloaded WASM bytes (`wasmBinary`) |
| `tools/build_firmware_manifest.py` | Copies the release `.bin` files into `firmware/` and writes `firmware/manifest.json` |
| `firmware/` | Generated, not in git |
| `tests/` | Browser tests for everything that does not need a dice |

## How releases get onto the page

The browser is not allowed to download release files directly from github.com,
so the workflow `.github/workflows/pages.yml` runs `tools/build_firmware_manifest.py`
and publishes the files together with the page. It runs when a release is
published, changed or deleted, when `web/` changes on `main`, every 6 hours,
and by hand from the Actions tab. On a fork the release trigger never fires
(releases of the main repository are not copied), so there a new release
appears after the next scheduled run, or right away with **Run workflow**.

Release rules (enforced by the script):

- tag `GenX.Y` or `GenX.Y.Z`, from `Gen4.0` onwards (drafts are skipped);
- exactly one `*.bin` (application) and one `*.merged.bin` (complete image);
- both built for the ESP32-S3, the application inside the merged image must be
  identical to the separate `.bin`, and the partition table must contain
  `app0` at 0x10000 and a `littlefs` partition;
- `firmware/EXAMPLE_config.txt` at that tag is used to check settings files
  for that version, so keep it up to date with the keys the firmware reads.

A release that breaks a rule is left out and listed in the workflow log.

## Running it on your own computer (VS Code)

Web Serial only works on `https://` pages or on `http://localhost`, so the page
must be served by a local web server (opening `index.html` as a file does not work).

1. Open the repository folder in VS Code and open a terminal (**Terminal → New Terminal**).
2. Fetch the firmware files once (and again after a new release):
   ```
   python3 web/tools/build_firmware_manifest.py
   ```
3. Start a web server:
   ```
   python3 -m http.server 8000 --directory web
   ```
4. Open **http://localhost:8000** in Chrome or Edge. Stop the server with `Ctrl+C`.

Run the tests at **http://localhost:8000/tests/** (they should all say PASS).

After changing a file, reload the page with `Ctrl+Shift+R` (`Cmd+Shift+R` on a Mac)
so the browser does not use an old copy.

Instead of step 3 you can use the VS Code extension **Live Server** (by Ritwick Dey):
right-click `web/index.html` → **Open with Live Server**. It reloads the page
automatically when you save a file.

## Turning on GitHub Pages (once)

1. Merge this branch into `main`.
2. In the repository on GitHub go to **Settings → Pages** and set **Source** to **GitHub Actions**.
3. Go to **Actions → Deploy Quantum Dice Tool to GitHub Pages → Run workflow**.
4. The page appears at `https://qlab-utwente.github.io/Quantum-Dice-by-UTwente/`.
