# Threat model

This is not meant to be a complete threat model for a commercial IoT product.

I use it mainly to keep track of the attacks I want the project to resist and to make sure each security feature has a clear reason to exist.

| Threat                                      | Current mitigation                  |
| ------------------------------------------- | ----------------------------------- |
| Firmware modified during distribution       | SHA-256 + ECDSA                     |
| Malicious firmware placed on the OTA server | ECDSA signature                     |
| Network interception                        | HTTPS with a trusted development CA |
| Bootloader replacement                      | Secure Boot v2                      |
| Direct firmware modification                | Secure Boot v2                      |
| OTA slot modified after installation        | Boot-time integrity seal            |
| Firmware extraction from flash              | Flash Encryption                    |
| Downgrade to an older release               | Version check + ESP32 anti-rollback |
| Correctly signed but broken firmware        | Self-test + rollback                |
| Power loss during OTA                       | A/B partition design                |
| Signing key accidentally committed          | Keys excluded from the repository   |

## What I am not trying to solve yet

The following attacks are currently outside the scope of the project:

* voltage or clock glitching;
* advanced fault injection;
* side-channel attacks;
* invasive physical attacks;
* extraction attacks against the silicon itself.

Those would require a very different test setup.

