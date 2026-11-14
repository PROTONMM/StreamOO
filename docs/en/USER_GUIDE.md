# StreamOO — User Guide

[English index](00_INDEX.md) · [Polski](../USER_GUIDE.md)

Updated: 2026-11-14. This guide covers the normal radio profile with networking,
display and encoder enabled. For building and flashing, see [BUILD_GUIDE.md](BUILD_GUIDE.md).
Test evidence and outstanding work are described in [STATUS.md](STATUS.md).
The firmware's web interface currently uses Polish labels; this guide gives
their English meaning alongside the labels you will see. It does not change
the interface language.

## First startup

1. Switch on the radio. The StreamOO logo appears at startup and may remain
   visible while the radio connects to Wi-Fi and starts playback.
2. With no saved networks, the radio starts the **StreamOO-XXXXXX** access
   point. The OLED shows its SSID, password and setup URL. The password comes
   from the private build configuration; there is no public default password.
3. Connect your computer or phone to this network. On Android, accept staying
   connected without Internet access. If the phone switches connections,
   temporarily disable mobile data during setup.
4. Open **http://192.168.4.1/** manually. Automatic captive-portal opening is
   neither required nor guaranteed. The page runs locally and needs JavaScript.
5. Under **Sieci Wi-Fi** (Wi-Fi networks), add your home 2.4 GHz network name
   and password, then select **Zapisz i połącz** (Save and connect).
   For an open network, select **Sieć otwarta, bez hasła** (Open network,
   no password).
6. Once connected, the radio saves the profiles and turns off its AP after
   about five seconds. Reconnect your phone/computer to the same home network.
   Open the IP address shown on the OLED, or the link displayed by the portal
   before the AP disconnected.
7. Add a station under **Playlista** (Playlist), or use **Playlista → Import
   playlisty** (Import playlist). Click a station in **Odtwarzacz** (Player).

Audio is disabled during AP setup. You can configure Wi-Fi, import stations
and select a time zone there. The player and editor become available after
joining the home network. You can import stations before step 5.

On later boots, the radio first tries saved networks in priority order,
allowing about 15 seconds per profile. It opens the AP only after all attempts
fail. By default, it plays the last saved station if that station still exists.
Startup behavior can be changed in settings.

## Opening the web interface

On your home network, use `http://RADIO_IP_ADDRESS/`. Alternatively, use
`http://streamoo-xxxxxx.local/` or the custom hostname configured in settings.
If mDNS does not work on your phone or router, use the IP address.
A previous address may stop working after a network or DHCP address change.

The local panel does not require a separate login. Anyone with network access
to it can control the radio and change its settings. Do not expose it to the
Internet through port forwarding. After updating firmware, bypass the browser
cache when reloading if you see an old layout or controls that do not work.

### Player

- Click a station name to play it.
- The scrollable window contains all saved stations, without pagination.
  **Szukaj stacji…** (Search stations) filters names; **↻** reloads the catalog.
- The icon buttons, in order, are **previous station, play, stop, next station**.
  Previous/next uses the entire catalog regardless of the search filter and
  wraps between the first and last stations.
- The volume slider takes effect while you drag it. Its range is 0–100:
  0 mutes the output and 100 applies no digital attenuation.
- STOP closes the stream and turns off the OLED. PLAY reconnects to the selected
  station. This is not a pause that retains your position in a broadcast.
  The web interface remains available.
- Codec, bitrate and sample rate are shown. **≈** marks a bitrate estimate based
  on decoded audio; **Zmienny** means detected variable bitrate; **—** means
  unavailable information. Detecting variation takes several seconds of
  observation. A number without the variable label does not prove CBR.

Choose **Systemowy / Jasny / Ciemny** (System / Light / Dark) in the header.
The choice is stored in that browser for that page origin. It does not change
the OLED appearance.

### Playlist editing and backup

1. Under **Playlista**, expand **Dodaj stację** (Add station), enter a name and
   an `http://` or `https://` stream URL, then click **Dodaj do listy**
   (Add to list).
2. Edit existing names and URLs in their fields. **×** removes a row from the
   working copy. The editor displays 25 stations per page.
3. Click **Zapisz zmiany** (Save changes) to store the entire edited catalog
   in the radio. **Odrzuć zmiany i odczytaj** (Discard changes and reload)
   restores the saved version.
4. **Eksportuj playlist.csv** (Export playlist.csv) downloads the saved catalog,
   excluding unsaved editor changes. Use export to make a backup.
5. **Usuń całą listę** (Delete the entire list) requires confirmation and deletes
   the saved catalog. Export it first. Deleting the active station stops playback.

Changing a station name or URL retains its identifier. Editing the active
station does not restart playback; its new URL takes effect on the next
connection. Selecting the same station while it is already playing also does
not restart the stream. If another browser has changed the catalog, reload the
current version and reapply your changes. Do not assume a rejected save succeeded.

### Importing a file

On the home network, open **Playlista → Import playlisty**. In AP mode, the
import section is on the setup page. Choose a file, click **Sprawdź playlistę**
(Check playlist), read the result, then click **Zatwierdź import** (Confirm import).
Import **appends** stations. Duplicate URLs are reported and retained; importing
the same file again can add duplicate entries. The limits are 2048 stations in
the resulting catalog and 256 KiB per file.

To replace a catalog, export a backup, check the replacement file, delete the
catalog with the separate button, then check and confirm the import again.
Checking a file does not save it. An empty import is not a delete operation.

Despite the `.csv` extension, the format is:

```text
station name<TAB>HTTP or HTTPS URL<TAB>ovol
```

Use UTF-8 without a header and exactly three fields separated by **literal TAB
characters**, not commas, semicolons or the text `<TAB>`. Each station occupies
one line. Names may contain up to 128 UTF-8 bytes; URLs may contain up to 2048
bytes. `ovol` is an integer retained for yoRadio compatibility. Use `0` for new
stations. StreamOO does not currently use it for per-station volume adjustment.
Exporting a catalog from the radio is the easiest way to obtain a valid example.

A file may mix HTTP and HTTPS. Import does not change the protocol or verify
that every station is broadcasting. The URL should point to a stream or a
supported playlist, rather than a station's website. A local `playlist.csv`
in the project root is not automatically embedded or flashed with the firmware.

## Rotary encoder

Default **Mode A**:

| Gesture | Home screen | Station list |
| --- | --- | --- |
| Turn | Adjust volume | Move the selection |
| Short press | While playing: open the list; after STOP: PLAY | Select the highlighted center station |
| Long press | STOP and turn off the OLED | STOP, leave the list and turn off the OLED |
| Turn while pressed | Change visualization | No action |

If no station is selected, a short press opens the list. A long press means
holding the button for about 0.6 seconds or longer; the action occurs on release.
After turning while pressed, releasing the button does not trigger another click.

The OLED list stops at its first and last entries. The current station continues
playing while you browse. By default, **five seconds of inactivity** selects
the highlighted station. Web settings allow 1–120 seconds and a return without
changing station instead of automatic selection.

Optional **Mode B**, selected at build time in `menuconfig`, changes the home
screen gestures: a short press toggles PLAY/STOP and a long press opens the list.
In the list, a short press confirms and a long press leaves without changing station.

## OLED and visualizations

The screen displays the station name, scrolling broadcast metadata, IP address,
volume, audio/Wi-Fi information, date and clock. Metadata comes from the broadcaster;
a missing title does not necessarily indicate a playback problem.

Turn the encoder while holding its button to cycle through four views:

| View | Meaning |
| --- | --- |
| Spectrum | Maximum FFT-bin level within each frequency band |
| L/R bargraphs | Current channel RMS levels with separate peak markers |
| L/R needles | RMS level indicated by the needle and peak by a two-pixel marker |
| Spectrum with peak hold | The same spectrum, with held, slowly falling peak markers |

Both spectrum views use **−45…0 dBFS**, the same nonlinear height mapping
(gamma 2), and a pixel checkerboard fill. The fourth view is not an RMS spectrum.
Audio measurements are taken before volume control, so muting the output does
not reduce the readings while the stream continues playing.

## Settings and persistence

| Web section | Settings |
| --- | --- |
| Sieci Wi-Fi (Wi-Fi networks) | Up to eight profiles, priority order using ↑/↓, passwords and open networks |
| Data i czas (Date and time) | Time zone; Poland by default, with automatic daylight-saving changes |
| Odtwarzanie i obsługa (Playback and controls) | OLED list timeout/action, start stopped or play the last station, `.local` hostname |

Saved Wi-Fi passwords are not displayed. Leaving an existing profile's password
blank keeps its previous value; use the checkbox for an open network. A new
profile list replaces the old one only after a successful connection. If the
new profiles fail, the radio tries the previous ones. Explicitly saving an empty
list removes profiles and leaves the radio in AP setup mode.

Enter a hostname without `http://` or `.local`: up to 63 lowercase letters,
digits and hyphens, with no leading or trailing hyphen. An empty field restores
the MAC-derived name.

Volume, visualization and the last station are saved after about five seconds
without changes. Wait that long before disconnecting power. Web forms require
their own save button and a success message. A normal firmware update preserves
data when the partition layout is unchanged.

## Formats and limitations

The implementation includes MP3, AAC/HE-AAC, native FLAC, Ogg Vorbis/Opus and
M3U/PLS URL resolution. Output supports mono/stereo at 8–48 kHz. Supporting a
format does not guarantee every profile or station. Multichannel audio,
Ogg-FLAC and raw Opus/RTP are not declared supported.

HLS covers unencrypted MP3/AAC audio and MPEG-TS within the supported subset.
AES/DRM, fMP4, BYTERANGE and LL-HLS are unsupported. A discontinuity after startup
ends the session and causes a retry. See the [format validation guide](FORMAT_VALIDATION.md)
for procedures and links to the original hardware reports.

HTTPS requires valid UTC obtained through SNTP. The radio verifies certificates
and does not automatically fall back from HTTPS to HTTP. The normal profile has
no OTA, timeshift, RC5 remote support or microSD playback. Long-running tests and
some hardware scenarios remain OPEN in the project status.

## Troubleshooting

| Symptom | Check |
| --- | --- |
| Phone leaves the radio AP | Stay connected without Internet; temporarily disable automatic switching to mobile data |
| Web panel will not open | Same network, current IP from OLED/router, `http://`, no client isolation; use IP if `.local` fails |
| Neither AP nor Wi-Fi connection starts | Check the build's private AP configuration and `STREAMOO_NETWORK_ENABLED`; see the build guide |
| OLED is dark but the web panel works | The radio may be stopped; use PLAY or a short encoder press in Mode A |
| Stations are listed but there is no sound | Select a station, check volume and playback state; audio is disabled in AP mode; compare a known working station |
| HTTPS takes a long time or the clock is missing | Check Internet, DNS and SNTP access; changing the time zone does not synchronize UTC |
| Dropouts or repeated reconnects | Check signal strength, source availability and supported formats; collect diagnostic logs as described in the build guide |
| Duplicate stations after import | Import appends and retains duplicates; delete unwanted entries in the editor and save |
| Playlist format error | UTF-8, three TAB-separated fields, integer third field, file and catalog limits |
| Last change lost after power-off | Wait at least five seconds after changing volume, view or station; check form save messages |

Do not start troubleshooting by erasing the entire flash. Exporting the playlist
and recording settings you want to keep makes recovery easier.
