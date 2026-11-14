# 14 — Feature and Validation Map

[English index](00_INDEX.md)

The files below are executable host test entry points. Listing a test is not a claim
that a hardware acceptance item has passed. Run the suite and review its result and
skips; use the board procedures for physical behavior.

| Feature | Host evidence | Hardware follow-up |
| --- | --- | --- |
| GPIO and N16R8 profile | [GPIO](../../tests/test_gpio_config.py), [platform](../../tests/test_platform_config.py) | Verify wiring and actual memory |
| SSD1322 and canvas | [display](../../tests/test_ssd1322.py), [canvas](../../tests/test_canvas.py) | Cold reset, orientation, grayscale, timing |
| Fonts and text | [small font](../../tests/test_font5x8.py), [layout](../../tests/test_text_layout.py), [bitmap font](../../tests/test_bitmap_font.py) | Readability and scrolling |
| Home, list and boot | [home](../../tests/test_home.py), [UI](../../tests/test_ui.py), [boot](../../tests/test_boot_transition.py) | Startup/AP/STOP transitions on OLED |
| Encoder A/B | [input](../../tests/test_input.py), [controls](../../tests/test_controls.py) | Detents, direction, fast turning |
| Four visualizations | [widgets](../../tests/test_visualization.py), [envelopes](../../tests/test_spectrum_envelope.py) | Contrast and held peaks |
| RMS/peak | [levels](../../tests/test_levels.py), [ballistics](../../tests/test_ballistics.py), [metering](../../tests/test_metering.py) | Response under playback load |
| FFT/Hann/SIMD | [FFT](../../tests/test_spectrum.py), [sc16](../../tests/test_spectrum_sc16.py), [stream](../../tests/test_spectrum_stream.py) | CPU/time comparison and scheduling |
| Gain and PCM | [gain](../../tests/test_audio_gain.py), [PCM](../../tests/test_pcm.py) | Clean changes, intentional stall recovery |
| Live audio and codecs | [live](../../tests/test_live_stream.py), [formats](../../tests/test_extended_decoder.py), [FLAC backends](../../tests/test_flac_backends.py) | Per-format listening and stress |
| FLAC parser | [frames](../../tests/test_flac_parse.py), [headers](../../tests/test_flac_header.py) | Public/live input and CRC recovery |
| Transport and HLS | [HTTP](../../tests/test_http_stream.py), [playlists](../../tests/test_playlist_resolver.py), [HLS](../../tests/test_hls.py) | TLS and repeated segment reloads |
| Player ownership | [network player](../../tests/test_network_player.py), [buffered source](../../tests/test_buffered_source.py) | Switching and Wi-Fi loss |
| Catalog storage | [model](../../tests/test_configuration_model.py), [store](../../tests/test_configuration_store.py), [stations](../../tests/test_stations.py) | Power interruption and concurrent editing |
| Wi-Fi/AP | [network config](../../tests/test_network_config.py), [startup](../../tests/test_network_startup.py), [AP/audio](../../tests/test_ap_audio_startup.py) | Fallback and AP-off on real routers |
| mDNS and time | [discovery](../../tests/test_discovery.py), [clock](../../tests/test_clock.py), [clock persistence](../../tests/test_clock_settings.py) | Multicast, DST display, HTTPS startup |
| Web/API | [HTTP](../../tests/test_portal_http.py), [status](../../tests/test_portal_status.py), [configuration](../../tests/test_portal_configuration.py) | LAN/AP transitions and phone operation |
| Slider and frontend | [volume](../../tests/test_portal_volume.py), [frontend](../../tests/test_portal_frontend.py) | Physical dragging and disconnect behavior |
| Bitrate | [bitrate](../../tests/test_bitrate.py) | Observed variable-rate sources and display |
| Deferred settings | [preferences](../../tests/test_preferences.py), [system](../../tests/test_system_settings.py) | Power cycle before/after save interval |
| Flash and profiles | [partitions](../../tests/test_partition_layout.py), [release](../../tests/test_release_build.py), [FLAC builds](../../tests/test_flac_build.py) | Wired update preserving data |

Browser integration: [check_portal_browser.py](../../tools/check_portal_browser.py).
Layout checks: [check_portal_layout.py](../../tools/check_portal_layout.py).
Both use local synthetic data and are additional to the unittest suite.
