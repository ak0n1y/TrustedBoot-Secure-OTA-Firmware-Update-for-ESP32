#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
DIR="$ROOT/server/firmware"
MANIFEST="$DIR/manifest.json"

if [ ! -f "$MANIFEST" ]; then
    echo "manifest not found, run sign_firmware.sh first" >&2
    exit 1
fi

cp "$MANIFEST" "$MANIFEST.orig"

python3 - "$DIR" <<'PY'
import json
import sys
from pathlib import Path

directory = Path(sys.argv[1])
manifest = json.loads((directory / "manifest.json").read_text())

base, name = manifest["url"].rsplit("/", 1)
data = bytearray((directory / name).read_bytes())
data[len(data) // 2] ^= 0xFF

tampered = "tampered-" + name
(directory / tampered).write_bytes(bytes(data))

manifest["url"] = base + "/" + tampered
(directory / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
print("tampered image:", tampered)
PY

echo "restore with: mv $MANIFEST.orig $MANIFEST"
