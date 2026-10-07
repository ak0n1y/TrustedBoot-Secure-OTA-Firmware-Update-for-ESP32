# Scenarios

| Id | Setup | Expected |
|---|---|---|
| S1 | Flash 1.0.0 over serial, publish 1.0.1 | Device downloads, verifies, reboots on the other slot, validates the image |
| S2 | `tools/tamper_test.sh` after publishing 1.0.1 | SHA-256 mismatch, update rejected, device stays on 1.0.0 |
| S3 | Publish a manifest signed with another key | Signature invalid, update rejected |
| S4 | Build 1.0.1 with `CONFIG_SOTA_DEMO_CRASH_ON_BOOT=y` | Crash while pending, bootloader rolls back to 1.0.0 |
| S5 | Build 1.0.1 with `CONFIG_SOTA_DEMO_FORCE_SELF_TEST_FAILURE=y` | Self-test fails, rollback to 1.0.0 |
| S6 | Cut power during the download | Old slot untouched, device boots 1.0.0, update retried |
| S7 | Publish 0.9.0 | Not newer, no update |
| S8 | Manifest with an `http://` URL | Rejected unless `CONFIG_SOTA_ALLOW_HTTP=y` |
| S9 | Modify bytes in the inactive slot with `esptool.py write_flash` after an update | Seal mismatch at boot, slot invalidated |
| S10 | Secure Boot enabled, flash an unsigned bootloader | ROM refuses to boot |
