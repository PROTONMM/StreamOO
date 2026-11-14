# 11 — Persistence, Partitions and Security

[English index](00_INDEX.md)

## Storage ownership

| Data | Storage and behavior |
| --- | --- |
| Wi-Fi profiles | Versioned NVS data; new nonempty lists saved after a successful connection test |
| Volume and visualization | Preferences service; deferred write after five seconds without changes |
| Last station | Stable 64-bit ID in system settings; separate deferred write after five seconds |
| List timeout/action, autoplay, hostname | Validated system settings written by the explicit save operation |
| Time zone | Clock settings in NVS, published after successful commit |
| Station catalog | LittleFS, staged/validated replacement followed by snapshot publication |
| Web theme | Browser localStorage, per origin; not device NVS |
| AP password | Private build file and firmware binary; not a runtime web setting |

Persistent writes run outside the sample output path. Rapid UI changes update
RAM and renew the preference timer; they do not synchronously write flash from
the encoder or I²S callback. Power loss within the pending interval may lose
the last change. Wait at least five seconds before disconnecting power.

The system settings blob is version 2. Defaults are a five-second list timeout,
automatic selection, autoplay enabled and a MAC-derived hostname. Version 1 is
read using the existing compatibility rule that enables autoplay; a saved version 2
STOP preference remains respected. Autoplay also requires a matching saved station
ID, usable STA and AP off. A missing ID does not select another station.

## Flash layout and updates

The [partition table](../../partitions.csv) has NVS at `0x9000/0x6000`, PHY at
`0xf000/0x1000`, one factory app at `0x10000/0xEF0000`, and 1 MiB LittleFS at
`0xF00000`. There are no OTA data or alternate app slots. Firmware updates are wired.

With this unchanged layout, normal flashing does not write a blank NVS or filesystem
image. Keep the catalog and settings unless explicitly resetting/migrating them.
Export the playlist before changing historical layouts, because moving the filesystem
is not an automatic data migration. A nonempty damaged filesystem is not silently
formatted as a repair strategy. Flash/power-loss acceptance remains a hardware task.

## Passwords and local access

AP configuration is stored outside the repository by `configure_network.py`, using
hidden terminal input and a private file. It is never entered in Kconfig/defaults.
The generated header belongs under ignored build output. The value is nevertheless
embedded in the binary, so the shared password is not protection against extracting
firmware. Displaying it on the local OLED during AP setup is intentional.

The local HTTP panel has no separate user authentication. A client with network
access can control playback and change settings. The custom request header and
Origin checks guard browser request behavior; they do not identify users or create
an Internet-facing administration service. Keep the device on a trusted network.

Saved Wi-Fi passwords are omitted from API read responses and logs. A profile read
returns only SSID and `has_password`. Public/private station URLs and names may
appear in user-supplied files or diagnostic logs; review them before sharing.
Do not commit private playlists, probe results, credentials or secret-bearing builds.

## TLS and scope of protection

Stream HTTPS verifies certificates using the configured CA bundle and synchronized
UTC, including hostname and date checks. It does not use insecure TLS as a fallback.
The local web control panel is HTTP; encrypted stream transport does not encrypt
local configuration requests. Secure Boot and Flash Encryption are not enabled in
the normal profile. See [network behavior](09_NETWORK.md) and [license scope](LICENSING.md).
