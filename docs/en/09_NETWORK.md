# 09 — Wi-Fi, Discovery, Time and HTTPS

[English index](00_INDEX.md)

## STA and setup AP

Up to eight ordered SSID/password profiles are stored in NVS. At boot, the radio
tries STA first in list order, allowing about 15 seconds per profile for connection
and an IP address. It does not select only the strongest RSSI. No profiles or failure
of all profiles starts the WPA2-Personal setup AP, `StreamOO-XXXXXX`, with DHCP and
the local page at `http://192.168.4.1/`.

The shared AP password is injected from a private external file during the build.
Without that file, networking remains disabled rather than starting an open AP.
The password is intentionally displayed on the local OLED in setup mode, but not
returned in status/profile responses or written to UART logs.

A candidate Wi-Fi list is tried before replacing saved profiles. On a failed change,
the previous list is retained and tried before falling back to AP. An explicitly
saved empty list removes profiles and keeps setup mode. Once an already-active AP
has a successful STA/IP connection, it remains for about five seconds to allow the
web response to finish, then turns off. Audio is disabled while AP setup is active.

The web UI accepts SSIDs manually; it does not present a nearby-network scanner.
Open networks require an explicit checkbox. For an existing SSID, an omitted
password retains the old password; an explicitly empty password selects an open
network. Saved passwords are never read back by the browser.

## Local discovery

Default hostnames are `streamoo-xxxxxx.local`, based on the MAC suffix. HTTP service
advertisement uses mDNS/DNS-SD on the LAN after STA connects and the AP is off.
Discovery failure does not remove direct IP access. The SDK handles name conflicts;
the actual advertised name may have a suffix.

The custom hostname accepts 1–63 lowercase letters, digits and hyphens, without
leading/trailing hyphens. An empty setting restores the MAC-derived name. Updating
it restarts discovery, not Wi-Fi or audio. Multicast may be blocked by a router,
client isolation or phone network settings; use the IP address in that case.

## UTC and local time

SNTP starts after STA obtains an IP, without the default random startup delay.
Configured fallback servers are `0.pool.ntp.org`, `1.pool.ntp.org` and
`2.pool.ntp.org`. The firmware accepts a plausible UTC range of 2020–2099.
A temporary Wi-Fi loss does not immediately invalidate a previously synchronized
clock; a reboot or invalid correction resets validity.

The time-zone setting affects local display only, not TLS UTC. Available zones:
UTC, Europe/Warsaw, Europe/London, Europe/Berlin, Europe/Paris, Europe/Helsinki,
Europe/Madrid and Europe/Rome. Poland is the default. The implementation applies
the encoded European daylight-saving rules; it is not a downloadable full IANA
time-zone database. Date presentation is day–month–year.

## Stream transport and TLS

HTTP can start without a valid clock. HTTPS waits for valid UTC, including URLs
reached through redirects, playlists and HLS. The wait is bounded and cancellable;
failure leads to a reported error/retry rather than disabling certificate validation.
Certificate signatures, hostnames and validity dates are checked with the SDK's CA
bundle; cross-signed verification can find a path to an existing trusted root.

The reader retains complete URLs, including query parameters needed for HLS session
reloads and signed URLs. It does not automatically downgrade HTTPS to HTTP. Input
and parsing limits apply to redirects, nested playlists and segment manifests.

The normal profile disables Wi-Fi modem sleep (`WIFI_PS_NONE`) to reduce delivery
latency, at the cost of higher Wi-Fi power use. TCP receive window/mailbox defaults
are 23040 bytes/18. These are recorded configuration choices, not a guarantee that
every Internet source will play without interruption.

## Hardware cryptography

The default profile enables `CONFIG_MBEDTLS_HARDWARE_AES` and
`CONFIG_MBEDTLS_HARDWARE_SHA`. The ESP-IDF cryptography stack uses ESP32-S3
hardware AES for supported encryption/decryption operations and hardware SHA
for supported hashes. Actual use depends on the TLS cipher suite and operation;
the rest of the protocol still runs on the CPU.

`CONFIG_STREAMOO_CRYPTO_EARLY_INIT` runs a known-answer AES/SHA preparation step
before I²S starts, initializing the shared crypto DMA resources early. This
configuration does not add support for encrypted HLS segments or an HTTPS server
for the local web panel.

## Access boundary

The web panel is HTTP on the local network, without separate authentication.
Origin/request-header checks reduce unintended browser requests but are not user
authentication. Keep it on a trusted LAN; do not expose it through Internet port
forwarding. See [web/API behavior](10_WEB.md) and [security/storage](11_STORAGE_SECURITY.md).
