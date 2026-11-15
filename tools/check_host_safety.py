#!/usr/bin/env python3
"""Run existing native executable tests with ASan/UBSan, then GCC analysis.

Run after idf.py reconfigure has fetched the locked managed components.
This does not instrument shared objects loaded into an unsanitized Python.
"""
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


def main():
    os.chdir(ROOT)
    flags = ("-fsanitize=address,undefined -fno-sanitize-recover=all "
             "-fno-omit-frame-pointer -g -O1 -no-pie")
    # The existing opt-in variables only instrument standalone executables.
    suite = unittest.TestSuite()
    for path in sorted((ROOT / "tests").glob("test_*.py")):
        variables = set(re.findall(r"STREAMOO_[A-Z0-9_]+_SANITIZE", path.read_text()))
        if variables:
            for variable in variables:
                os.environ[variable] = flags
            suite.addTests(unittest.defaultTestLoader.discover("tests", path.name))
    os.environ["UBSAN_OPTIONS"] = "halt_on_error=1:print_stacktrace=1"
    result = unittest.TextTestRunner(verbosity=2).run(suite)
    if not result.wasSuccessful() or result.skipped:
        print("Safety suite failed or skipped required coverage", file=sys.stderr)
        return 1

    includes = ["-I" + str(path) for path in sorted((ROOT / "components").glob("*/include"))]
    includes += ["-Imanaged_components/espressif__cjson/cJSON"]
    # Portable project code only; SDK-dependent owners are covered by firmware
    # compilation and the existing host adapter tests, not fake analyzer headers.
    sources = [
        "streaming/streamoo_hls.c", "streaming/streamoo_playlist_resolver.c",
        "streaming/streamoo_icy.c", "configuration/streamoo_config_model.c",
        "configuration/streamoo_catalog_format.c", "stations/streamoo_stations.c",
        "compressed_source/streamoo_byte_source.c", "pcm_adapter/streamoo_pcm_adapter.c",
        "audio_gain/streamoo_audio_gain.c", "pcm/streamoo_pcm.c",
        "levels/streamoo_levels.c", "levels/streamoo_ballistics.c",
        "graphics/streamoo_canvas.c", "text/streamoo_text.c",
        "text/streamoo_text_layout.c", "text/streamoo_bitmap_font.c",
        "spectrum/streamoo_spectrum.c", "spectrum/streamoo_spectrum_stream.c",
        "spectrum/streamoo_spectrum_envelope.c",
    ]
    with tempfile.TemporaryDirectory(prefix="streamoo-analysis-") as tmp:
        for index, source in enumerate(sources):
            subprocess.run([
                "gcc", "-std=c11", "-D_POSIX_C_SOURCE=200809L", "-Wall", "-Wextra",
                "-Werror", "-fanalyzer", "-O1", *includes, "-c", "components/" + source,
                "-o", str(Path(tmp) / f"{index}.o"),
            ], check=True)
    print(f"GCC -fanalyzer: {len(sources)} portable project translation units PASS")
    return 0


if __name__ == "__main__":
    sys.exit(main())
