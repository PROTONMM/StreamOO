# Espressif audio codec used by StreamOO

Component: espressif/esp_audio_codec 2.6.2, pinned in idf_component.yml and
root dependencies.lock. Registry component hash:
a5b9275af74247e3ce536d16a7080e4d77207a185b63e1bea368eb4d57176c2b.
Archive records upstream commit f9110cbb471b4017b0faf9bd48fcac84c61ecb6d.

The component is supplied under Espressif Modified MIT, copyright2025
Espressif Systems (Shanghai) CO., LTD. The complete notice is retained in
Espressif-Audio-Codec-Modified-MIT.txt. It permits use only with Espressif
products and prohibits redistribution for non-Espressif products. This is
not unrestricted MIT. StreamOO targets ESP32-S3; the vendor binary is not
used in native host tests, which test only our original adapter/model code.

The vendor's official copyright list attributes the MP3 implementation to
The Android Open Source Project, copyright2005–2008, under Apache License2.0.
Retain Apache-2.0.txt alongside this notice. Only MP3 is registered by this
stage; AAC/other codecs are not enabled through our adapter. The ELF audit
found MP3 open/decode symbols and no AAC/Opus/FLAC open/decode symbols.
No GPL code was imported into target firmware by this integration.

Sources checked2026-10-25:
- https://components.espressif.com/components/espressif/esp_audio_codec/versions/2.6.2/license
- https://docs.espressif.com/projects/esp-adf/en/latest/COPYRIGHT.html
- Package LICENSE/idf_component.yml and public headers, downloaded from the official registry.

Dependencies are installed unmodified by ESP-IDF Component Manager into
ignored managed_components; firmware distributions need the retained notices.
The original generated MP3 signal is not third-party recorded material.
