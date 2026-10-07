
# eFuse warning

This is the part of the project where I am intentionally being conservative.

ESP32 eFuses are one-time programmable. Some security changes therefore cannot simply be undone by reflashing the board.

Before enabling irreversible protections I want the normal OTA, rollback and recovery workflows to be fully tested first.

My current rules are:

* develop without irreversible protections first;
* use Flash Encryption development mode while testing;
* verify the ESP32 revision before enabling Secure Boot v2;
* keep a spare board available;
* back up private signing keys outside the repository;
* inspect every eFuse operation before executing it.

The goal is to learn Secure Boot, not accidentally turn the board into a paperweight.
