# Boot chain

One of the main things I wanted to understand with this project was where trust actually starts.

A secure firmware verifier is not very useful if an attacker can simply replace the verifier itself.

On the ESP32 the answer is the ROM bootloader.

```text
ROM
 |
 v
second-stage bootloader
 |
 v
application
```

## Stage 1 — ROM bootloader

The first code executed is stored in ROM and cannot be replaced through normal flash programming.

When Secure Boot is enabled, this stage establishes the hardware root of trust.

## Stage 2 — second-stage bootloader

This bootloader lives in flash, so unlike the ROM it is modifiable.

My custom verification hook also runs at this level.

Its job is not to replace ESP32 Secure Boot. It adds an extra integrity check for OTA slots that were previously installed by the application.

## Stage 3 — application

The application handles:

* Wi-Fi;
* the OTA protocol;
* download verification;
* self-tests;
* final image validation.

## Why Secure Boot matters

Without Secure Boot, an attacker with sufficient access to flash could replace the second-stage bootloader and remove my application-level verification.

With Secure Boot v2 enabled, the ROM verifies the next stage before allowing it to execute.

That closes one of the weaknesses that originally motivated this project.
