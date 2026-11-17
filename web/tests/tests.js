// Browser tests for the parts that do not need a dice.
// Open web/tests/ via the local web server (see web/README.md).
import { md5Hex, sha256Hex } from '../js/hash.js';
import { parseConfig, validateConfig, findActiveConfig, worstLevel } from '../js/config.js';
import { parsePartitionTable, samePartitions, changedRange } from '../js/dice.js';
import { checkAppImage, installChecks, hasBlocking, compareVersions } from '../js/firmware.js';
import { openImage, emptyFilesystem, rootFiles, replaceConfig, isBlank } from '../js/storage.js';

const out = document.getElementById('out');
let passed = 0;
let failed = 0;
function ok(cond, name, detail = '') {
  const line = document.createElement('div');
  line.className = cond ? 'pass' : 'fail';
  line.textContent = `${cond ? 'PASS' : 'FAIL'}  ${name}${!cond && detail ? `  -> ${detail}` : ''}`;
  out.append(line);
  cond ? passed++ : failed++;
}
const enc = (s) => new TextEncoder().encode(s);
const levels = (v) => v.issues.map((i) => i.level);

async function run() {
  // --- hashes ---------------------------------------------------------------
  ok(md5Hex(enc('')) === 'd41d8cd98f00b204e9800998ecf8427e', 'md5 empty');
  ok(md5Hex(enc('The quick brown fox jumps over the lazy dog')) === '9e107d9d372bb6826bd81d3542a419d6', 'md5 fox');
  ok(md5Hex(new Uint8Array(1000).fill(0xff)) === md5Hex(new Uint8Array(1000).fill(0xff)), 'md5 deterministic');

  // --- manifest + real release files -----------------------------------------
  const manifest = await (await fetch('../firmware/manifest.json')).json();
  const rel = manifest.releases[0];
  ok(!!rel, 'manifest has a release', JSON.stringify(manifest.rejected));
  const app = new Uint8Array(await (await fetch(`../${rel.app.file}`)).arrayBuffer());
  const merged = new Uint8Array(await (await fetch(`../${rel.merged.file}`)).arrayBuffer());
  ok(app.length === rel.app.size, 'app size matches manifest');
  ok(md5Hex(app) === rel.app.md5, 'app md5 (JS) matches manifest (Python)');
  ok((await sha256Hex(app)) === rel.app.sha256, 'app sha256 matches');
  ok((await sha256Hex(merged)) === rel.merged.sha256, 'merged sha256 matches');
  ok(checkAppImage(app) === null, 'app image passes check');
  ok(checkAppImage(merged.subarray(manifest.app0Offset)) === null, 'merged app part passes check');
  ok(checkAppImage(enc('hello world, this is not firmware at all......................')) !== null, 'text file rejected as firmware');
  const fakeChip = app.slice(0, 64); fakeChip[12] = 0; // ESP32 instead of S3
  ok(checkAppImage(fakeChip) !== null, 'firmware for other chip rejected');

  const parts = parsePartitionTable(merged.subarray(0x8000, 0x8000 + 0xc00));
  ok(samePartitions(parts, rel.partitions), 'partition table parsed in JS equals manifest');
  ok(parts.find((p) => p.label === 'littlefs')?.offset === 0x810000, 'littlefs at 0x810000');
  ok(parsePartitionTable(new Uint8Array(0xc00).fill(0x12)) === null, 'garbage partition table detected');

  // --- install checks ---------------------------------------------------------
  const goodInfo = { isEsp32s3: true, chipName: 'ESP32-S3', flashSize: '16MB', partitions: rel.partitions, installed: { state: 'known', release: rel } };
  ok(!hasBlocking(installChecks(goodInfo, rel, 'update')), 'update allowed on matching dice');
  ok(!hasBlocking(installChecks(goodInfo, rel, 'full')), 'full allowed on matching dice');
  const otherLayout = { ...goodInfo, partitions: rel.partitions.slice(0, 3), installed: { state: 'unknown' } };
  ok(hasBlocking(installChecks(otherLayout, rel, 'update')), 'update blocked with different layout');
  ok(!hasBlocking(installChecks(otherLayout, rel, 'full')), 'full allowed with different layout');
  ok(hasBlocking(installChecks({ ...goodInfo, flashSize: '8MB' }, rel, 'full')), '8MB flash blocked');
  ok(hasBlocking(installChecks({ ...goodInfo, isEsp32s3: false, chipName: 'ESP32' }, rel, 'full')), 'non-S3 blocked');
  ok(hasBlocking(installChecks({ ...goodInfo, installed: { state: 'empty' } }, rel, 'update')), 'update blocked on empty dice');
  const newer = { ...rel, tag: 'Gen9.0', version: [9, 0, 0] };
  const downgrade = installChecks({ ...goodInfo, installed: { state: 'known', release: newer } }, rel, 'update');
  ok(downgrade.some((c) => c.level === 'warning' && /OLDER/.test(c.msg)), 'downgrade warning');
  ok(compareVersions([4, 0, 1], [4, 0]) > 0 && compareVersions([4, 0], [4, 0, 0]) === 0, 'version compare');

  // --- config parsing & validation -------------------------------------------
  const refText = await (await fetch(`../${rel.exampleConfig}`)).text();
  const p = parseConfig('# c\n diceId = ABC # comment\r\nnoequals\nx_background=0x1F\n');
  ok(p.entries[0].key === 'diceId' && p.entries[0].value === 'ABC', 'parse key/value with inline comment and CRLF');
  ok(p.badLines.length === 1 && p.badLines[0].line === 3, 'line without = reported');

  let v = validateConfig('EXAMPLE_config.txt', enc(refText), refText, rel.tag);
  ok(worstLevel(v.issues) !== 'error', 'reference example validates without errors', JSON.stringify(v.issues));

  v = validateConfig('MyDice_config.txt', enc('diceId=MyDice\nx_background=70000\nchecksum=0\n'), refText, rel.tag);
  ok(levels(v).includes('error'), 'colour out of range is an error');
  v = validateConfig('MyDice_config.txt', enc('diceId=MyDice\nchecksum=5\n'), refText, rel.tag);
  ok(levels(v).includes('error'), 'non-zero checksum is an error');
  v = validateConfig('MyDice.txt', enc('diceId=MyDice\n'), refText, rel.tag);
  ok(levels(v).includes('error') && v.diceId === 'MyDice', 'wrong file name is an error, diceId still found');
  v = validateConfig('My Dice_config.txt', enc('diceId=MyDice\n'), refText, rel.tag);
  ok(levels(v).includes('error'), 'space in file name is an error');
  v = validateConfig('MyDice_config.txt', enc('diceId=MyDice\nfoo=1\n'), refText, rel.tag);
  ok(v.issues.some((i) => i.level === 'warning' && /foo/.test(i.msg)), 'unknown key is a warning');
  v = validateConfig('MyDice_config.txt', new Uint8Array([0xff, 0xfe, 0x00, 0x41]), refText, rel.tag);
  ok(levels(v).includes('error'), 'binary file rejected');
  v = validateConfig('MyDice_config.txt', enc('diceId=MyDice\nx_background=010\n'), refText, rel.tag);
  ok(v.issues.some((i) => /octal/.test(i.msg)), 'leading zero (octal) warned');
  // entang_colors is only read by Gen4.0, so check the rule without a release reference.
  v = validateConfig('MyDice_config.txt', enc('diceId=MyDice\nentang_colors=1,2,3,4,5,6,7,8,9\n'), null, 'none');
  ok(v.issues.some((i) => /first 8/.test(i.msg)), 'more than 8 colours warned');
  v = validateConfig('MyDice_config.txt', enc('diceId=MyDice\nanimationDuration=70000\nanimationSpins=300\n'), null, 'none');
  ok(v.issues.filter((i) => i.level === 'error' && /animation(Duration|Spins)/.test(i.msg)).length === 2, 'animation settings out of range');
  ok(/FW:\s*(.+?)\s*$/.exec('FW: Gen 4.1\r')?.[1] === 'Gen 4.1', 'firmware version with a space is read completely');
  v = validateConfig('MyDice_config.txt', enc('diceId=MyDice\ndeepSleepTimeout=1000\n'), refText, rel.tag);
  ok(v.issues.some((i) => i.level === 'warning' && /sleep/.test(i.msg)), 'very short sleep time warned');

  // main-branch example (newer keys) checked against the Gen4.0 reference
  try {
    const mainExample = await (await fetch(`https://raw.githubusercontent.com/${manifest.repo}/main/firmware/EXAMPLE_config.txt`)).text();
    v = validateConfig('EXAMPLE_config.txt', enc(mainExample), refText, rel.tag);
    out.append(`\nmain-branch EXAMPLE_config.txt vs ${rel.tag} reference: ${JSON.stringify(v.issues.map((i) => `${i.level}: ${i.msg}`), null, 1)}\n\n`);
  } catch { /* offline */ }

  ok(findActiveConfig([{ name: 'A_config.txt', type: 'file' }, { name: 'DEFAULT_config.txt', type: 'file' }]).active === 'A_config.txt', 'active config = first match');
  ok(findActiveConfig([{ name: 'readme.txt', type: 'file' }]).active === null, 'no config found');

  // --- LittleFS round trip ----------------------------------------------------
  const lfs = rel.littlefs;
  const fs = await emptyFilesystem(lfs.size);
  fs.writeFile('/DEFAULT_config.txt', enc('diceId=DEFAULT\n'));
  fs.writeFile('/ZED_config.txt', enc('diceId=ZED\n'));
  fs.writeFile('/notes.txt', enc('hello'));
  const img1 = fs.toImage();
  ok(img1.length === lfs.size, 'image has partition size');
  const fs2 = await openImage(img1);
  const files = rootFiles(fs2);
  ok(files.length === 3, 'three files listed', JSON.stringify(files));
  out.append(`directory order: ${files.map((f) => f.name).join(', ')}\n`);
  ok(findActiveConfig(files).all.length === 2, 'two config files found');
  const img2 = replaceConfig(fs2, 'MyDice_config.txt', enc('diceId=MyDice\n'));
  const fs3 = await openImage(img2);
  const files3 = rootFiles(fs3);
  ok(findActiveConfig(files3).active === 'MyDice_config.txt' && findActiveConfig(files3).all.length === 1, 'replaceConfig leaves exactly one config');
  ok(files3.some((f) => f.name === 'notes.txt'), 'other files kept');
  ok(new TextDecoder().decode(fs3.readFile('/MyDice_config.txt')) === 'diceId=MyDice\n', 'content round trip');
  const r = changedRange(img1, img2);
  ok(r && r.start % 4096 === 0 && r.end <= img2.length && r.end - r.start < img2.length, 'changed range is partial and aligned', JSON.stringify(r));
  ok(changedRange(img2, img2) === null, 'no change -> null');
  ok(isBlank(new Uint8Array(8192).fill(0xff)) && !isBlank(img2), 'isBlank');
  let mountFailed = false;
  try { await openImage(new Uint8Array(lfs.size).fill(0xff)); } catch { mountFailed = true; }
  ok(mountFailed, 'blank image does not mount (handled as empty storage)');

  document.getElementById('summary').textContent = `${passed} passed, ${failed} failed`;
  document.getElementById('summary').className = failed ? 'fail' : 'pass';
  document.title = `TESTS ${failed ? 'FAILED' : 'PASSED'} ${passed}/${passed + failed}`;
}

run().catch((err) => {
  document.getElementById('summary').textContent = `Test run crashed: ${err.stack || err}`;
  document.title = 'TESTS CRASHED';
});
