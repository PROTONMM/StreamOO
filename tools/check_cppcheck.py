#!/usr/bin/env python3
"""Analyze active project sources with the actual ESP-IDF compile configuration."""
import argparse
import json
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("database", type=Path, help="build directory's compile_commands.json")
    args = parser.parse_args()
    entries = json.loads(args.database.read_text())
    selected = []
    for entry in entries:
        source = Path(entry["file"])
        if not source.is_absolute():
            source = Path(entry["directory"]) / source
        source = source.resolve()
        if (source.suffix in (".c", ".cpp") and
                (ROOT / "components" in source.parents or ROOT / "main" in source.parents) and
                "third_party" not in source.parts):
            selected.append(entry)
    if not selected:
        parser.error("No active StreamOO sources found in compilation database")
    print(f"Cppcheck: {len(selected)} active project translation units", flush=True)
    with tempfile.TemporaryDirectory(prefix="streamoo-cppcheck-") as temporary:
        database = Path(temporary) / "compile_commands.json"
        database.write_text(json.dumps(selected))
        subprocess.run([
            "cppcheck", "--enable=warning,performance,portability", "--std=c11",
            "--inline-suppr", "--error-exitcode=1",
            # Upstream intentionally asserts !"message" in unreachable cases.
            # Keep all other diagnostics, including project wrapper warnings.
            "--suppress=incorrectStringBooleanError:" + str(
                ROOT / "components/decoder/third_party/dr_flac/dr_flac.h"),
            "--project=" + str(database),
        ], check=True, cwd=ROOT)


if __name__ == "__main__":
    main()
