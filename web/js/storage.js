// The dice's "littlefs" partition holds the settings file. We read the whole
// partition, open it in the browser with the LittleFS WebAssembly module from
// ESPConnect, change it there and write back only the parts that changed.

import { createLittleFS, createLittleFSFromImage, DISK_VERSION_2_0 } from '../vendor/littlefs/index.js';
import { CONFIG_SUFFIX } from './config.js';

const BLOCK_SIZE = 4096; // flash sector size used by the firmware's LittleFS
const WASM_URL = new URL('../vendor/littlefs/littlefs.wasm', import.meta.url).href;

let wasmPromise = null;

/**
 * Load the LittleFS WebAssembly code once (with retries) and keep it in memory.
 * Called when the page opens and again before the dice is switched to
 * programming mode, so a network problem never interrupts work on the dice.
 */
export function prepareStorage() {
  if (!wasmPromise) {
    wasmPromise = (async () => {
      let lastErr;
      for (let attempt = 0; attempt < 3; attempt++) {
        try {
          const res = await fetch(WASM_URL, { cache: attempt ? 'reload' : 'default' });
          if (!res.ok) throw new Error(`HTTP ${res.status}`);
          const bytes = new Uint8Array(await res.arrayBuffer());
          if (!WebAssembly.validate(bytes)) throw new Error('damaged download');
          return bytes;
        } catch (err) {
          lastErr = err;
          await new Promise((r) => setTimeout(r, 500 * (attempt + 1)));
        }
      }
      throw new Error(
        'This page could not load one of its parts (littlefs.wasm). Reload the page ' +
        `(Ctrl+Shift+R, or Cmd+Shift+R on a Mac) and try again. (${lastErr?.message || lastErr})`,
      );
    })();
    wasmPromise.catch(() => { wasmPromise = null; }); // allow a new attempt later
  }
  return wasmPromise;
}

export async function openImage(image) {
  const wasmBinary = await prepareStorage();
  return await createLittleFSFromImage(image, { blockSize: BLOCK_SIZE, blockCount: image.length / BLOCK_SIZE, wasmBinary });
}

export async function emptyFilesystem(size) {
  const wasmBinary = await prepareStorage();
  return await createLittleFS({
    blockSize: BLOCK_SIZE,
    blockCount: size / BLOCK_SIZE,
    formatOnInit: true,
    diskVersion: DISK_VERSION_2_0,
    wasmBinary,
  });
}

/** Files in the root folder, in the order the dice sees them. */
export function rootFiles(fs) {
  return fs
    .list('/')
    .filter((e) => e.type === 'file' && e.path.split('/').length === 2)
    .map((e) => ({ name: e.name ?? e.path.slice(1), type: 'file', size: e.size }));
}

/** Replace all settings files by a single new one. Returns the new image. */
export function replaceConfig(fs, name, bytes) {
  for (const f of rootFiles(fs)) {
    if (f.name.endsWith(CONFIG_SUFFIX)) fs.deleteFile(`/${f.name}`);
  }
  fs.writeFile(`/${name}`, bytes);
  return fs.toImage();
}

export function isBlank(image) {
  for (let i = 0; i < image.length; i++) if (image[i] !== 0xff) return false;
  return true;
}
