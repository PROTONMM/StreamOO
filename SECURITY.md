# Security / Bezpieczeństwo

StreamOO is development firmware for a radio on a **trusted local network**.
The local HTTP panel intentionally has no login (ADR0032). Any client with
network access can control playback and change Wi-Fi, stations and settings.
The shared AP password controls Wi-Fi access, not per-user authorization.
Do not expose the HTTP port to the Internet or an untrusted guest network.

The panel uses HTTP: configuration, including newly submitted Wi-Fi passwords,
is not protected by application-level encryption. Stream HTTPS is separate:
certificate chain, hostname and validity checks remain enabled. CSRF request
headers, Origin checks, no CORS and CSP reduce browser attacks but are not
authentication or a complete defense against DNS rebinding or LAN attackers.
NVS/firmware secrets are not protected from physical flash readout by the
default configuration. Use network isolation and protect firmware artifacts.

## Reporting / Zgłaszanie

Do not put credentials, private playlists, tokens or exploitable unpublished
details in public Issues. Use GitHub's **Security → Report a vulnerability**
when available: [private reporting](https://github.com/PROTONMM/StreamOO/security/advisories/new).
Availability depends on repository settings; the audit did not enable it.
If unavailable, open an Issue asking the maintainer for a private reporting
channel, without vulnerability details. No response SLA is promised.

Provide the commit/tag, ESP-IDF version, affected component, impact and a small
synthetic reproducer. Remove secrets and private station URLs from logs.
Fixes are developed on the current `main`; no older-version maintenance
window is promised. See [audit results](docs/AUDIT_REPORT.md) for limitations.

PL: panel lokalny celowo nie wymaga logowania. Używaj zaufanego LAN; HTTP
nie szyfruje konfiguracji. Podatności zgłaszaj prywatnie, jeśli GitHub
udostępnia tę możliwość; w przeciwnym razie poproś o prywatny kanał przez
Issue bez szczegółów podatności. Nigdy nie publikuj sekretów.
