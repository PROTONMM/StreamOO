# ESP-IDF native Wi-Fi integration

StreamOO09a uses native esp_wifi/esp_netif/esp_event and hardware support
from the pinned ESP-IDF6.1 SDK. The esp_wifi public header records copyright
2015-2026 Espressif Systems (Shanghai) CO LTD, SPDX Apache-2.0. The local
SDK esp_wifi/lib/LICENSE is Apache License2.0. Retain Apache-2.0.txt and
SDK notices with firmware distributions. No SDK implementation was copied
into StreamOO sources, and no Arduino or GPL integration was added.

Reviewed local SDK public headers and LICENSE on2026-10-31. Actual linked
implementation and SDK version are recorded in docs/STATUS.md; this does
not claim radio certification, performance or feature coverage beyond09a.
