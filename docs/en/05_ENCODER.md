# 05 — Encoder and Station Selection

[English index](00_INDEX.md) · [User controls](USER_GUIDE.md#rotary-encoder)

The default wiring is A=GPIO5, B=GPIO6 and button=GPIO7. GPIO edge capture uses
interrupts and a bounded queue. The pure input decoder converts transitions into
rotation, short press, long press and rotation-while-pressed events. Queue overflow
resets decoding and can be diagnosed; maximum reliable turning speed still requires
hardware characterization.

Defaults are half-quadrature detents, reversed direction and internal pull-ups
disabled for the user's hardware. Configure them under **Hardware: Encoder**.
The button uses 20 ms debounce and a 600 ms long-press threshold. Press actions
occur on release. Rotation while pressed suppresses the release click.

| Gesture | Mode A, home | Mode B, home |
| --- | --- | --- |
| Turn | Adjust volume | Adjust volume |
| Short press | PLAY after STOP; otherwise station list | Toggle PLAY/STOP |
| Long press | STOP and OLED off | Open station list |
| Turn while pressed | Cycle visualization in either direction | Same |

In Mode A without a selected station, short press opens the list. In either mode,
turning in the list moves the highlight and a short press confirms it. A long
press in Mode A stops playback and exits; in Mode B it exits without changing
station. Turning while pressed has no action in the list.

## Selection semantics

The list clamps at its ends rather than wrapping. This differs from web previous/
next controls, which wrap through the entire catalog. The playing station continues
until confirmation or automatic selection. Reconfirming the same playing ID closes
the list without restarting audio.

The default list timeout is five seconds, with automatic selection. Web settings
allow 1–120 seconds and either selection or return without a change. Interaction
renews the timeout. A rejected selection command does not silently close the list.

The UI owns the selection and animation, while the player owns actual playback.
Selection commands include the catalog generation so a stale list cannot select
an unrelated station after edits. Station, volume and visualization persistence
is delayed by about five seconds; see [storage](11_STORAGE_SECURITY.md).

Mode A/B is a build-time choice, not a web setting. RC5 and a second encoder are
not implemented control sources. Future inputs must send commands through the same
owners rather than directly modifying I²S or rendering state.
