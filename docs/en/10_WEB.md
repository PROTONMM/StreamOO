# 10 — Web Interface and REST API

[English index](00_INDEX.md) · [Operating instructions](USER_GUIDE.md#opening-the-web-interface)

The native ESP-IDF HTTP server serves embedded `/`, `/portal.css` and `/portal.js`.
No CDN or external frontend runtime is required. The UI uses Polish labels and
requires JavaScript. English documentation explains those labels without changing
the firmware interface.

## Interface modes

In AP mode the page exposes Wi-Fi setup, playlist import and time-zone selection.
On the LAN after AP shutdown it exposes Player, Playlist and Settings tabs.
Import moves into the Playlist tab in LAN mode. The player uses a full scrollable
catalog and client-side search; the editor uses 25-row pages. Themes are stored
per browser origin under `streamoo.theme`, independently of device settings.

Previous/next station uses the full catalog and wraps even while search is active.
The volume slider sends its first change immediately, spaces subsequent requests
by at least 100 ms and keeps at most one request in flight with the latest pending
value. Releasing the slider flushes the final value. Polling does not overwrite the
value during editing. Offline/error handling does not endlessly replay old commands.

Status is refreshed approximately every two seconds. A 202 response means a command
was accepted, not that a new stream is already playing. The UI should follow status
for buffering and actual codec information.

## Common API conventions

Base path: `/api/v1`. This is StreamOO's API, not yoRadio's API. The implementation
and tests are the authoritative contract for the current version.

- POST operations require `X-StreamOO-Request: 1`.
- If `Origin` is present, it must match `http://` plus the request's `Host`.
  A non-browser client may omit Origin; the request header still applies.
- POST bodies require the specified Content-Type and must fit endpoint limits.
- Saved passwords are not exposed by read endpoints.
- Status uses `schema_version: 1`. Station IDs are lowercase, fixed-width
  16-digit hexadecimal strings. Catalog generations are decimal strings,
  avoiding JavaScript precision loss for 64-bit values.
- JSON errors contain `error`; playlist validation also provides `error_line`
  and counts. Check HTTP status before treating the body as success.
- Origin and custom-header checks are not authentication. The panel is local
  HTTP without a user login; see [security](11_STORAGE_SECURITY.md).

## Routes

| Method and route | Purpose / request |
| --- | --- |
| `GET /api/v1/status` | Network/audio status and uptime |
| `GET /api/v1/wifi/profiles` | Ordered profiles with `ssid` and `has_password`, never passwords |
| `POST /api/v1/wifi/config` | JSON `profiles` array; test and save a candidate list asynchronously |
| `POST /api/v1/stations/import/preview` | Raw three-field playlist, `text/plain`; validate without saving |
| `POST /api/v1/stations/import` | Raw three-field playlist, `text/plain`; append and save |
| `GET /api/v1/stations/import/status` | Current saved playlist counts/status |
| `GET /api/v1/clock` | Current `timezone` and supported `zones` |
| `POST /api/v1/clock` | JSON containing exactly `timezone` |
| `GET /api/v1/settings` | List timeout/action, autoplay and hostname |
| `POST /api/v1/settings` | JSON containing all four system settings |
| `POST /api/v1/player/control` | Explicit play, stop or absolute volume command |
| `GET /api/v1/stations` | Entire catalog: `generation` and `stations` array |
| `GET /api/v1/stations/export` | Saved three-field `playlist.csv` download |
| `POST /api/v1/stations/replace` | Replace/clear catalog with generation check; text or editor TSV |
| `POST /api/v1/stations/select` | Select ID with generation check |

Catalog read/export/replace/select require player mode: STA connected and AP off.
Player control must also have an available network-stream owner. Setup routes
remain usable in AP mode. Do not use a hidden frontend control as an API permission
model; handlers independently validate requests and mode where required.

## Request bodies

Player commands, with `Content-Type: application/json`:

```json
{"action":"play"}
```

```json
{"action":"stop"}
```

```json
{"action":"volume","value":50}
```

Volume must be an integer from 0 to 100. Play/stop have no extra fields. The limit
is 128 bytes. Previous/next are client operations that choose an ID from the catalog,
not additional player action names.

System settings, all four fields required, limit 512 bytes:

```json
{"list_timeout_s":5,"list_autoselect":true,"autoplay":true,"hostname":""}
```

Timeout is an integer 1–120; the two flags are JSON booleans. Hostname follows the
[network rules](09_NETWORK.md). Time-zone update, limit 128 bytes:

```json
{"timezone":"Europe/Warsaw"}
```

Wi-Fi input, limit 4096 bytes, uses `{"profiles":[...]}` with up to eight entries.
Each entry has `ssid` and, when being set, `password`. An omitted password can retain
the saved value for that SSID. An explicit empty password means an open network.
An empty array explicitly removes saved profiles. SSIDs are limited to 32 bytes
and nonempty passwords to 8–63 bytes; duplicate SSIDs are rejected. A 202 response
starts a connection test; inspect status for its outcome before claiming persistence.

## Catalog consistency

`GET /api/v1/stations` returns each station as `id`, `name`, `url` and `ovol`.
Before select or replace, read the catalog and send its exact generation in:

```text
X-StreamOO-Catalog-Generation: <decimal generation from GET>
```

Selection uses JSON with exactly `station_id`, containing an existing nonzero
16-digit lowercase hexadecimal ID; its body limit is 64 bytes.

Replace accepts either `text/plain` for the public three-field format or
`application/vnd.streamoo.stations+tsv` for editor rows:

```text
16-hex-digit-id<TAB>name<TAB>url<TAB>ovol
```

Editor rows use zero ID (`0000000000000000`) for newly added entries. Existing IDs
preserve identity across name/URL edits. The editor format allows the additional
17 bytes per row beyond the 256 KiB public-file allowance. An empty replace body
can clear the catalog; it still requires the headers and current generation.
This is separate from import, which rejects empty input and appends stations.

Import preview does not create a server-side confirmation token. The frontend sends
the chosen file again on commit. Import's storage operation checks concurrent state
and total limits when combining the file with the current catalog. Do not assume
preview reserves capacity or locks the catalog.

## Status data and errors

`network` includes mode (`ap`, `apsta`, `sta`), readiness, AP state, STA connection,
configuration progress, IP, saved profile count and error. Its `ssid` field is the
setup AP name, not a password or the home-network password.

`audio` includes availability, source, requested running state, buffering, volume,
station ID, codec, bitrate, sample rate, and the `bitrate_measured` and
`bitrate_variable` flags. Codec data should be treated as unknown until valid for
the current session. The status serializer's upper limits are defensive bounds,
not promises of every sample rate/bitrate being playable.

Typical responses are 400 for invalid values, 403 for request proof/Origin errors,
409 for stale catalogs or mode/state conflicts, 413 for oversized bodies, 415 for
wrong Content-Type, and 503 for unavailable resources. A 409 must trigger a fresh
read and deliberate reconciliation, not an unconditional overwrite.

Read-only inspection from a computer on the same LAN:

```sh
curl http://RADIO_IP_ADDRESS/api/v1/status
curl http://RADIO_IP_ADDRESS/api/v1/stations
```

Replace the address placeholder. Integration details live in
[portal handlers](../../components/portal/streamoo_portal_config.c),
[status serialization](../../components/portal/streamoo_portal_status.c)
and [frontend requests](../../components/portal/web/portal.js).
