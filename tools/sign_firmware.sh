#!/usr/bin/env bash
set -euo pipefail

if [ "$#" -lt 3 ]; then
    echo "usage: $0 <firmware.bin> <version> <base-url>" >&2
    exit 1
fi

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SOURCE="$1"
VERSION="$2"
BASE_URL="${3%/}"
OUT="$ROOT/server/firmware"
OTA_KEY="${OTA_SIGNING_KEY_FILE:-$ROOT/keys/ota_signing_key.pem}"
SECURE_BOOT_KEY="${SECURE_BOOT_KEY_FILE:-$ROOT/keys/secure_boot_signing_key.pem}"
ESPSECURE="${ESPSECURE:-espsecure.py}"
NAME="firmware-$VERSION.bin"

mkdir -p "$OUT"

if [ "${SECURE_BOOT_SIGN:-0}" = "1" ]; then
    "$ESPSECURE" sign_data --version 2 --keyfile "$SECURE_BOOT_KEY" --output "$OUT/$NAME" "$SOURCE"
else
    cp "$SOURCE" "$OUT/$NAME"
fi

openssl dgst -sha256 -sign "$OTA_KEY" -out "$OUT/$NAME.sig" "$OUT/$NAME"

python3 "$ROOT/tools/make_manifest.py" \
    --firmware "$OUT/$NAME" \
    --signature "$OUT/$NAME.sig" \
    --version "$VERSION" \
    --base-url "$BASE_URL" \
    --output "$OUT/manifest.json"
