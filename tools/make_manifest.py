import argparse
import base64
import hashlib
import json
from pathlib import Path


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--firmware", required=True)
    parser.add_argument("--signature", required=True)
    parser.add_argument("--version", required=True)
    parser.add_argument("--base-url", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()

    firmware = Path(args.firmware)
    data = firmware.read_bytes()
    signature = Path(args.signature).read_bytes()

    manifest = {
        "version": args.version,
        "url": f"{args.base_url.rstrip('/')}/{firmware.name}",
        "size": len(data),
        "sha256": hashlib.sha256(data).hexdigest(),
        "signature": base64.b64encode(signature).decode("ascii"),
    }

    Path(args.output).write_text(json.dumps(manifest, indent=2) + "\n")
    print(json.dumps(manifest, indent=2))


if __name__ == "__main__":
    main()
