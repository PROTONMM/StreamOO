# Native HTTP/ICY/TLS source and license record

StreamOO 06i uses the existing pinned **ESP-IDF 6.1** checkout
`fff9895c82d744c7237be8847347bdd1b07c6643`; no replacement SDK library downloaded.
Reviewed local public APIs, manifests/source and licenses before integration.

- `components/esp_http_client` — Apache-2.0, Espressif Systems 2015–2026.
  `open/fetch_headers/read/close`, manual redirects and verified TLS bundle.
- `components/http_parser` — SDK SBOM version 2.7.0, NGINX/Joyent/Node
  contributors, MIT. Retained [license](HTTP-Parser-MIT.txt). Pinned parser
  source explicitly supports `ICY 200`; client treats unknown content length
  as streaming data. SDK handles HTTP chunk framing before our ICY filter.
- `components/esp_netif/include/esp_netif_sntp.h` — Apache-2.0,
  Espressif Systems 2022–2025. Default SNTP API from the same checkout.
- SDK **Mbed TLS 4.1.0**, verified in `include/mbedtls/build_info.h`.
  Root license offers Apache-2.0 OR GPL-2.0-or-later; this project chooses
  **Apache-2.0**. Retained [Apache text](Apache-2.0.txt), no GPL code imported
  under GPL terms. SDK source/license:
  https://github.com/espressif/esp-idf/tree/fff9895c82d744c7237be8847347bdd1b07c6643/components/mbedtls

The full built-in SDK CA bundle is generated from Mozilla's certificate data,
converted via curl. That data is **MPL-2.0**, not the SDK Apache license.
Retained [MPL text](MPL-2.0.txt) and corresponding certificate source
[`sources/cacrt_all.pem`](sources/cacrt_all.pem). Source header dates its
Mozilla data to 2026-02-11; this is the pinned SDK snapshot, not a claim that
it is the newest available CA set. PEM blocks match the SDK exactly; only
trailing whitespace/blank lines in the retained source were normalized.
Build uses the SDK's unmodified copy and `esp_crt_bundle` generator.

Recipients can obtain the CA source directly from the supplied PEM file
under MPL-2.0; preserve this notice, MPL text and source with firmware delivery.
This file-level license applies to the CA data; original StreamOO protocol,
PCM conversion, player and UI code remain separate files.

Upstream source and terms:
- https://github.com/espressif/esp-idf/blob/fff9895c82d744c7237be8847347bdd1b07c6643/components/mbedtls/esp_crt_bundle/cacrt_all.pem
- https://curl.se/docs/caextract.html (CA store license section)
- https://www.mozilla.org/en-US/MPL/2.0/

No private station playlist is part of this notice, certificate source or build.
