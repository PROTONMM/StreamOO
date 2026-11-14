# Development and Documentation Maintenance

[English index](00_INDEX.md)

The repository's [AGENTS.md](../../AGENTS.md) and
[CODEX_MASTER_PROMPT.md](../../CODEX_MASTER_PROMPT.md) remain the original project
instructions. This English reference describes the current workflow; it does not
replace those files or a later explicit user decision.

## Engineering rules

1. Read the index, current status and relevant component/stage documents before
   changing behavior. Keep hardware uncertainties and missing evidence explicit.
2. Use native ESP-IDF and explicit component interfaces. Do not introduce Arduino
   Core or copy yoRadio's web API. Check a dependency's API, version, limitations
   and license before importing or integrating it.
3. Preserve the real-time boundary: no allocation, flash operation, FFT or blocking
   I/O in I²S sample delivery. Analysis is droppable; audio output is not.
4. Use the schematic plus confirmed wiring corrections, including encoder B=6.
   Validate GPIO and N16R8 configuration before building.
5. Work in small steps. Run appropriate existing tests and build checks, record
   results and limitations, update status and document material decisions in ADRs.
6. Keep credentials outside the repository. Do not log passwords or import private
   station lists as fixtures. Do not claim hardware tests that were not performed.
7. Use the product name StreamOO; refer to yoRadio only as the source/reference
   project. New code and artifacts should not revive a previous modification name.

## Build and test workflow

Follow the [build guide](BUILD_GUIDE.md). `sdkconfig` is local and takes precedence
over defaults. Use isolated explicit configurations for diagnostic/FLAC comparisons;
do not assume `-B` changes configuration. Avoid concurrent cleanup of shared build
dependencies. Keep an ELF with any firmware used to collect crash reports.

Use the [validation matrix](14_TRACEABILITY.md) to select meaningful tests. For a
documentation-only change, verify links, commands, configuration names and factual
agreement with code. A successful build and host suite do not close hardware OPEN items.

## Assets

Normal builds use committed generated font/logo assets. They do not need to regenerate
fonts or install rasterization tools. For an intentional logo edit, the shared source
is `tools/generate_brand_mark.py`; host pycairo generates the SVG, web wordmark and
4bpp OLED mask. Preview regeneration:

```sh
python3 tools/generate_brand_mark.py
python3 tools/preview_boot_logo.py assets/branding/startup-256x64.png
python3 tools/preview_boot_logo.py assets/branding/startup-preview-4x.png --scale 4
```

Review all generated changes together. Font source records and regeneration steps
are in [assets/fonts/README.md](../../assets/fonts/README.md) and
[tools/font_generator/README.md](../../tools/font_generator/README.md), already in
English where relevant. Do not replace source fonts without checking their terms.

## English documentation scope

`README_EN.md` and `docs/en/` cover current operating instructions and final-form
technical documentation. By user decision, the chronological Polish status log,
historical experiments and full ADR archive are not translated. Archived links
are labeled as such; old proposals must not be presented as current defaults.

When changing a feature, update both current-language guides and the relevant English
technical chapter. Preserve API field names, Kconfig symbols, filenames, command
semantics, numeric limits and license notices. Translate link anchors when translating
headings, and adjust relative paths for the extra `en/` directory. Keep actual Polish
UI labels alongside their English meanings so readers can find controls.

## Commits and metrics

Do not add `AI-Model` or `AI-Tokens` trailers to commits, including documentation
commits. Do not rewrite older commit messages to change their historical policy.
Record model/token metrics only when verified source data is actually provided;
never estimate them from diffs, word counts, cost or a default model setting.
See the original [commit-metrics policy](../COMMIT_METRICS.md).
