// Firmware releases: loading the list, downloading files and the safety
// checks that decide which install options are allowed for a connected dice.

import { sha256Hex } from './hash.js';
import { samePartitions, EXPECTED_FLASH } from './dice.js';

const CHIP_ID_ESP32S3 = 9;

export async function loadManifest() {
  const res = await fetch('firmware/manifest.json', { cache: 'no-cache' });
  if (!res.ok) {
    throw new Error('The list of firmware versions could not be loaded. Try again later.');
  }
  const manifest = await res.json();
  if (!manifest.releases?.length) throw new Error('No firmware versions are available yet.');
  return manifest;
}

export function versionLabel(release) {
  return release.prerelease ? `${release.tag} (test version)` : release.tag;
}

export function compareVersions(a, b) {
  for (let i = 0; i < Math.max(a.length, b.length); i++) {
    const d = (a[i] ?? 0) - (b[i] ?? 0);
    if (d) return d;
  }
  return 0;
}

const cache = new Map();

/** Download a firmware file and check that it is exactly the expected file. */
export async function downloadVerified(fileInfo, onProgress) {
  if (cache.has(fileInfo.file)) return cache.get(fileInfo.file);
  const res = await fetch(fileInfo.file, { cache: 'no-cache' });
  if (!res.ok) throw new Error(`Download failed (${res.status}). Check your internet connection.`);
  const total = Number(res.headers.get('content-length')) || fileInfo.size;
  const reader = res.body.getReader();
  const chunks = [];
  let received = 0;
  for (;;) {
    const { done, value } = await reader.read();
    if (done) break;
    chunks.push(value);
    received += value.length;
    onProgress?.(received, total);
  }
  const data = new Uint8Array(received);
  let off = 0;
  for (const c of chunks) { data.set(c, off); off += c.length; }

  if (data.length !== fileInfo.size || (await sha256Hex(data)) !== fileInfo.sha256) {
    throw new Error('The downloaded firmware file is damaged or incomplete. Nothing was written to the dice. Please try again.');
  }
  cache.set(fileInfo.file, data);
  return data;
}

/** Sanity check of an application image (first bytes of a .bin file). */
export function checkAppImage(data) {
  if (data.length < 64 || data[0] !== 0xe9) return 'This is not an ESP32 firmware file.';
  const chipId = data[12] | (data[13] << 8);
  if (chipId !== CHIP_ID_ESP32S3) return 'This firmware is not made for the ESP32-S3 chip in the Quantum Dice.';
  return null;
}

/**
 * Identify the firmware currently on the dice by comparing the MD5 of the
 * app0 partition with every known release.
 */
export async function identifyInstalled(session, manifest) {
  const header = await session.read(manifest.app0Offset, 64);
  if (header[0] !== 0xe9) return { state: 'empty' };
  for (const rel of manifest.releases) {
    const md5 = await session.flashMd5(manifest.app0Offset, rel.app.size);
    if (md5 === rel.app.md5) return { state: 'known', release: rel };
  }
  return { state: 'unknown' };
}

/**
 * Inspect the connected dice (in bootloader) and return everything the page
 * needs to decide which actions are safe.
 */
export async function inspectDice(session, manifest) {
  const info = {
    chipName: session.chipName,
    isEsp32s3: session.isEsp32s3,
    flashSize: session.flashSize,
    mac: session.mac,
    partitions: null,
    installed: { state: 'unknown' },
  };
  if (!session.isEsp32s3) return info;
  info.partitions = await session.readPartitionTable();
  if (info.partitions?.length) {
    info.installed = await identifyInstalled(session, manifest);
  } else {
    info.installed = { state: 'empty' };
  }
  return info;
}

/**
 * Checks shown to the user before installing `release` with `mode`
 * ('update' = app only, keep settings; 'full' = erase everything).
 * Each check: { level: 'ok'|'info'|'warning'|'error', msg }
 * Any 'error' blocks the installation.
 */
export function installChecks(info, release, mode) {
  const checks = [];
  const add = (level, msg) => checks.push({ level, msg });

  if (!info) {
    add('error', 'Connect your dice first.');
    return checks;
  }
  if (!info.isEsp32s3) {
    add('error', `The connected device is a ${info.chipName}, not a Quantum Dice (ESP32-S3). Nothing will be written.`);
    return checks;
  }
  add('ok', 'Quantum Dice processor (ESP32-S3) found.');

  if (info.flashSize !== EXPECTED_FLASH) {
    add('error', `This board has ${info.flashSize ?? 'an unknown amount of'} flash memory; the Quantum Dice needs 16MB. Nothing will be written.`);
    return checks;
  }
  add('ok', 'Memory size is correct (16MB).');

  const sameLayout = samePartitions(info.partitions, release.partitions);
  if (mode === 'update') {
    if (!sameLayout) {
      add('error', 'The memory layout on this dice is different from what this firmware needs, so an update is not possible. Choose "Complete install" instead.');
    } else {
      add('ok', 'Memory layout matches this firmware.');
    }
    if (info.installed.state === 'empty') {
      add('error', 'There is no firmware on this dice yet. Choose "Complete install" instead.');
    }
  } else if (!sameLayout) {
    add('info', 'The memory layout will be changed to the one this firmware needs.');
  }

  const inst = info.installed;
  if (inst.state === 'known') {
    const cmp = compareVersions(release.version, inst.release.version);
    if (cmp === 0) add('info', `Version ${inst.release.tag} is already installed. Installing it again is allowed.`);
    else if (cmp < 0) add('warning', `You are going back to an OLDER version: ${inst.release.tag} is installed and you chose ${release.tag}. Only do this if you were asked to.`);
    else add('ok', `Upgrade from ${inst.release.tag} to ${release.tag}.`);
  } else if (inst.state === 'unknown') {
    add('warning', 'The firmware on the dice is not one of the published versions (it may be older or a custom build).');
  }

  if (release.prerelease) add('warning', `${release.tag} is a test version and may not work correctly.`);
  return checks;
}

export function hasBlocking(checks) {
  return checks.some((c) => c.level === 'error');
}
