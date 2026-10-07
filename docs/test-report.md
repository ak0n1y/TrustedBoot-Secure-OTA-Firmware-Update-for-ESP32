
# Test report

This file contains tests that I have actually reproduced on hardware.

The scenarios are defined separately in `tests/scenarios.md`.

A scenario stays unvalidated until I have run it on the ESP32 and recorded the result here.

| Scenario                             | Date | Board / IDF | Result     | Notes |
| ------------------------------------ | ---- | ----------- | ---------- | ----- |
| S1 — Normal OTA update               |      |             | Not tested |       |
| S2 — Modified firmware               |      |             | Not tested |       |
| S3 — Invalid signature               |      |             | Not tested |       |
| S4 — Firmware crash on first boot    |      |             | Not tested |       |
| S5 — Self-test failure               |      |             | Not tested |       |
| S6 — Power loss during update        |      |             | Not tested |       |
| S7 — Older firmware offered          |      |             | Not tested |       |
| S8 — Plain HTTP firmware URL         |      |             | Not tested |       |
| S9 — OTA slot modified after install |      |             | Not tested |       |
| S10 — Replaced / unsigned bootloader |      |             | Not tested |       |

For failed tests, I also want to keep the failure itself documented. A failing security test is still useful if it exposes a weakness that needs to be fixed.
