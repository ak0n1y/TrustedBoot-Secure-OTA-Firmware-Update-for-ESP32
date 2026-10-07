# TrustedBoot — Secure OTA Firmware Updates for ESP32

TrustedBoot is a personal embedded-security project built around one question:

**How can an ESP32 update its firmware remotely without blindly trusting what it receives?**

I started this project to better understand what actually happens between powering on a microcontroller and running a trusted application.

The project gradually combines OTA updates, A/B partitions, firmware signatures, rollback mechanisms and the ESP32 hardware root of trust.

It is mainly a learning and portfolio project, not a production-ready OTA platform.

---

## What I wanted to build

The initial goal was simple:

```text
ESP32 running firmware A
        |
        | Wi-Fi
        v
Download firmware B
        |
        v
Install it safely
        |
        v
Reboot on firmware B
```

Then I started adding the questions that make OTA updates interesting from a security point of view:

* What if the firmware is modified during download?
* What if someone replaces the firmware on the server?
* What if the new firmware is correctly signed but crashes?
* What if someone flashes an older vulnerable version?
* What if an attacker physically replaces the bootloader?
* What happens if power is lost during an update?

TrustedBoot is my attempt to explore these problems one layer at a time.

---

## Current architecture

```text
                    OTA Server
               manifest + firmware
                        |
                      HTTPS
                        |
                        v
                  ESP32 application
                        |
          SHA-256 + ECDSA verification
                        |
                        v
                inactive OTA slot
                        |
                        v
                     reboot
                        |
                        v
               second-stage bootloader
                        |
                 integrity checks
                        |
                        v
                 new application
                        |
                    self-test
                  /           \
               success       failure
                  |             |
               validate       rollback
```

The flash contains two application slots:

```text
ota_0  <---->  ota_1
```

One runs the current firmware while the other can receive the next update.

The active firmware is therefore not overwritten while an OTA update is being downloaded.

---

## Security layers

I deliberately separated the different security mechanisms instead of treating “secure OTA” as one feature.

### 1. HTTPS

The ESP32 retrieves the manifest and firmware over HTTPS using a development CA embedded in the application.

Plain HTTP firmware URLs are rejected by default.

### 2. SHA-256

The firmware is hashed while it is downloaded.

The result must match the hash announced in the manifest before the image can be activated.

### 3. OTA signature

Firmware releases are signed with an **ECDSA P-256 private key**.

Only the public key is embedded in the ESP32 firmware.

This means that changing the firmware and recalculating its SHA-256 hash is not enough: a valid signature is also required.

### 4. A/B rollback

A new image is not immediately trusted.

After the first boot, it enters a pending state and runs a self-test.

If the self-test fails or the firmware crashes before being validated, the ESP32 can return to the previous application.

### 5. Boot-time seal

I also added a small integrity mechanism to the second-stage bootloader.

After an OTA image is installed, its SHA-256 digest is stored in a dedicated `fwmeta` partition.

At boot, the custom bootloader hook checks the slot again.

This is mainly a defense-in-depth mechanism and does **not** replace Secure Boot.

### 6. ESP32 Secure Boot

The final trust anchor is not my application or my custom bootloader.

It is the ESP32 ROM bootloader.

With Secure Boot v2 enabled:

```text
ESP32 ROM
    |
    | verifies
    v
second-stage bootloader
    |
    | verifies
    v
application
```

The Secure Boot public-key digest is stored in eFuses, creating a hardware-backed chain of trust.

---

## Flash layout

The project currently targets a 4 MB ESP32 flash:

| Partition |     Offset |       Size | Purpose                     |
| --------- | ---------: | ---------: | --------------------------- |
| NVS       |  `0x11000` |   `0x4000` | Non-volatile storage        |
| OTA data  |  `0x15000` |   `0x2000` | Active OTA slot information |
| PHY init  |  `0x17000` |   `0x1000` | PHY configuration           |
| `fwmeta`  |  `0x18000` |   `0x1000` | Slot integrity metadata     |
| `ota_0`   |  `0x20000` | `0x1D0000` | Firmware slot A             |
| `ota_1`   | `0x1F0000` | `0x1D0000` | Firmware slot B             |

More details are available in [`docs/architecture.md`](docs/architecture.md).

---

## Repository structure

```text
.
├── firmware/
│   ├── bootloader_components/
│   │   └── custom_verify/
│   ├── main/
│   ├── partitions.csv
│   ├── sdkconfig.defaults
│   └── sdkconfig.secure
│
├── server/
│   ├── certs/
│   ├── firmware/
│   └── ota_server.py
│
├── tools/
│   ├── gen_keys.sh
│   ├── make_manifest.py
│   ├── sign_firmware.sh
│   └── tamper_test.sh
│
├── tests/
│   └── scenarios.md
│
└── docs/
    ├── architecture.md
    ├── boot-chain.md
    ├── efuse-warning.md
    ├── test-report.md
    └── threat-model.md
```

---

## Building the project

The project currently uses:

* ESP32
* ESP-IDF 6.0.3
* Python 3
* OpenSSL
* Wi-Fi

Generate the development keys and certificates:

```bash
./tools/gen_keys.sh <OTA_SERVER_IP>
```

Then configure the firmware:

```bash
cd firmware
idf.py menuconfig
```

The `Secure OTA` menu contains the Wi-Fi configuration, manifest URL, OTA interval and demonstration options.

Build with:

```bash
idf.py build
```

and flash the initial firmware with:

```bash
idf.py -p <PORT> flash monitor
```

---

## Publishing an OTA image

After building a new firmware version:

```bash
./tools/sign_firmware.sh \
    firmware/build/esp32-secure-ota.bin \
    1.0.1 \
    https://<OTA_SERVER_IP>:8443
```

The tool generates the firmware, its signature and `manifest.json` inside:

```text
server/firmware/
```

Then start the development OTA server:

```bash
python3 server/ota_server.py
```

The ESP32 periodically checks the manifest and installs the update only if its checks succeed.

---

## Attack / failure scenarios

I am using the project to test both normal updates and failure cases.

The planned scenarios include:

* normal OTA update;
* modified firmware;
* invalid signature;
* firmware crash after update;
* self-test failure;
* power loss during download;
* downgrade attempt;
* HTTP instead of HTTPS;
* modification of an installed OTA slot;
* replaced or unsigned bootloader.

The scenarios themselves are documented in:

[`tests/scenarios.md`](tests/scenarios.md)

and actual hardware results are recorded in:

[`docs/test-report.md`](docs/test-report.md)

I only mark a scenario as validated once I have reproduced it on the board.

---

## Current status

The main OTA and security mechanisms are implemented.

I am currently working through the hardware validation of each attack and recovery scenario.

One remaining hardening point is to make sure the version announced by the OTA manifest is also bound to the version stored inside the ESP32 application image.

Flash Encryption is also intentionally kept in **development mode** while the workflow is still being tested.

---

## A note about eFuses

Some ESP32 security settings are irreversible.

Secure Boot, Flash Encryption and anti-rollback can modify eFuses permanently.

For that reason, I am keeping the secure configuration separate from the normal development configuration and testing the complete OTA flow before enabling irreversible protections.

See [`docs/efuse-warning.md`](docs/efuse-warning.md) before experimenting with those features.

---

## Why this project?

The main reason I built TrustedBoot was not simply to use ESP-IDF's OTA API.

I wanted to understand how the different pieces fit together:

```text
OTA
+
cryptography
+
flash partitions
+
bootloader
+
rollback
+
eFuses
+
hardware root of trust
```

That interaction between low-level embedded development and cybersecurity is the part of the project I find the most interesting.

---

## License

MIT License.

Copyright © 2026 Laye.
