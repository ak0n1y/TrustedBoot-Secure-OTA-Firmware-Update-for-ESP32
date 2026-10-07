# Architecture

This document describes how I currently structured TrustedBoot.

The goal was to keep the OTA mechanism understandable: the application handles the update itself, while the bootloader is responsible for deciding what is allowed to run.

I also wanted the different security mechanisms to remain visible instead of hiding everything behind a single OTA helper function.

## Main components

| Component            | Location                                       | Role                                                                 |
| -------------------- | ---------------------------------------------- | -------------------------------------------------------------------- |
| Application          | `firmware/main`                                | Wi-Fi connection, OTA client, firmware verification and self-test    |
| Bootloader extension | `firmware/bootloader_components/custom_verify` | Checks the integrity seal associated with OTA slots                  |
| OTA server           | `server/ota_server.py`                         | Small HTTPS server used during development                           |
| Tools                | `tools/`                                       | Key generation, firmware signing, manifest creation and tamper tests |

## Flash layout

| Name            | Type |     Offset |           Size |
| --------------- | ---- | ---------: | -------------: |
| bootloader      |      |   `0x1000` | up to `0xF000` |
| partition table |      |  `0x10000` |       `0x1000` |
| nvs             | data |  `0x11000` |       `0x4000` |
| otadata         | data |  `0x15000` |       `0x2000` |
| phy_init        | data |  `0x17000` |       `0x1000` |
| fwmeta          | data |  `0x18000` |       `0x1000` |
| ota_0           | app  |  `0x20000` |     `0x1D0000` |
| ota_1           | app  | `0x1F0000` |     `0x1D0000` |

The two OTA partitions allow me to download a new firmware without overwriting the currently working one.

## Why two different signatures?

There are two different trust problems in this project.

The **OTA ECDSA signature** answers:

> Did this firmware release come from someone holding my OTA signing key?

Secure Boot answers a different question:

> Is the code that the ESP32 is about to execute part of my trusted boot chain?

Keeping the two separate made the security model much easier for me to reason about.

## OTA flow

The current update flow is:

1. Retrieve `manifest.json` over HTTPS.
2. Check whether the offered version is newer.
3. Select the inactive OTA partition.
4. Download the firmware while computing SHA-256.
5. Compare the resulting digest with the manifest.
6. Verify the ECDSA signature.
7. Let ESP-IDF validate the application image.
8. Store the slot integrity seal.
9. Select the new partition and reboot.
10. Run a self-test on the new image.
11. Validate it or rollback.