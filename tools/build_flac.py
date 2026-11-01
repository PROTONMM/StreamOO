#!/usr/bin/env python3
"""Build/flash one FLAC comparison backend with an explicit, persistent sdkconfig."""
import argparse
import hashlib
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
BACKENDS = {
    "micro": ("MICRO", b"micro-flac-0.3.0"),
    "dr": ("DR", b"dr_flac-0.13.4"),
    "esp": ("ESPRESSIF", b"espressif-2.6.2"),
}


def select_backend(text, backend):
    """Preserve all other settings; write exactly one choice for this backend."""
    symbols = {"CONFIG_STREAMOO_FLAC_" + item[0] for item in BACKENDS.values()}
    lines = [line for line in text.splitlines()
             if not any(line.startswith(symbol + "=") or line == "# " + symbol + " is not set"
                        for symbol in symbols)]
    selected = "CONFIG_STREAMOO_FLAC_" + BACKENDS[backend][0]
    lines += [symbol + "=y" if symbol == selected else "# " + symbol + " is not set"
              for symbol in sorted(symbols)]
    return "\n".join(lines) + "\n"


def verify(build_dir, backend):
    header = (build_dir / "config/sdkconfig.h").read_text()
    selected = [key for key, (symbol, _) in BACKENDS.items()
                if re.search(r"^#define CONFIG_STREAMOO_FLAC_" + symbol + r"\s+1\s*$", header, re.M)]
    if selected != [backend]:
        raise RuntimeError(f"Generated configuration selects {selected}, expected {backend}")
    binary = (build_dir / "streamoo.bin").read_bytes()
    labels = [key for key, (_, label) in BACKENDS.items() if label in binary]
    if labels != [backend]:
        raise RuntimeError(f"Firmware contains backend labels {labels}, expected {backend}")
    return len(binary), hashlib.sha256(binary).hexdigest()


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("backend", choices=BACKENDS)
    parser.add_argument("action", choices=("build", "flash"), nargs="?", default="build")
    parser.add_argument("--port", help="Serial port for flash, e.g. /dev/ttyUSB0")
    args = parser.parse_args(argv)
    idf_path = os.environ.get("IDF_PATH")
    idf = Path(idf_path) / "tools/idf.py" if idf_path else shutil.which("idf.py")
    if not idf or not Path(idf).is_file():
        parser.error("Activate the ESP-IDF6.1 environment first (IDF_PATH or idf.py required).")
    # Keep configuration outside build/: fullclean must not discard the choice.
    config = ROOT / f"sdkconfig.flac-{args.backend}"
    source = config if config.exists() else ROOT / "sdkconfig"
    if not source.exists():
        source = ROOT / "sdkconfig.defaults"
    config.write_text(select_backend(source.read_text(), args.backend))
    build_dir = ROOT / "build" / f"flac-{args.backend}"
    command = [sys.executable, str(idf), "-C", str(ROOT), "-B", str(build_dir),
               "-D", "SDKCONFIG=" + str(config)]
    subprocess.run(command + ["build"], cwd=ROOT, check=True)
    size, digest = verify(build_dir, args.backend)
    print(f"Verified FLAC backend={BACKENDS[args.backend][1].decode()} bytes={size} SHA256={digest}", flush=True)
    if args.action == "flash":
        port = ["-p", args.port] if args.port else []
        subprocess.run(command + port + ["flash"], cwd=ROOT, check=True)
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (RuntimeError, OSError, subprocess.CalledProcessError) as error:
        print(str(error), file=sys.stderr)
        sys.exit(1)
