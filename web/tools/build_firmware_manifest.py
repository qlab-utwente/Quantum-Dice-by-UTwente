#!/usr/bin/env python3
"""
Mirror Quantum Dice firmware releases into the website and write a manifest.

Why: a web page on github.io is not allowed (CORS) to download release assets
directly from github.com. This script copies the release .bin files next to
the website, checks that they are sane, and writes web/firmware/manifest.json
that the web page reads.

It is used by the GitHub Pages workflow, and you can run it locally too:

    python3 web/tools/build_firmware_manifest.py

Only the Python standard library is used. Set GITHUB_TOKEN to avoid the
anonymous API rate limit (the workflow does this automatically).
"""

import hashlib
import json
import os
import re
import struct
import sys
import urllib.error
import urllib.request
from datetime import datetime, timezone
from pathlib import Path

REPO = os.environ.get("QD_REPO", "qlab-utwente/Quantum-Dice-by-UTwente")
OUT_DIR = Path(__file__).resolve().parent.parent / "firmware"

# Releases are accepted from this generation onward (tag "Gen4.0" and newer).
MIN_GENERATION = (4, 0)
TAG_RE = re.compile(r"^Gen(\d+)\.(\d+)(?:\.(\d+))?$", re.IGNORECASE)

# Facts about the Quantum Dice hardware / firmware layout.
CHIP_ID_ESP32S3 = 9
FLASH_SIZE = 16 * 1024 * 1024
PARTITION_TABLE_OFFSET = 0x8000
APP0_OFFSET = 0x10000
MARKER_TEXT = b"QUANTUM DICE"  # printed by the firmware at start-up


def log(msg):
    print(msg, flush=True)


def http_get(url, accept=None):
    headers = {"User-Agent": "quantum-dice-pages-build"}
    if accept:
        headers["Accept"] = accept
    token = os.environ.get("GITHUB_TOKEN")
    if token and url.startswith("https://api.github.com/"):
        headers["Authorization"] = f"Bearer {token}"
    req = urllib.request.Request(url, headers=headers)
    with urllib.request.urlopen(req, timeout=120) as resp:
        return resp.read()


def tag_version(tag):
    m = TAG_RE.match(tag or "")
    if not m:
        return None
    return tuple(int(x) if x else 0 for x in m.groups())


def parse_partition_table(data):
    """Parse an ESP-IDF binary partition table (32-byte entries)."""
    parts = []
    for i in range(0, 0xC00, 32):
        entry = data[i : i + 32]
        if len(entry) < 32 or entry[0:2] == b"\xff\xff":
            break
        if entry[0:2] == b"\xeb\xeb":  # MD5 entry
            break
        if entry[0:2] != b"\xaa\x50":
            raise ValueError(f"bad partition entry magic at +0x{i:x}")
        ptype, subtype, offset, size = struct.unpack("<BBII", entry[2:12])
        label = entry[12:28].split(b"\x00")[0].decode("ascii", "replace")
        parts.append(
            {"label": label, "type": ptype, "subtype": subtype, "offset": offset, "size": size}
        )
    return parts


def check_app_image(data, where):
    """Basic checks of an ESP32 application image. Returns list of problems."""
    problems = []
    if len(data) < 64 or data[0] != 0xE9:
        problems.append(f"{where}: not an ESP32 firmware image (bad magic byte)")
        return problems
    chip_id = struct.unpack("<H", data[12:14])[0]
    if chip_id != CHIP_ID_ESP32S3:
        problems.append(f"{where}: built for chip id {chip_id}, expected ESP32-S3 ({CHIP_ID_ESP32S3})")
    if MARKER_TEXT not in data:
        problems.append(f"{where}: does not look like Quantum Dice firmware (marker text missing)")
    return problems


def process_release(rel):
    tag = rel["tag_name"]
    result = {"tag": tag, "problems": []}
    bins = [a for a in rel.get("assets", []) if a["name"].lower().endswith(".bin")]
    merged_assets = [a for a in bins if a["name"].lower().endswith(".merged.bin")]
    app_assets = [a for a in bins if not a["name"].lower().endswith(".merged.bin")]
    if len(merged_assets) != 1 or len(app_assets) != 1:
        result["problems"].append(
            f"expected exactly one *.bin and one *.merged.bin, found {[a['name'] for a in bins]}"
        )
        return result

    log(f"  downloading {app_assets[0]['name']}")
    app = http_get(app_assets[0]["url"], accept="application/octet-stream")
    log(f"  downloading {merged_assets[0]['name']}")
    merged = http_get(merged_assets[0]["url"], accept="application/octet-stream")

    problems = result["problems"]
    problems += check_app_image(app, app_assets[0]["name"])

    # Merged image: bootloader at 0, partition table at 0x8000, app at 0x10000.
    if len(merged) > FLASH_SIZE:
        problems.append("merged image is larger than the 16 MB flash")
    if len(merged) < APP0_OFFSET + len(app):
        problems.append("merged image is too small to contain the application")
    if merged[:1] != b"\xe9":
        problems.append("merged image does not start with a bootloader")
    try:
        partitions = parse_partition_table(merged[PARTITION_TABLE_OFFSET : PARTITION_TABLE_OFFSET + 0xC00])
    except ValueError as e:
        partitions = []
        problems.append(f"merged image partition table: {e}")
    app0 = next((p for p in partitions if p["type"] == 0 and p["label"] == "app0"), None)
    lfs = next((p for p in partitions if p["type"] == 1 and p["label"] == "littlefs"), None)
    otadata = next((p for p in partitions if p["type"] == 1 and p["subtype"] == 0), None)
    if not app0 or app0["offset"] != APP0_OFFSET:
        problems.append("partition table has no app0 partition at 0x10000")
    elif len(app) > app0["size"]:
        problems.append("application is larger than the app0 partition")
    if not lfs:
        problems.append("partition table has no 'littlefs' partition")
    if merged[APP0_OFFSET : APP0_OFFSET + len(app)] != app:
        problems.append("the application inside the merged image differs from the separate application file")
    if problems:
        return result

    # The flash is fully erased before a complete install, so trailing 0xFF
    # bytes do not need to be sent. Trimming keeps the website small.
    trimmed = merged.rstrip(b"\xff")
    trimmed += b"\xff" * ((-len(trimmed)) % 4)

    rel_dir = OUT_DIR / tag
    rel_dir.mkdir(parents=True, exist_ok=True)
    (rel_dir / "app.bin").write_bytes(app)
    (rel_dir / "merged.bin").write_bytes(trimmed)

    example_file = None
    try:
        example = http_get(
            f"https://raw.githubusercontent.com/{REPO}/{tag}/firmware/EXAMPLE_config.txt"
        )
        (rel_dir / "EXAMPLE_config.txt").write_bytes(example)
        example_file = f"firmware/{tag}/EXAMPLE_config.txt"
    except urllib.error.HTTPError as e:
        log(f"  no EXAMPLE_config.txt at {tag} ({e.code})")

    version = tag_version(tag)
    result.update(
        {
            "name": rel.get("name") or tag,
            "version": list(version),
            "published": rel.get("published_at"),
            "prerelease": bool(rel.get("prerelease")),
            "url": rel.get("html_url"),
            "notes": (rel.get("body") or "")[:4000],
            "app": {
                "file": f"firmware/{tag}/app.bin",
                "originalName": app_assets[0]["name"],
                "size": len(app),
                "sha256": hashlib.sha256(app).hexdigest(),
                "md5": hashlib.md5(app).hexdigest(),
            },
            "merged": {
                "file": f"firmware/{tag}/merged.bin",
                "originalName": merged_assets[0]["name"],
                "size": len(trimmed),
                "originalSize": len(merged),
                "sha256": hashlib.sha256(trimmed).hexdigest(),
            },
            "partitions": partitions,
            "littlefs": {"offset": lfs["offset"], "size": lfs["size"]},
            "otadata": {"offset": otadata["offset"], "size": otadata["size"]} if otadata else None,
            "exampleConfig": example_file,
        }
    )
    return result


def main():
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    releases = []
    page = 1
    while True:
        batch = json.loads(
            http_get(f"https://api.github.com/repos/{REPO}/releases?per_page=100&page={page}")
        )
        if not batch:
            break
        releases += batch
        page += 1

    accepted, rejected = [], []
    for rel in releases:
        tag = rel.get("tag_name", "")
        if rel.get("draft"):
            continue
        version = tag_version(tag)
        if not version or version[:2] < MIN_GENERATION:
            continue
        log(f"Release {tag}")
        res = process_release(rel)
        if res["problems"]:
            for p in res["problems"]:
                log(f"  REJECTED: {p}")
            rejected.append({"tag": tag, "problems": res["problems"]})
        else:
            res.pop("problems")
            accepted.append(res)

    accepted.sort(key=lambda r: r["version"], reverse=True)
    manifest = {
        "repo": REPO,
        "generated": datetime.now(timezone.utc).isoformat(timespec="seconds"),
        "flashSize": FLASH_SIZE,
        "chipId": CHIP_ID_ESP32S3,
        "app0Offset": APP0_OFFSET,
        "partitionTableOffset": PARTITION_TABLE_OFFSET,
        "releases": accepted,
        "rejected": rejected,
    }
    (OUT_DIR / "manifest.json").write_text(json.dumps(manifest, indent=2))
    log(f"Wrote {OUT_DIR / 'manifest.json'}: {len(accepted)} release(s), {len(rejected)} rejected")
    if not accepted:
        log("ERROR: no usable firmware release found")
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
