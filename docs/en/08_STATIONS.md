# 08 — Station Catalog and Playlist Format

[English index](00_INDEX.md) · [User import instructions](USER_GUIDE.md#importing-a-file)

StreamOO has one persistent station catalog, limited to 2048 entries and a 256 KiB
public import file. The player shows a scrollable full catalog; the web editor
shows 25 editable entries per page. The current editor provides name/URL editing,
add/delete, save, export and clear. It does not provide reorder buttons.

## Public import/export

The yoRadio-compatible file uses UTF-8, one station per line, no header and three
literal TAB-separated fields:

```text
name<TAB>http-or-https-url<TAB>ovol
```

Names have a 128-byte UTF-8 limit; URLs have a 2048-byte limit. `ovol` is a signed
32-bit integer retained for compatibility, not applied as per-station gain.
New editor entries use zero. HTTP and HTTPS may coexist; import does not upgrade,
downgrade or probe stream URLs. Embedded credentials, fragments and invalid/control
characters are rejected under the URL validation policy. Supported normalization
is reported by preview; duplicate URLs are reported and retained.

Import is a two-step web workflow: preview, then explicit commit. Preview validates
the file without changing storage. Commit validates and **appends** to the existing
catalog, subject to total limits and concurrent-write checks. Empty imports are
rejected. A separate clear operation writes an empty catalog; export of that catalog
is an empty file. Importing the same file twice can duplicate its entries.

Export uses the saved immutable snapshot, not unsaved browser edits. It preserves
the three-field public format and does not expose internal IDs. A local
`playlist.csv` in the project root is private, ignored by Git, and never automatically
embedded in a build.

## Identity and storage

Each station has a nonzero 64-bit stable ID. The editor retains IDs when names or
URLs change. New records receive generated IDs with collision checks. The current
LittleFS catalog format begins with `SOOCAT2\n`, followed by:

```text
16-hex-digit-id<TAB>name<TAB>url<TAB>ovol
```

The older three-field stored format remains readable. Public import/export still
has only three fields; it is not a backup of internal identity metadata.
Do not assume reimport after clearing will preserve a saved station ID.

Writes validate a candidate, stage a temporary file, flush/synchronize and rename
before publishing a new snapshot. Generation checks reject stale editor writes or
selections. Readers hold reference-counted snapshots while a new catalog is installed.
These software checks are not a substitute for physical power-loss testing.

## Interaction with playback

Browsing does not interrupt playback. Choosing the same already-playing ID is a
no-op for the stream. Editing its name or URL retains the current connection;
the new URL is used next time. Removing the active ID stops it without choosing a
different station. Last-station persistence uses the ID and a five-second delay.
ICY metadata belongs to the active stream generation rather than the catalog.

## Host tools

```sh
python3 tools/check_playlist.py playlist.csv
python3 tools/probe_playlist.py playlist.csv
```

The first command audits format locally and reports line numbers/error codes without
station data; it does not upload anything. The second uses host `ffprobe` and network
access to inspect sources and create a new active-list/report pair. It does not edit
the source file or the radio. Its outputs can contain private station information.
See [format and playlist validation](FORMAT_VALIDATION.md#checking-a-station-list).
