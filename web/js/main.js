import { Dice, isWebSerialSupported, changedRange } from './dice.js';
import {
  loadManifest, versionLabel, downloadVerified, checkAppImage, inspectDice, installChecks, hasBlocking,
} from './firmware.js';
import { validateConfig, findActiveConfig, worstLevel, CONFIG_SUFFIX } from './config.js';
import { openImage, emptyFilesystem, rootFiles, replaceConfig, isBlank } from './storage.js';

const $ = (id) => document.getElementById(id);

const state = {
  manifest: null,
  info: null,          // result of inspectDice for the connected dice
  busy: false,
  reportedFw: null,    // "FW: ..." line from the serial monitor
  reportedName: null,  // "Dice ID: ..." line from the serial monitor
  lastBackup: null,    // settings saved before a complete install
  pendingConfig: null, // { name, bytes, validation } chosen for upload
  references: new Map(),
};

const dice = new Dice();

// ---------------------------------------------------------------------------
// Small UI helpers
// ---------------------------------------------------------------------------

function el(tag, attrs = {}, ...children) {
  const e = document.createElement(tag);
  for (const [k, v] of Object.entries(attrs)) {
    if (k === 'class') e.className = v;
    else if (k.startsWith('on')) e.addEventListener(k.slice(2), v);
    else e.setAttribute(k, v);
  }
  for (const c of children) e.append(c);
  return e;
}

function renderList(ul, items) {
  ul.replaceChildren(...items.map((i) => el('li', { class: `lvl-${i.level}` }, i.msg)));
}

function formatBytes(n) {
  if (n >= 1024 * 1024) return `${(n / 1024 / 1024).toFixed(1)} MB`;
  if (n >= 1024) return `${Math.round(n / 1024)} KB`;
  return `${n} bytes`;
}

function saveFile(name, data, type = 'text/plain') {
  const url = URL.createObjectURL(new Blob([data], { type }));
  const a = el('a', { href: url, download: name });
  document.body.append(a);
  a.click();
  a.remove();
  setTimeout(() => URL.revokeObjectURL(url), 1000);
}

function techlog(msg) {
  const pre = $('techlog');
  const time = new Date().toLocaleTimeString();
  pre.textContent += `[${time}] ${msg}\n`;
  if (pre.textContent.length > 200000) pre.textContent = pre.textContent.slice(-150000);
  pre.scrollTop = pre.scrollHeight;
}

// ---------------------------------------------------------------------------
// Confirmation dialog
// ---------------------------------------------------------------------------

function confirmDialog({ title, body, ok, checkText }) {
  return new Promise((resolve) => {
    const dlg = $('confirm');
    $('confirm-title').textContent = title;
    $('confirm-body').replaceChildren(...body);
    $('btn-confirm-ok').textContent = ok;
    const check = $('confirm-check');
    $('confirm-check-wrap').hidden = !checkText;
    check.checked = false;
    $('confirm-check-text').textContent = checkText || '';
    $('btn-confirm-ok').disabled = !!checkText;
    check.onchange = () => { $('btn-confirm-ok').disabled = !check.checked; };
    const finish = (v) => { dlg.close(); resolve(v); };
    $('btn-confirm-ok').onclick = () => finish(true);
    $('btn-confirm-cancel').onclick = () => finish(false);
    dlg.oncancel = (e) => { e.preventDefault(); finish(false); };
    dlg.showModal();
  });
}

// ---------------------------------------------------------------------------
// Progress dialog
// ---------------------------------------------------------------------------

function onBeforeUnload(e) {
  e.preventDefault();
  e.returnValue = '';
}

/**
 * Run a long task with a progress dialog that cannot be closed until done.
 * `fn(progress)` receives { step(text), set(done,total), unknown() }.
 */
async function runBusy(title, { dangerous = false }, fn) {
  const dlg = $('busy');
  $('busy-title').textContent = title;
  $('busy-unplug').hidden = !dangerous;
  $('busy-step').textContent = '';
  $('busy-percent').textContent = '';
  $('busy-result').hidden = true;
  $('busy-result').replaceChildren();
  $('busy-actions').hidden = true;
  const bar = $('busy-bar');
  const barWrap = bar.parentElement;
  barWrap.hidden = false;
  barWrap.classList.add('indeterminate');
  bar.style.width = '';
  dlg.oncancel = (e) => e.preventDefault(); // Esc must not close it
  dlg.showModal();
  state.busy = true;
  updateButtons();
  window.addEventListener('beforeunload', onBeforeUnload);

  const progress = {
    step(text) {
      $('busy-step').textContent = text;
      $('busy-percent').textContent = '';
      barWrap.classList.add('indeterminate');
      bar.style.width = '';
      techlog(text);
    },
    set(done, total) {
      const pct = total ? Math.min(100, Math.floor((done / total) * 100)) : 0;
      barWrap.classList.remove('indeterminate');
      bar.style.width = `${pct}%`;
      $('busy-percent').textContent = `${pct}%`;
    },
    unknown() {
      barWrap.classList.add('indeterminate');
      bar.style.width = '';
      $('busy-percent').textContent = '';
    },
  };

  let result;
  let error = null;
  try {
    result = await fn(progress);
  } catch (err) {
    error = err;
    techlog(`ERROR: ${err.message || err}`);
    console.error(err);
  } finally {
    window.removeEventListener('beforeunload', onBeforeUnload);
    state.busy = false;
    updateButtons();
  }

  barWrap.hidden = true;
  $('busy-percent').textContent = '';
  $('busy-unplug').hidden = true;
  return { result, error, dlg };
}

/** Show the outcome in the (still open) progress dialog and wait for Close. */
function finishBusy(dlg, { ok, title, lines = [], extra = [] }) {
  return new Promise((resolve) => {
    $('busy-title').textContent = title;
    $('busy-step').textContent = '';
    const box = $('busy-result');
    box.hidden = false;
    box.replaceChildren(
      el('div', { class: `banner ${ok ? 'banner-ok' : 'banner-error'}` }, ...lines.map((l) => el('p', {}, l))),
      ...extra,
    );
    $('busy-actions').hidden = false;
    dlg.oncancel = (e) => { e.preventDefault(); dlg.close(); resolve(); };
    $('btn-busy-close').onclick = () => { dlg.close(); resolve(); };
    $('btn-busy-close').focus();
  });
}

// ---------------------------------------------------------------------------
// Buttons and status
// ---------------------------------------------------------------------------

function selectedRelease() {
  const tag = $('fw-version').value;
  return state.manifest?.releases.find((r) => r.tag === tag) ?? null;
}

function selectedMode() {
  return document.querySelector('input[name="fw-mode"]:checked').value;
}

function updateButtons() {
  const connected = dice.connected;
  const idle = !state.busy;
  $('btn-connect').hidden = connected;
  $('btn-connect').disabled = !idle || !isWebSerialSupported();
  $('btn-disconnect').hidden = !connected;
  $('btn-disconnect').disabled = !idle;
  $('btn-recheck').hidden = !connected;
  $('btn-recheck').disabled = !idle;
  $('connect-intro').hidden = connected && !!state.info;

  const release = selectedRelease();
  const checks = release ? installChecks(state.info, release, selectedMode()) : [];
  $('btn-install').disabled = !idle || !connected || !state.info || !release || hasBlocking(checks);

  const canTalk = idle && connected && state.info?.isEsp32s3;
  $('btn-read-config').disabled = !canTalk;
  $('config-file').disabled = !canTalk;
  $('lbl-config-file').setAttribute('aria-disabled', String(!canTalk));
  const writeBtn = $('btn-write-config');
  if (writeBtn) writeBtn.disabled = !canTalk || !state.pendingConfig || worstLevel(state.pendingConfig.validation.issues) === 'error';

  $('btn-mon-restart').disabled = !idle || dice.mode !== 'monitor';
}

function renderChecks() {
  const release = selectedRelease();
  if (!release) {
    renderList($('fw-checks'), [{ level: 'info', msg: 'No firmware version selected.' }]);
  } else if (!state.info) {
    renderList($('fw-checks'), [{ level: 'info', msg: 'Connect your dice to run the checks.' }]);
  } else {
    renderList($('fw-checks'), installChecks(state.info, release, selectedMode()));
  }
  updateButtons();
  renderReferenceNote();
}

function renderReleaseInfo() {
  const r = selectedRelease();
  const p = $('fw-release-info');
  p.replaceChildren();
  if (!r) return;
  const date = r.published ? new Date(r.published).toLocaleDateString('en-GB',{ year: 'numeric', month: 'long', day: 'numeric' }) : '';
  p.append(`${r.name}${date ? ` · released ${date}` : ''} · `, el('a', { href: r.url, target: '_blank', rel: 'noopener' }, "What's new"));
}

function renderDeviceInfo() {
  const info = state.info;
  $('device-info').hidden = !dice.connected;
  const status = {
    monitor: 'Connected',
    bootloader: 'Busy (programming mode)',
    closed: 'Not connected',
    closing: 'Disconnecting…',
  }[dice.mode] ?? dice.mode;
  $('di-status').textContent = status;
  $('di-chip').textContent = info ? (info.isEsp32s3 ? 'ESP32-S3 ✓' : `${info.chipName} — not a Quantum Dice`) : '–';
  $('di-flash').textContent = info?.flashSize ?? '–';
  let fw = '–';
  if (info?.installed) {
    if (info.installed.state === 'known') fw = info.installed.release.tag;
    else if (info.installed.state === 'empty') fw = 'None (empty)';
    else fw = 'Not a published version (older or custom)';
  }
  if (state.reportedFw) fw += ` · the dice reports "FW: ${state.reportedFw}"`;
  $('di-firmware').textContent = fw;
  $('di-name').textContent = state.reportedName ?? '– (shown when the dice starts)';
  $('di-mac').textContent = info?.mac || '–';

  const pill = $('mon-status');
  pill.textContent = dice.mode === 'monitor' ? 'Connected' : dice.mode === 'bootloader' ? 'Paused (programming)' : 'Not connected';
  pill.className = `pill ${dice.mode === 'monitor' ? 'pill-on' : dice.mode === 'bootloader' ? 'pill-busy' : 'pill-off'}`;
}

function connMessage(level, text) {
  const box = $('conn-message');
  box.hidden = !text;
  box.className = `note lvl-${level}`;
  box.textContent = text || '';
}

// ---------------------------------------------------------------------------
// Serial monitor
// ---------------------------------------------------------------------------

let termBuffer = '';
let lineBuffer = '';
let renderPending = false;
const ANSI = /\x1b\[[0-9;?]*[A-Za-z]/g;

function onSerialText(text) {
  const clean = text.replace(ANSI, '');
  termBuffer += clean;
  if (termBuffer.length > 300000) termBuffer = termBuffer.slice(-200000);

  lineBuffer += clean;
  const lines = lineBuffer.split('\n');
  lineBuffer = lines.pop();
  for (const line of lines) {
    const fw = /FW:\s*(\S+)/.exec(line);
    if (fw) { state.reportedFw = fw[1]; renderDeviceInfo(); }
    const id = /Dice ID:\s*(.+?)\s*$/.exec(line);
    if (id) { state.reportedName = id[1]; renderDeviceInfo(); }
  }

  if (!renderPending) {
    renderPending = true;
    requestAnimationFrame(() => {
      renderPending = false;
      const term = $('terminal');
      term.textContent = termBuffer;
      if ($('mon-autoscroll').checked) term.scrollTop = term.scrollHeight;
    });
  }
}

function terminalNote(text) {
  onSerialText(`\n--- ${text} ---\n`);
}

// ---------------------------------------------------------------------------
// Connect / inspect
// ---------------------------------------------------------------------------

async function inspect(title = 'Checking your dice') {
  const { result, error, dlg } = await runBusy(title, {}, async (p) => {
    p.step('Switching the dice to programming mode…');
    terminalNote('dice switched to programming mode');
    return await dice.withBootloader(async (s) => {
      p.step('Reading information from the dice…');
      return await inspectDice(s, state.manifest);
    });
  });
  if (error) {
    state.info = null;
    await finishBusy(dlg, { ok: false, title: 'Could not check the dice', lines: [error.message || String(error)] });
    connMessage('error', 'The dice could not be checked. See the tips in the user manual, then click "Check dice again".');
  } else {
    dlg.close();
    state.info = result;
    terminalNote('dice restarted');
    if (!result.isEsp32s3) connMessage('error', `This device (${result.chipName}) is not a Quantum Dice. Nothing will be written to it.`);
    else connMessage('', '');
  }
  renderDeviceInfo();
  renderChecks();
}

async function connect() {
  try {
    await dice.choosePort();
  } catch (err) {
    if (err?.name !== 'NotFoundError') connMessage('error', `Could not open the USB connection: ${err.message || err}`);
    else connMessage('warning', 'No dice selected. If the dice is not in the list: check that it is switched on and awake, and try another USB cable.');
    return;
  }
  state.reportedFw = null;
  state.reportedName = null;
  updateButtons();
  await inspect();
}

async function disconnect() {
  await dice.forget();
  state.info = null;
  connMessage('', '');
  renderDeviceInfo();
  renderChecks();
}

// ---------------------------------------------------------------------------
// Firmware installation
// ---------------------------------------------------------------------------

async function install() {
  const release = selectedRelease();
  const mode = selectedMode();
  const keepSettings = mode === 'update' || $('fw-backup').checked;
  const checks = installChecks(state.info, release, mode);
  if (hasBlocking(checks)) return;

  const warnings = checks.filter((c) => c.level === 'warning');
  const body = [];
  if (mode === 'update') {
    body.push(el('p', {}, `The firmware on your dice will be updated to `, el('strong', {}, release.tag), '. Your settings are kept.'));
  } else {
    body.push(el('p', {}, el('strong', {}, 'Everything on the dice will be erased'), ` and firmware `, el('strong', {}, release.tag), ' will be installed.'));
    body.push(el('p', {}, keepSettings
      ? 'Your settings are saved first and put back afterwards.'
      : 'Your settings will NOT be kept: the dice starts with default settings and the name "DEFAULT".'));
  }
  if (warnings.length) body.push(el('ul', {}, ...warnings.map((w) => el('li', {}, w.msg))));
  body.push(el('div', { class: 'banner banner-warning' },
    el('strong', {}, 'Do not unplug the dice or close this page during the installation.'),
    ' It takes about one to two minutes.'));

  const ok = await confirmDialog({
    title: mode === 'update' ? 'Update firmware?' : 'Complete install?',
    body,
    ok: mode === 'update' ? 'Update now' : 'Erase and install',
    checkText: mode === 'full' && !keepSettings
      ? 'I understand that all settings on the dice will be lost.'
      : (warnings.length ? 'I have read the warnings above and want to continue.' : null),
  });
  if (!ok) return;

  state.lastBackup = null;
  const outcome = { restored: null, restoreNote: null };

  const { error, dlg } = await runBusy(mode === 'update' ? 'Updating firmware' : 'Installing firmware', { dangerous: true }, async (p) => {
    const fileInfo = mode === 'full' ? release.merged : release.app;
    p.step('Downloading firmware…');
    const data = await downloadVerified(fileInfo, (d, t) => p.set(d, t));
    const appPart = mode === 'full' ? data.subarray(state.manifest.app0Offset) : data;
    const problem = checkAppImage(appPart);
    if (problem) throw new Error(`${problem} Nothing was written to the dice.`);

    p.step('Switching the dice to programming mode…');
    terminalNote('dice switched to programming mode');
    await dice.withBootloader(async (s) => {
      // Check again: the user may have plugged in a different dice.
      p.step('Checking the dice…');
      const info = await inspectDice(s, state.manifest);
      state.info = info;
      const recheck = installChecks(info, release, mode);
      if (hasBlocking(recheck)) {
        throw new Error(`Installation stopped, nothing was written: ${recheck.find((c) => c.level === 'error').msg}`);
      }

      if (mode === 'update') {
        p.step('Writing firmware…');
        await s.writeAndVerify(state.manifest.app0Offset, data, (d, t) => p.set(d, t));
        // Make sure the dice starts the new firmware (app0) and not an old copy.
        const otadata = release.otadata;
        if (otadata) {
          const cur = await s.read(otadata.offset, otadata.size);
          if (!isBlank(cur)) {
            p.step('Selecting the new firmware…');
            await s.writeAndVerify(otadata.offset, new Uint8Array(otadata.size).fill(0xff));
          }
        }
      } else {
        let backup = null;
        if (keepSettings) {
          const lfs = info.partitions?.find((x) => x.label === 'littlefs' && x.type === 1);
          if (lfs) {
            p.step('Saving your settings…');
            const image = await s.read(lfs.offset, lfs.size, (d, t) => p.set(d, t));
            backup = await makeBackup(image, lfs);
            state.lastBackup = backup;
            if (!backup.files.length) outcome.restoreNote = 'No settings file was found on the dice, so there was nothing to keep.';
          } else {
            outcome.restoreNote = 'This dice had no settings storage, so there was nothing to keep.';
          }
        }

        p.step('Erasing the dice… (this can take up to a minute)');
        p.unknown();
        await s.eraseAll();

        p.step('Writing firmware…');
        await s.writeAndVerify(0, data, (d, t) => p.set(d, t));

        if (backup?.files.length) {
          p.step('Putting your settings back…');
          const target = release.littlefs;
          let image;
          if (backup.part.offset === target.offset && backup.part.size === target.size && backup.mountable) {
            image = backup.image;
          } else {
            const fs = await emptyFilesystem(target.size);
            for (const f of backup.files) fs.writeFile(`/${f.name}`, f.bytes);
            image = fs.toImage();
          }
          const range = changedRange(new Uint8Array(image.length).fill(0xff), image);
          if (range) {
            await s.writeAndVerify(target.offset + range.start, image.subarray(range.start, range.end), (d, t) => p.set(d, t));
          }
          outcome.restored = backup;
        }
      }
      state.info = { ...info, partitions: release.partitions, installed: { state: 'known', release } };
      p.step('Restarting the dice…');
    });
    terminalNote('dice restarted');
  });

  renderDeviceInfo();
  renderChecks();

  if (error) {
    const extra = [];
    if (state.lastBackup?.files.length) {
      extra.push(el('p', {}, 'Your settings were saved before the dice was erased. Keep a copy on this computer:'));
      extra.push(backupButtons(state.lastBackup));
    }
    await finishBusy(dlg, {
      ok: false,
      title: 'Installation did not finish',
      lines: [
        error.message || String(error),
        'Do not worry: the dice cannot be damaged by this. Check the cable and that the dice is awake, then try again. ' +
          'If an update keeps failing, use "Complete install".',
      ],
      extra,
    });
    return;
  }

  const lines = [`Firmware ${release.tag} is installed and checked. The dice restarts now.`];
  const extra = [];
  if (outcome.restoreNote) lines.push(outcome.restoreNote);
  if (outcome.restored) {
    const active = outcome.restored.active;
    lines.push(`Your settings file "${active}" has been put back.`);
    const ref = await getReference(release);
    const v = validateConfig(active, outcome.restored.files.find((f) => f.name === active).bytes, ref.text, ref.label);
    const relevant = v.issues.filter((i) => i.level === 'error' || i.level === 'warning');
    if (relevant.length) {
      extra.push(el('p', {}, `Your settings file does not fully match firmware ${release.tag}. You may want to update it (see step 3):`));
      const ul = el('ul', { class: 'checks' });
      renderList(ul, relevant);
      extra.push(ul);
    }
    extra.push(backupButtons(outcome.restored));
  }
  await finishBusy(dlg, { ok: true, title: 'Done!', lines, extra });
}

async function makeBackup(image, part) {
  const backup = { image, part, files: [], active: null, mountable: false };
  if (isBlank(image)) return backup;
  try {
    const fs = await openImage(image);
    backup.mountable = true;
    const files = rootFiles(fs);
    for (const f of files) backup.files.push({ name: f.name, bytes: fs.readFile(`/${f.name}`) });
    backup.active = findActiveConfig(files).active;
  } catch (err) {
    techlog(`Settings storage could not be read: ${err.message || err}`);
  }
  // Only keep it if it contains a settings file.
  if (!backup.active) backup.files = [];
  return backup;
}

function backupButtons(backup) {
  const f = backup.files.find((x) => x.name === backup.active);
  return el('div', { class: 'actions' },
    el('button', { class: 'btn', onclick: () => saveFile(f.name, f.bytes) }, `Save "${f.name}" to this computer`));
}

// ---------------------------------------------------------------------------
// Settings file
// ---------------------------------------------------------------------------

/** EXAMPLE_config.txt belonging to a release (the reference for checks). */
async function getReference(release) {
  if (!release) return { text: null, label: 'unknown' };
  if (state.references.has(release.tag)) return state.references.get(release.tag);
  let text = null;
  if (release.exampleConfig) {
    try {
      const res = await fetch(release.exampleConfig, { cache: 'no-cache' });
      if (res.ok) text = await res.text();
    } catch { /* offline */ }
  }
  const ref = { text, label: release.tag, url: release.exampleConfig };
  state.references.set(release.tag, ref);
  return ref;
}

function referenceRelease() {
  if (state.info?.installed?.state === 'known') return { release: state.info.installed.release, guessed: false };
  return { release: state.manifest?.releases[0] ?? null, guessed: true };
}

async function renderReferenceNote() {
  const { release, guessed } = referenceRelease();
  const p = $('config-reference');
  if (!release) { p.replaceChildren(); return; }
  const ref = await getReference(release);
  if (!ref.url) { p.replaceChildren(); return; }
  p.replaceChildren(
    `Settings are checked against the example settings file of firmware ${release.tag}`,
    guessed ? ' (the newest version, because the version on the dice is not known)' : '',
    '. ',
    el('a', { href: ref.url, download: 'EXAMPLE_config.txt' }, 'Download the example settings file'),
    ' to see all settings with an explanation.',
  );
}

function showConfigResult({ title, text, issues = [], preview = null, actions = [] }) {
  $('config-result').hidden = false;
  $('config-result-title').textContent = title;
  $('config-result-text').textContent = text || '';
  $('config-result-text').hidden = !text;
  renderList($('config-issues'), issues);
  $('config-preview-wrap').hidden = preview === null;
  $('config-preview').textContent = preview ?? '';
  $('config-actions').replaceChildren(...actions);
  updateButtons();
}

async function readSettings() {
  state.pendingConfig = null;
  const { result, error, dlg } = await runBusy('Reading settings', {}, async (p) => {
    p.step('Switching the dice to programming mode…');
    terminalNote('dice switched to programming mode');
    const res = await dice.withBootloader(async (s) => {
      p.step('Reading the settings storage…');
      const parts = await s.readPartitionTable();
      const lfs = parts?.find((x) => x.label === 'littlefs' && x.type === 1);
      if (!lfs) throw new Error('This dice has no settings storage. Do a "Complete install" first (step 2).');
      return await s.read(lfs.offset, lfs.size, (d, t) => p.set(d, t));
    });
    terminalNote('dice restarted');
    return res;
  });
  if (error) {
    await finishBusy(dlg, { ok: false, title: 'Could not read the settings', lines: [error.message || String(error)] });
    return;
  }
  dlg.close();

  let fs = null;
  if (!isBlank(result)) {
    try { fs = await openImage(result); } catch (err) { techlog(`Mount failed: ${err.message || err}`); }
  }
  if (!fs) {
    showConfigResult({
      title: 'No settings file on the dice',
      text: 'The settings storage is empty. The dice creates "DEFAULT_config.txt" with default settings when it starts. You can put your own settings file on the dice with "Choose file…".',
    });
    return;
  }
  const files = rootFiles(fs);
  const { active, all } = findActiveConfig(files);
  if (!active) {
    showConfigResult({
      title: 'No settings file on the dice',
      text: 'The dice uses its built-in default settings. You can put your own settings file on the dice with "Choose file…".',
    });
    return;
  }
  const bytes = fs.readFile(`/${active}`);
  const { release } = referenceRelease();
  const ref = await getReference(release);
  const v = validateConfig(active, bytes, ref.text, ref.label);
  const issues = [...v.issues];
  if (all.length > 1) {
    issues.unshift({
      level: 'warning',
      msg: `There are ${all.length} settings files on the dice (${all.join(', ')}). The dice only uses "${active}". When you put a new settings file on the dice, the others are removed.`,
    });
  }
  const worst = worstLevel(v.issues);
  if (worst === 'ok') issues.unshift({ level: 'ok', msg: 'The settings file looks correct.' });
  showConfigResult({
    title: `Settings file on the dice: ${active}`,
    text: v.diceId ? `Dice name: ${v.diceId}` : '',
    issues,
    preview: v.text ?? '(not a text file)',
    actions: [el('button', { class: 'btn btn-primary', onclick: () => saveFile(active, bytes) }, 'Save to this computer')],
  });
}

async function chooseConfigFile(file, overrideName) {
  const bytes = new Uint8Array(await file.arrayBuffer());
  const name = overrideName ?? file.name;
  const { release, guessed } = referenceRelease();
  const ref = await getReference(release);
  const v = validateConfig(name, bytes, ref.text, ref.label);
  state.pendingConfig = { name, bytes, validation: v, file };

  const actions = [];
  const issues = [...v.issues];
  if (guessed) issues.push({ level: 'info', msg: `Checked against firmware ${release?.tag}, because the firmware on the dice is not a published version.` });

  // Offer an automatic fix for the most common mistake: the file name.
  const suggested = v.diceId ? `${v.diceId}${CONFIG_SUFFIX}` : null;
  if (!name.endsWith(CONFIG_SUFFIX) || !/^[A-Za-z0-9_.-]+$/.test(name)) {
    if (suggested && /^[A-Za-z0-9_.-]+$/.test(suggested)) {
      actions.push(el('button', { class: 'btn', onclick: () => chooseConfigFile(file, suggested) }, `Use the name "${suggested}"`));
    }
  }
  const worst = worstLevel(v.issues);
  const writeBtn = el('button', { class: 'btn btn-primary', id: 'btn-write-config', onclick: () => writeSettings() }, 'Write to dice');
  actions.unshift(writeBtn);
  if (worst === 'ok') issues.unshift({ level: 'ok', msg: 'The settings file looks correct.' });
  if (worst === 'error') issues.unshift({ level: 'error', msg: 'This file cannot be put on the dice. Fix the problems below and choose the file again.' });

  showConfigResult({
    title: `Selected file: ${name}`,
    text: v.diceId ? `Dice name in this file: ${v.diceId}` : '',
    issues,
    preview: v.text ?? '(not a text file)',
    actions,
  });
}

async function writeSettings() {
  const pending = state.pendingConfig;
  if (!pending) return;
  const v = pending.validation;
  if (worstLevel(v.issues) === 'error') return;
  const warnings = v.issues.filter((i) => i.level === 'warning');

  const body = [el('p', {}, 'The settings file ', el('strong', {}, pending.name), ' will be put on the dice. Any other settings file on the dice is removed.')];
  if (warnings.length) {
    body.push(el('p', {}, 'Please note:'));
    body.push(el('ul', {}, ...warnings.map((w) => el('li', {}, w.msg))));
  }
  body.push(el('div', { class: 'banner banner-warning' }, el('strong', {}, 'Do not unplug the dice or close this page'), ' until it is finished (up to a minute).'));
  const ok = await confirmDialog({
    title: 'Write settings to the dice?',
    body,
    ok: 'Write settings',
    checkText: warnings.length ? 'I have read the notes above and want to continue.' : null,
  });
  if (!ok) return;

  const { error, dlg } = await runBusy('Writing settings', { dangerous: true }, async (p) => {
    p.step('Switching the dice to programming mode…');
    terminalNote('dice switched to programming mode');
    await dice.withBootloader(async (s) => {
      const parts = await s.readPartitionTable();
      const lfs = parts?.find((x) => x.label === 'littlefs' && x.type === 1);
      if (!lfs) throw new Error('This dice has no settings storage. Do a "Complete install" first (step 2).');
      p.step('Reading the settings storage…');
      const image = await s.read(lfs.offset, lfs.size, (d, t) => p.set(d, t));
      let fs = null;
      if (!isBlank(image)) {
        try { fs = await openImage(image); } catch (err) { techlog(`Existing storage unreadable, creating a new one: ${err.message || err}`); }
      }
      if (!fs) fs = await emptyFilesystem(lfs.size);
      const newImage = replaceConfig(fs, pending.name, pending.bytes);

      // Check the result before writing it.
      const check = await openImage(newImage);
      const after = findActiveConfig(rootFiles(check));
      const back = check.readFile(`/${pending.name}`);
      if (after.active !== pending.name || after.all.length !== 1 || back.length !== pending.bytes.length) {
        throw new Error('Internal check of the new settings storage failed. Nothing was written.');
      }

      const range = changedRange(image, newImage);
      if (range) {
        p.step('Writing settings…');
        await s.writeAndVerify(lfs.offset + range.start, newImage.subarray(range.start, range.end), (d, t) => p.set(d, t));
      }
      p.step('Restarting the dice…');
    });
    terminalNote('dice restarted');
  });

  if (error) {
    await finishBusy(dlg, { ok: false, title: 'Settings were not written', lines: [error.message || String(error), 'Check the cable and that the dice is awake, then try again.'] });
    return;
  }
  state.reportedName = null;
  renderDeviceInfo();
  await finishBusy(dlg, {
    ok: true,
    title: 'Settings written',
    lines: [`"${pending.name}" is now on the dice. The dice restarts and uses the new settings. You can see them in the serial monitor (step 4).`],
  });
  showConfigResult({
    title: `Settings file on the dice: ${pending.name}`,
    text: v.diceId ? `Dice name: ${v.diceId}` : '',
    issues: [{ level: 'ok', msg: 'Written and checked.' }],
    preview: v.text,
    actions: [],
  });
  state.pendingConfig = null;
  $('config-file').value = '';
}

// ---------------------------------------------------------------------------
// Start-up
// ---------------------------------------------------------------------------

async function init() {
  if (!isWebSerialSupported()) {
    $('unsupported').hidden = false;
  }

  dice.addEventListener('serial', (e) => onSerialText(e.detail));
  dice.addEventListener('log', (e) => techlog(e.detail));
  dice.addEventListener('mode', () => { renderDeviceInfo(); updateButtons(); });
  dice.addEventListener('portlost', () => {
    if (state.busy) return;
    terminalNote('dice disconnected');
    connMessage('warning', 'The dice was disconnected (unplugged, or it went to sleep). Wake it up or plug it in again; the monitor reconnects automatically.');
  });

  $('btn-connect').addEventListener('click', connect);
  $('btn-disconnect').addEventListener('click', disconnect);
  $('btn-recheck').addEventListener('click', () => inspect('Checking your dice'));
  $('btn-install').addEventListener('click', install);
  $('fw-version').addEventListener('change', () => { renderReleaseInfo(); renderChecks(); });
  document.querySelectorAll('input[name="fw-mode"]').forEach((r) => r.addEventListener('change', renderChecks));
  $('btn-read-config').addEventListener('click', readSettings);
  $('config-file').addEventListener('change', (e) => { const f = e.target.files?.[0]; if (f) chooseConfigFile(f); });
  $('btn-mon-restart').addEventListener('click', async () => {
    terminalNote('restarting dice');
    try { await dice.restartFirmware(); } catch (err) { techlog(err.message); }
  });
  $('btn-mon-clear').addEventListener('click', () => { termBuffer = ''; $('terminal').textContent = ''; });
  $('btn-mon-save').addEventListener('click', () => {
    const stamp = new Date().toISOString().slice(0, 19).replace(/[:T]/g, '-');
    saveFile(`quantum-dice-log-${stamp}.txt`, termBuffer);
  });

  try {
    state.manifest = await loadManifest();
    const sel = $('fw-version');
    sel.replaceChildren(...state.manifest.releases.map((r, i) =>
      el('option', { value: r.tag }, `${versionLabel(r)}${i === 0 ? ' — newest' : ''}`)));
    // Default to the newest regular (non-test) version.
    const stable = state.manifest.releases.find((r) => !r.prerelease);
    if (stable) sel.value = stable.tag;
    sel.disabled = false;
    renderReleaseInfo();
    renderReferenceNote();
  } catch (err) {
    $('fw-load-error').hidden = false;
    $('fw-load-error').textContent = err.message || String(err);
  }

  renderDeviceInfo();
  renderChecks();
}

init();
