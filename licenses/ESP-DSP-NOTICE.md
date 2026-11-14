# ESP-DSP used by StreamOO

Version: espressif/esp-dsp **1.8.2**, registry component hash
`ed9ac175b14e0414399a2b33c525579414b5bad19a2a67b12ea212def96f9a98`.
Upstream commit: `a53a0756833c045311ea1d79a2badf495cdfde4c`.
Source: https://github.com/espressif/esp-dsp/tree/v1.8.2

Copyright Espressif Systems (Shanghai) PTE LTD. Apache License 2.0;
the complete license is retained in [Apache-2.0.txt](Apache-2.0.txt).
No vendor sources are modified or copied into StreamOO's own components.
The component manager downloads the source with its original notices.

Audited 2026-11-01: LICENSE, idf_component.yml (IDF >=4.2),
modules/fft/include/dsps_fft2r.h, dsps_fft2r_platform.h,
modules/fft/fixed/dsps_fft2r_sc16_ansi.c and dsps_fft2r_sc16_aes3.S.
The latter contains the ESP32-S3 PIE/SIMD complex multiplies and vector
butterflies used by the target backend. The float AES3 transform uses scalar
FPU arithmetic with extended loads/stores; it is not selected for vector
arithmetic here.
