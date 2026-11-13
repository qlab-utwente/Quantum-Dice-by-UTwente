// The dice's "littlefs" partition holds the settings file. We read the whole
// partition, open it in the browser with the LittleFS WebAssembly module from
// ESPConnect, change it there and write back only the parts that changed.

import { createLittleFS, createLittleFSFromImage, DISK_VERSION_2_0 } from '../vendor/littlefs/index.js';
import { CONFIG_SUFFIX } from './config.js';

const BLOCK_SIZE = 4096; // flash sector size used by the firmware's LittleFS

export async function openImage(image) {
  return await createLittleFSFromImage(image, { blockSize: BLOCK_SIZE, blockCount: image.length / BLOCK_SIZE });
}

export async function emptyFilesystem(size) {
  return await createLittleFS({
    blockSize: BLOCK_SIZE,
    blockCount: size / BLOCK_SIZE,
    formatOnInit: true,
    diskVersion: DISK_VERSION_2_0,
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
