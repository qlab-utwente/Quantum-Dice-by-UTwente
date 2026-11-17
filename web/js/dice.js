// Talking to the Quantum Dice over Web Serial.
//
// The serial port is used in two ways, never at the same time:
//  - "monitor": the normal firmware runs and we show its text output;
//  - "bootloader": the dice is reset into its ROM bootloader and the
//    tasmota-webserial-esptool library (also used by ESPConnect) reads and
//    writes the flash memory. Afterwards the dice is reset to run normally.

import { ESPLoader, CHIP_FAMILY_ESP32S3 } from '../vendor/esptool/index.js';
import { md5Hex } from './hash.js';

export const ESPRESSIF_USB_VENDOR = 0x303a;
const MONITOR_BAUD = 115200;
const ROM_BAUD = 115200;
const ESP_SPI_FLASH_MD5 = 0x13;
const SECTOR = 0x1000;

export const PARTITION_TABLE_OFFSET = 0x8000;
export const EXPECTED_FLASH = '16MB';

const sleep = (ms) => new Promise((r) => setTimeout(r, ms));

export function isWebSerialSupported() {
  return typeof navigator !== 'undefined' && 'serial' in navigator;
}

export class Dice extends EventTarget {
  constructor() {
    super();
    this.port = null;
    this.mode = 'closed'; // 'closed' | 'monitor' | 'bootloader'
    this._reader = null;
    this._monitorDone = null;
    this._decoder = null;
    this._wantMonitor = false;

    if (isWebSerialSupported()) {
      navigator.serial.addEventListener('disconnect', (e) => {
        if (e.target === this.port) this._emit('portlost');
      });
      navigator.serial.addEventListener('connect', (e) => {
        if (this._wantMonitor && this.mode === 'closed' && this._samePort(e.target)) {
          this.port = e.target;
          this.startMonitor().catch(() => {});
        }
      });
    }
  }

  _emit(type, detail) {
    this.dispatchEvent(new CustomEvent(type, { detail }));
  }

  _log(msg) {
    this._emit('log', msg);
  }

  _samePort(port) {
    if (!this.port || !port) return false;
    const a = this.port.getInfo();
    const b = port.getInfo();
    return a.usbVendorId === b.usbVendorId && a.usbProductId === b.usbProductId;
  }

  get connected() {
    return this.port !== null;
  }

  /** Ask the user to pick the dice in the browser's port chooser. */
  async choosePort() {
    const port = await navigator.serial.requestPort({ filters: [{ usbVendorId: ESPRESSIF_USB_VENDOR }] });
    if (this.port && this.port !== port) await this.forget();
    this.port = port;
    this._wantMonitor = true;
    return port;
  }

  async forget() {
    this._wantMonitor = false;
    await this.stopMonitor();
    this.port = null;
    this.mode = 'closed';
    this._emit('mode', this.mode);
  }

  // --------------------------------------------------------------------------
  // Serial monitor
  // --------------------------------------------------------------------------

  async startMonitor() {
    if (!this.port || this.mode === 'monitor') return;
    this._wantMonitor = true;
    await this._openWithRetry(MONITOR_BAUD);
    try {
      // Release the reset / boot lines so the firmware runs normally.
      await this.port.setSignals({ dataTerminalReady: false, requestToSend: false });
    } catch { /* not all adapters support this */ }
    this.mode = 'monitor';
    this._emit('mode', this.mode);
    this._decoder = new TextDecoder();
    this._monitorDone = this._monitorLoop();
  }

  async _monitorLoop() {
    try {
      while (this.port?.readable) {
        this._reader = this.port.readable.getReader();
        try {
          for (;;) {
            const { value, done } = await this._reader.read();
            if (done) return;
            if (value?.length) this._emit('serial', this._decoder.decode(value, { stream: true }));
          }
        } finally {
          try { this._reader.releaseLock(); } catch { /* ignore */ }
          this._reader = null;
        }
      }
    } catch (err) {
      this._log(`Serial monitor stopped: ${err.message || err}`);
    } finally {
      if (this.mode === 'monitor') {
        try { await this.port?.close(); } catch { /* ignore */ }
        this.mode = 'closed';
        this._emit('mode', this.mode);
      }
    }
  }

  async stopMonitor() {
    if (this.mode !== 'monitor') return;
    this.mode = 'closing';
    try { await this._reader?.cancel(); } catch { /* ignore */ }
    try { await this._monitorDone; } catch { /* ignore */ }
    try { await this.port?.close(); } catch { /* ignore */ }
    this.mode = 'closed';
    this._emit('mode', this.mode);
  }

  /** Restart the dice (like pressing reset) while the monitor is running. */
  async restartFirmware() {
    if (this.mode !== 'monitor') return;
    await this.port.setSignals({ dataTerminalReady: false, requestToSend: true });
    await sleep(100);
    await this.port.setSignals({ dataTerminalReady: false, requestToSend: false });
  }

  async _openWithRetry(baudRate, attempts = 20) {
    let lastErr;
    for (let i = 0; i < attempts; i++) {
      try {
        if (!this.port.readable) await this.port.open({ baudRate, bufferSize: 64 * 1024 });
        return;
      } catch (err) {
        lastErr = err;
        // After a reset the USB device can disappear for a moment; find it again.
        await sleep(500);
        try {
          const ports = await navigator.serial.getPorts();
          const again = ports.find((p) => this._samePort(p));
          if (again) this.port = again;
        } catch { /* ignore */ }
      }
    }
    throw new Error(`Could not open the USB connection to the dice (${lastErr?.message || lastErr}).`);
  }

  // --------------------------------------------------------------------------
  // Bootloader session
  // --------------------------------------------------------------------------

  /**
   * Reset the dice into its bootloader, run `fn(session)`, then reset the
   * dice to run its firmware again and resume the serial monitor.
   */
  async withBootloader(fn) {
    if (!this.port) throw new Error('No dice connected.');
    const resume = this._wantMonitor;
    await this.stopMonitor();
    this.mode = 'bootloader';
    this._emit('mode', this.mode);

    const logger = {
      log: (msg, ...a) => this._log([msg, ...a].join(' ')),
      error: (msg, ...a) => this._log(`ERROR: ${[msg, ...a].join(' ')}`),
      debug: () => {},
    };

    let loader = null;
    try {
      await this._openWithRetry(ROM_BAUD);
      loader = new ESPLoader(this.port, logger);
      try {
        await loader.initialize();
      } catch (err) {
        throw new Error(
          'The dice did not answer. Make sure it is switched on and awake, then try again. ' +
          `(${err.message || err})`,
        );
      }
      const session = new BootloaderSession(loader, this);
      await session._init();
      return await fn(session);
    } finally {
      if (loader) {
        try {
          // DTR must be released, otherwise the dice restarts into the bootloader again.
          await loader.setDTR(false);
          await loader.hardReset(false);
        } catch { /* ignore */ }
        // disconnect() can hang if the device is already gone (same guard as ESPConnect).
        try { await Promise.race([loader.disconnect(), sleep(1500)]); } catch { /* ignore */ }
      }
      try { await this.port.close(); } catch { /* ignore */ }
      this.mode = 'closed';
      this._emit('mode', this.mode);
      if (resume) {
        await sleep(300);
        try { await this.startMonitor(); } catch (err) { this._log(err.message); }
      }
    }
  }
}

/** Operations available while the dice is in its bootloader. */
export class BootloaderSession {
  constructor(romLoader, dice) {
    this.rom = romLoader;
    this.dice = dice;
    this.esp = null;
  }

  async _init() {
    this.chipFamily = this.rom.getChipFamily();
    this.chipName = this.rom.chipName || 'unknown chip';
    this.isEsp32s3 = this.chipFamily === CHIP_FAMILY_ESP32S3;
    if (!this.isEsp32s3) {
      // Do not load anything onto a device that is not a Quantum Dice.
      return;
    }
    this.esp = await this.rom.runStub();
    this.flashSize = this.esp.flashSize ?? this.rom.flashSize ?? null;
    this.mac = (this.rom.macAddr?.() || []).map((b) => b.toString(16).padStart(2, '0')).join(':');
  }

  _require() {
    if (!this.esp) throw new Error('This device is not a Quantum Dice (no ESP32-S3 found).');
  }

  async read(addr, size, onProgress) {
    this._require();
    return await this.esp.readFlash(addr, size, (_pkt, progress, total) => onProgress?.(progress, total));
  }

  /** MD5 of a flash region, computed by the dice itself. */
  async flashMd5(addr, size) {
    this._require();
    const payload = [];
    for (const v of [addr, size, 0, 0]) payload.push(v & 0xff, (v >>> 8) & 0xff, (v >>> 16) & 0xff, (v >>> 24) & 0xff);
    const timeout = 3000 + Math.ceil(size / 0x100000) * 8000;
    const [, res] = await this.esp.checkCommand(ESP_SPI_FLASH_MD5, payload, 0, timeout);
    const bytes = new Uint8Array(res ?? []);
    if (bytes.length >= 32) return new TextDecoder().decode(bytes.slice(0, 32)).toLowerCase();
    return Array.from(bytes.slice(0, 16), (b) => b.toString(16).padStart(2, '0')).join('');
  }

  /** Write data at addr (compressed) and check the result with MD5. */
  async writeAndVerify(addr, data, onProgress) {
    this._require();
    if (addr % SECTOR !== 0) throw new Error('Write address must be sector aligned.');
    let bytes = data instanceof Uint8Array ? data : new Uint8Array(data);
    if (bytes.length % 4) {
      const padded = new Uint8Array(bytes.length + (4 - (bytes.length % 4))).fill(0xff);
      padded.set(bytes);
      bytes = padded;
    }
    const buf = bytes.buffer.slice(bytes.byteOffset, bytes.byteOffset + bytes.byteLength);
    await this.esp.flashData(buf, (written, total) => onProgress?.(written, total), addr, true);
    const expected = md5Hex(bytes);
    const actual = await this.flashMd5(addr, bytes.length);
    if (expected !== actual) {
      throw new Error(`Verification failed at 0x${addr.toString(16)}: the data on the dice is not what was written.`);
    }
  }

  async eraseAll() {
    this._require();
    await this.esp.eraseFlash();
  }

  async readPartitionTable() {
    const data = await this.read(PARTITION_TABLE_OFFSET, 0xc00);
    return parsePartitionTable(data);
  }
}

export function parsePartitionTable(data) {
  const parts = [];
  for (let i = 0; i + 32 <= data.length; i += 32) {
    if (data[i] === 0xff && data[i + 1] === 0xff) break;
    if (data[i] === 0xeb && data[i + 1] === 0xeb) break; // MD5 entry
    if (data[i] !== 0xaa || data[i + 1] !== 0x50) return null; // not a valid table
    const dv = new DataView(data.buffer, data.byteOffset + i, 32);
    let label = '';
    for (let j = 12; j < 28 && data[i + j] !== 0; j++) label += String.fromCharCode(data[i + j]);
    parts.push({
      label,
      type: data[i + 2],
      subtype: data[i + 3],
      offset: dv.getUint32(4, true),
      size: dv.getUint32(8, true),
    });
  }
  return parts;
}

export function samePartitions(a, b) {
  if (!a || !b || a.length !== b.length) return false;
  return a.every((p, i) => ['label', 'type', 'subtype', 'offset', 'size'].every((k) => p[k] === b[i][k]));
}

/** Smallest sector-aligned range in which two images differ, or null. */
export function changedRange(oldImg, newImg) {
  let first = -1;
  let last = -1;
  for (let off = 0; off < newImg.length; off += SECTOR) {
    const end = Math.min(off + SECTOR, newImg.length);
    let differs = false;
    for (let i = off; i < end; i++) {
      if (oldImg[i] !== newImg[i]) { differs = true; break; }
    }
    if (differs) {
      if (first < 0) first = off;
      last = end;
    }
  }
  return first < 0 ? null : { start: first, end: last };
}
