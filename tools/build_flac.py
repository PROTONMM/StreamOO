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


def idf_python(idf, environ):
    """Select IDF's interpreter, independently of the Python running this helper."""
    subdir = "Scripts" if os.name == "nt" else "bin"
    executable = "python.exe" if os.name == "nt" else "python"
    explicit = environ.get("IDF_PYTHON_ENV_PATH")
    if explicit:
        candidate = Path(explicit).expanduser() / subdir / executable
        if not candidate.is_file():
            raise RuntimeError(f"IDF_PYTHON_ENV_PATH has no Python: {candidate}. Activate ESP-IDF6.1 first.")
        return candidate
    # The SDK's canonical environment naming follows get_python_env_path() in
    # idf_tools.py. Match this SDK version; never choose an unrelated IDF env.
    version_file = Path(idf).resolve().parents[1] / "tools/cmake/version.cmake"
    version_text = version_file.read_text()
    major = re.search(r"set\(IDF_VERSION_MAJOR\s+(\d+)\)", version_text)
    minor = re.search(r"set\(IDF_VERSION_MINOR\s+(\d+)\)", version_text)
    if not major or not minor:
        raise RuntimeError("Cannot determine the SDK version. Activate ESP-IDF6.1 first.")
    version = major[1] + "." + minor[1]
    tools_path = Path(environ.get("IDF_TOOLS_PATH", str(Path.home() / ".espressif")))
    environments = tools_path / "python_env"
    preferred = environments / f"idf{version}_py{sys.version_info.major}.{sys.version_info.minor}_env" / subdir / executable
    if preferred.is_file():
        return preferred
    candidates = sorted(path for path in environments.glob(f"idf{version}_py*_env/{subdir}/{executable}") if path.is_file())
    if len(candidates) == 1:
        return candidates[0]
    raise RuntimeError(f"Cannot select one ESP-IDF{version} Python environment. Activate that SDK or set IDF_PYTHON_ENV_PATH.")


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


def select_software_crypto(text):
    """Use the SDK software AES/SHA implementations; preserve TLS verification."""
    symbols=("CONFIG_MBEDTLS_HARDWARE_AES", "CONFIG_MBEDTLS_HARDWARE_SHA", "CONFIG_STREAMOO_CRYPTO_EARLY_INIT")
    lines=[line for line in text.splitlines() if not any(
        line.startswith(symbol+"=") or line=="# "+symbol+" is not set" for symbol in symbols)]
    return "\n".join(lines+["# "+symbol+" is not set" for symbol in symbols])+"\n"


def select_hardware_crypto(text):
    """Reserve the SDK hardware crypto channels before the first I2S use."""
    symbols=("CONFIG_MBEDTLS_HARDWARE_AES", "CONFIG_MBEDTLS_HARDWARE_SHA", "CONFIG_STREAMOO_CRYPTO_EARLY_INIT")
    lines=[line for line in text.splitlines() if not any(
        line.startswith(symbol+"=") or line=="# "+symbol+" is not set" for symbol in symbols)]
    return "\n".join(lines+[symbol+"=y" for symbol in symbols])+"\n"


def tcp_mailbox(window, mss):
    if not 5760 <= window <= 32768 or not 536 <= mss <= 1460:
        raise RuntimeError("TCP window must be 5760..32768 bytes; MSS must be 536..1460")
    slots=max(6,(window+mss-1)//mss+2)
    if slots>64:
        raise RuntimeError("TCP receive mailbox exceeds the unscaled SDK limit")
    return slots


def select_tcp_window(text, window):
    match=re.search(r"^CONFIG_LWIP_TCP_MSS=(\d+)\s*$",text,re.M)
    mailbox=tcp_mailbox(window,int(match[1]) if match else 1440)
    values={"CONFIG_LWIP_TCP_WND_DEFAULT":window,"CONFIG_LWIP_TCP_RECVMBOX_SIZE":mailbox}
    lines=[line for line in text.splitlines() if not any(
        line.startswith(symbol+"=") or line=="# "+symbol+" is not set" for symbol in values)]
    return "\n".join(lines+[f"{symbol}={value}" for symbol,value in values.items()])+"\n"


def verify(build_dir, backend, software_crypto=False, tcp_window=None, hardware_crypto=False):
    header = (build_dir / "config/sdkconfig.h").read_text()
    selected = [key for key, (symbol, _) in BACKENDS.items()
                if re.search(r"^#define CONFIG_STREAMOO_FLAC_" + symbol + r"\s+1\s*$", header, re.M)]
    if selected != [backend]:
        raise RuntimeError(f"Generated configuration selects {selected}, expected {backend}")
    if software_crypto and re.search(r"^#define CONFIG_MBEDTLS_HARDWARE_(AES|SHA)\b",header,re.M):
        raise RuntimeError("Generated configuration still enables hardware AES/SHA; software crypto required")
    if hardware_crypto:
        for symbol in ("CONFIG_MBEDTLS_HARDWARE_AES","CONFIG_MBEDTLS_HARDWARE_SHA","CONFIG_STREAMOO_CRYPTO_EARLY_INIT"):
            if not re.search(r"^#define "+symbol+r"\s+1\s*$",header,re.M):
                raise RuntimeError(f"Generated configuration missing {symbol}; hardware crypto with early init required")
    if tcp_window is not None:
        def setting(symbol):
            match=re.search(r"^#define "+symbol+r"\s+(\d+)\s*$",header,re.M)
            if not match:
                raise RuntimeError(f"Generated configuration is missing {symbol}")
            return int(match[1])
        expected=tcp_mailbox(tcp_window,setting("CONFIG_LWIP_TCP_MSS"))
        if (setting("CONFIG_LWIP_TCP_WND_DEFAULT")!=tcp_window or
                setting("CONFIG_LWIP_TCP_RECVMBOX_SIZE")<expected):
            raise RuntimeError("Generated TCP receive window/mailbox does not match the requested profile")
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
    crypto=parser.add_mutually_exclusive_group()
    crypto.add_argument("--software-crypto", action="store_true",
                        help="Persist SDK software AES/SHA and verify the generated config before flash")
    crypto.add_argument("--hardware-crypto",action="store_true",
                        help="Persist hardware AES/SHA with checked early GDMA initialization; verify before flash")
    parser.add_argument("--tcp-window",type=int,help="TCP RX window 5760..32768 bytes; also sizes and verifies the receive mailbox")
    args = parser.parse_args(argv)
    idf_path = os.environ.get("IDF_PATH")
    idf = Path(idf_path) / "tools/idf.py" if idf_path else shutil.which("idf.py")
    if not idf or not Path(idf).is_file():
        parser.error("Activate the ESP-IDF6.1 environment first (IDF_PATH or idf.py required).")
    python = idf_python(idf, os.environ)
    # Keep the venv executable path: resolving its symlink to /usr/bin/python3
    # would lose Python's virtual-environment discovery and recreate this bug.
    environment = dict(os.environ)
    environment.update(IDF_PYTHON_ENV_PATH=str(python.parent.parent),
                       VIRTUAL_ENV=str(python.parent.parent), PYTHON=str(python))
    environment["PATH"] = str(python.parent) + os.pathsep + environment.get("PATH", "")
    print(f"ESP-IDF Python: {python}", flush=True)
    # Keep configuration outside build/: fullclean must not discard the choice.
    config = ROOT / f"sdkconfig.flac-{args.backend}"
    source = config if config.exists() else ROOT / "sdkconfig"
    if not source.exists():
        source = ROOT / "sdkconfig.defaults"
    selected=select_backend(source.read_text(), args.backend)
    if args.software_crypto:
        selected=select_software_crypto(selected)
    elif args.hardware_crypto:
        selected=select_hardware_crypto(selected)
    if args.tcp_window is not None:
        selected=select_tcp_window(selected,args.tcp_window)
    config.write_text(selected)
    build_dir = ROOT / "build" / f"flac-{args.backend}"
    command = [str(python), str(idf), "-C", str(ROOT), "-B", str(build_dir),
               "-D", "SDKCONFIG=" + str(config)]
    cache = build_dir / "CMakeCache.txt"
    if cache.exists():
        configured = re.search(r"^PYTHON:[^=\n]+=(.+)$", cache.read_text(), re.M)
        if configured and os.path.abspath(configured[1]) != os.path.abspath(python):
            # A clean CMake directory is required after an interpreter change.
            # Preserve old ELF/logs and avoid IDF fullclean hooks that can also
            # remove the project's shared managed_components directory.
            number = 1
            backup = build_dir.with_name(build_dir.name + f"-previous-python-{number}")
            while backup.exists():
                number += 1
                backup = build_dir.with_name(build_dir.name + f"-previous-python-{number}")
            build_dir.rename(backup)
            print(f"ESP-IDF Python changed; previous build preserved at {backup}", flush=True)
    subprocess.run(command + ["build"], cwd=ROOT, env=environment, check=True)
    size, digest = verify(build_dir, args.backend, args.software_crypto,args.tcp_window,args.hardware_crypto)
    if args.software_crypto:
        print("Verified TLS crypto: software AES/SHA (generated configuration)", flush=True)
    elif args.hardware_crypto:
        print("Verified TLS crypto: hardware AES/SHA with early init (generated configuration)",flush=True)
    if args.tcp_window is not None:
        print(f"Verified TCP RX window: {args.tcp_window} bytes (generated configuration)",flush=True)
    print(f"Verified FLAC backend={BACKENDS[args.backend][1].decode()} bytes={size} SHA256={digest}", flush=True)
    if args.action == "flash":
        port = ["-p", args.port] if args.port else []
        subprocess.run(command + port + ["flash"], cwd=ROOT, env=environment, check=True)
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (RuntimeError, OSError, subprocess.CalledProcessError) as error:
        print(str(error), file=sys.stderr)
        sys.exit(1)
