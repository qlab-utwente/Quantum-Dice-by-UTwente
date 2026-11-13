// Quantum Dice configuration file (<diceId>_config.txt) handling.
//
// The parsing rules copy firmware/main/DiceConfigManager.cpp so that the page
// sees the file exactly the way the dice will. Which keys are valid comes from
// firmware/EXAMPLE_config.txt of the matching release (the "reference").

export const CONFIG_SUFFIX = '_config.txt';
export const MAX_CONFIG_BYTES = 16 * 1024;
export const MAX_NAME_LENGTH = 60; // LittleFS on the dice allows 64 incl. "/"
const MAX_ENTANGLEMENT_COLORS = 8; // DiceConfigManager.hpp

// ----------------------------------------------------------------------------
// Parsing (mirrors DiceConfigManager::load)
// ----------------------------------------------------------------------------

export function parseConfig(text) {
  const entries = [];
  const badLines = [];
  const lines = text.split('\n');
  lines.forEach((raw, idx) => {
    let line = raw.endsWith('\r') ? raw.slice(0, -1) : raw;
    line = line.trim();
    if (line === '' || line.startsWith('#')) return;
    const sep = line.indexOf('=');
    if (sep === -1) {
      badLines.push({ line: idx + 1, text: line });
      return;
    }
    const key = line.slice(0, sep).trim();
    let value = line.slice(sep + 1);
    const hash = value.indexOf('#');
    if (hash !== -1) value = value.slice(0, hash);
    value = value.trim();
    entries.push({ key, value, line: idx + 1 });
  });
  return { entries, badLines };
}

// ----------------------------------------------------------------------------
// Value rules for keys the firmware knows (any release). A key is only
// accepted when it also appears in the reference EXAMPLE_config.txt.
// ----------------------------------------------------------------------------

// C's strtoul/strtol with base 0: "0x.." hex, leading "0" octal, else decimal.
function parseCInteger(value) {
  const m = /^([+-]?)(0[xX][0-9a-fA-F]+|0[0-7]*|[1-9][0-9]*)$/.exec(value);
  if (!m) return null;
  const sign = m[1] === '-' ? -1 : 1;
  const body = m[2];
  let n;
  if (/^0[xX]/.test(body)) n = parseInt(body.slice(2), 16);
  else if (body.length > 1 && body.startsWith('0')) n = parseInt(body, 8);
  else n = parseInt(body, 10);
  return { n: sign * n, octal: body.length > 1 && body.startsWith('0') && !/^0[xX]/.test(body) };
}

function intRule({ min, max, warnMin, warnMax, unit = '', what }) {
  return (value) => {
    const r = parseCInteger(value);
    if (!r) return { level: 'error', msg: `"${value}" is not a whole number. ${what}` };
    const out = [];
    if (r.octal) out.push({ level: 'warning', msg: `"${value}" starts with 0, so the dice reads it as the octal number ${r.n}. Remove the leading 0.` });
    if (r.n < min || r.n > max) {
      out.push({ level: 'error', msg: `${r.n}${unit} is out of range (${min} to ${max}${unit}).` });
    } else if ((warnMin !== undefined && r.n < warnMin) || (warnMax !== undefined && r.n > warnMax)) {
      out.push({ level: 'warning', msg: `${r.n}${unit} is unusual (normally ${warnMin ?? min} to ${warnMax ?? max}${unit}).` });
    }
    return out;
  };
}

const color = intRule({ min: 0, max: 65535, what: 'Use a colour number between 0 and 65535 (RGB565).' });
const rssi = intRule({ min: -128, max: 127, warnMin: -100, warnMax: 0, unit: ' dBm', what: 'Use a negative number such as -25.' });
const millis16 = intRule({ min: 0, max: 65535, unit: ' ms', what: 'Use a time in milliseconds.' });

const RULES = {
  diceId: (value) => {
    if (value === '') return { level: 'error', msg: 'The dice name (diceId) is empty.' };
    if (value.length > 20) return { level: 'warning', msg: `The dice name is long (${value.length} characters) and may not fit on the screen.` };
    return null;
  },
  x_background: color,
  y_background: color,
  z_background: color,
  entang_colors: (value) => {
    const parts = value.split(',').map((s) => s.trim()).filter((s) => s !== '');
    if (parts.length === 0) return { level: 'error', msg: 'At least one entanglement colour is needed.' };
    const out = [];
    for (const p of parts) {
      const r = color(p);
      if (r) out.push(...[].concat(r));
    }
    if (parts.length > MAX_ENTANGLEMENT_COLORS) {
      out.push({ level: 'warning', msg: `${parts.length} colours given; the dice only uses the first ${MAX_ENTANGLEMENT_COLORS}.` });
    }
    return out;
  },
  colorFlashTimeout: millis16,
  observedTime: millis16,
  rssiLimit: rssi,
  rssiClose: rssi,
  rssiFar: rssi,
  rssiScreen: (value) => (/^(true|false|1|0)$/i.test(value) ? null
    : { level: 'warning', msg: `"${value}" is not true or false; the dice will treat it as false.` }),
  tumbleConstant: (value) => {
    if (!/^[+-]?(\d+\.?\d*|\.\d+)([eE][+-]?\d+)?$/.test(value)) return { level: 'error', msg: `"${value}" is not a number.` };
    if (Number(value) <= 0) return { level: 'error', msg: 'The tumble constant must be larger than 0.' };
    return null;
  },
  deepSleepTimeout: (value) => {
    const r = parseCInteger(value);
    if (!r) return { level: 'error', msg: `"${value}" is not a whole number of milliseconds.` };
    if (r.n < 0 || r.n > 4294967295) return { level: 'error', msg: 'The sleep time is out of range.' };
    if (r.n < 30000) return { level: 'warning', msg: `The dice will go to sleep after only ${(r.n / 1000).toFixed(1)} seconds without use.` };
    return null;
  },
  checksum: (value) => {
    const r = parseCInteger(value);
    if (!r || r.n !== 0) {
      return { level: 'error', msg: 'checksum must be 0. With any other value the dice rejects the file and uses its built-in default settings.' };
    }
    return null;
  },
};

// ----------------------------------------------------------------------------
// Validation
// ----------------------------------------------------------------------------

/**
 * Validate a config file.
 * @param {string} name        file name (without leading "/")
 * @param {Uint8Array} bytes   file content
 * @param {string|null} referenceText  EXAMPLE_config.txt of the matching firmware
 * @param {string} referenceLabel      e.g. "Gen4.0"
 * @returns {{issues: Array<{level:'error'|'warning'|'info', msg:string}>, values: Object, diceId: string|null, text: string|null}}
 */
export function validateConfig(name, bytes, referenceText, referenceLabel) {
  const issues = [];
  const add = (level, msg) => issues.push({ level, msg });

  // File name
  if (!name.endsWith(CONFIG_SUFFIX)) {
    add('error', `The file name must end with "${CONFIG_SUFFIX}" (for example "MyDice${CONFIG_SUFFIX}"), otherwise the dice will not find it.`);
  }
  if (name.length > MAX_NAME_LENGTH) add('error', `The file name is too long (maximum ${MAX_NAME_LENGTH} characters).`);
  if (!/^[A-Za-z0-9_.-]+$/.test(name)) add('error', 'Only letters, digits, "_", "-" and "." are allowed in the file name (no spaces).');

  // Content
  if (bytes.length === 0) {
    add('error', 'The file is empty.');
    return { issues, values: {}, diceId: null, text: null };
  }
  if (bytes.length > MAX_CONFIG_BYTES) {
    add('error', `The file is too large (${bytes.length} bytes). A settings file is normally about 2 KB.`);
    return { issues, values: {}, diceId: null, text: null };
  }
  let text;
  try {
    text = new TextDecoder('utf-8', { fatal: true }).decode(bytes);
  } catch {
    add('error', 'This is not a text file. Choose the .txt settings file.');
    return { issues, values: {}, diceId: null, text: null };
  }
  if (text.includes('\u0000')) {
    add('error', 'This is not a text file. Choose the .txt settings file.');
    return { issues, values: {}, diceId: null, text: null };
  }

  const { entries, badLines } = parseConfig(text);
  for (const b of badLines) add('warning', `Line ${b.line} is ignored because it has no "=": ${b.text}`);

  const refKeys = referenceText ? parseConfig(referenceText).entries.map((e) => e.key) : null;
  const values = {};
  const seen = new Set();
  for (const { key, value, line } of entries) {
    if (seen.has(key)) add('warning', `"${key}" appears more than once; the dice uses the last one (line ${line}).`);
    seen.add(key);
    values[key] = value;

    const known = refKeys ? refKeys.includes(key) : key in RULES;
    if (!known) {
      add('warning', `Line ${line}: "${key}" is not in the example settings file of firmware ${referenceLabel}. The dice probably ignores it; check the spelling.`);
      continue;
    }
    const rule = RULES[key];
    if (rule) {
      const res = rule(value);
      for (const r of [].concat(res || [])) add(r.level, `Line ${line} (${key}): ${r.msg}`);
    }
  }

  if (entries.length === 0) add('error', 'The file contains no settings.');
  if (!('diceId' in values)) add('warning', 'The dice name (diceId) is missing; the dice will be called "DEFAULT".');
  if (refKeys) {
    const missing = refKeys.filter((k) => !(k in values) && k !== 'diceId');
    if (missing.length) add('info', `Not set (the dice uses its default value): ${missing.join(', ')}.`);
  }
  if ('diceId' in values && values.diceId && name.endsWith(CONFIG_SUFFIX)) {
    const expected = `${values.diceId}${CONFIG_SUFFIX}`;
    if (name !== expected) add('info', `Tip: files are usually named after the dice: "${expected}".`);
  }

  return { issues, values, diceId: values.diceId ?? null, text };
}

export function worstLevel(issues) {
  if (issues.some((i) => i.level === 'error')) return 'error';
  if (issues.some((i) => i.level === 'warning')) return 'warning';
  return 'ok';
}

// ----------------------------------------------------------------------------
// Which file does the dice use? (mirrors findConfigFile in DiceConfigManager.cpp)
// ----------------------------------------------------------------------------
//
// The firmware walks the root directory in on-disk order and takes the first
// file whose name ends in "_config.txt". It contains a check meant to prefer a
// non-default file over "/DEFAULT_config.txt", but File::name() returns the name
// without "/", so that check never matches: in practice the first match wins.
// The LittleFS image is read in the same on-disk order here.

export function findActiveConfig(rootEntries) {
  const matches = rootEntries
    .filter((e) => e.type === 'file' && e.name.endsWith(CONFIG_SUFFIX))
    .map((e) => e.name);
  return { active: matches[0] ?? null, all: matches };
}
