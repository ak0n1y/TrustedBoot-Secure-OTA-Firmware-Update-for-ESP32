#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
HOST="${1:-localhost}"
KEYS="$ROOT/keys"
CERTS="$ROOT/server/certs"
EMBED="$ROOT/firmware/main/certs"
ESPSECURE="${ESPSECURE:-espsecure.py}"

mkdir -p "$KEYS" "$CERTS" "$EMBED"

if [ "${SKIP_SECURE_BOOT_KEY:-0}" != "1" ] && [ ! -f "$KEYS/secure_boot_signing_key.pem" ]; then
    "$ESPSECURE" generate_signing_key --version 2 --scheme rsa3072 "$KEYS/secure_boot_signing_key.pem"
fi

if [ ! -f "$KEYS/ota_signing_key.pem" ]; then
    openssl ecparam -name prime256v1 -genkey -noout -out "$KEYS/ota_signing_key.pem"
fi
openssl ec -in "$KEYS/ota_signing_key.pem" -pubout -out "$EMBED/ota_signing_pub.pem"

if [[ "$HOST" =~ ^[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
    SAN="IP:$HOST"
else
    SAN="DNS:$HOST"
fi

openssl req -x509 -new -nodes -newkey rsa:2048 \
    -keyout "$CERTS/ca.key" -out "$CERTS/ca.crt" -days 825 \
    -subj "/CN=esp32-secure-ota-dev-ca" \
    -addext "basicConstraints=critical,CA:TRUE" \
    -addext "keyUsage=critical,keyCertSign,cRLSign"

openssl req -new -nodes -newkey rsa:2048 \
    -keyout "$CERTS/server.key" -out "$CERTS/server.csr" \
    -subj "/CN=$HOST"

printf 'subjectAltName=%s\nbasicConstraints=CA:FALSE\nkeyUsage=digitalSignature,keyEncipherment\nextendedKeyUsage=serverAuth\n' "$SAN" > "$CERTS/server.ext"

openssl x509 -req -in "$CERTS/server.csr" \
    -CA "$CERTS/ca.crt" -CAkey "$CERTS/ca.key" -CAcreateserial \
    -out "$CERTS/server.crt" -days 825 -sha256 -extfile "$CERTS/server.ext"

cp "$CERTS/ca.crt" "$EMBED/server_ca.pem"
rm -f "$CERTS/server.csr" "$CERTS/server.ext" "$CERTS/ca.srl"

echo "keys in $KEYS"
echo "server certificate for $HOST in $CERTS"
echo "public material for the firmware in $EMBED"
