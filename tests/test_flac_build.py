"""Backend choice and binary checks prevent comparing/flashing the wrong codec."""
import importlib.util
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch
import contextlib
import io

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("flac_build", ROOT / "tools/build_flac.py")
build = importlib.util.module_from_spec(spec)
spec.loader.exec_module(build)


class FlacBuildTests(unittest.TestCase):
    def test_choice_preserves_other_settings_and_is_idempotent(self):
        original = "CONFIG_STREAMOO_FLAC_MICRO=y\n# CONFIG_STREAMOO_FLAC_DR is not set\nCONFIG_BOARD_PIN=6\n"
        for backend, (symbol, _) in build.BACKENDS.items():
            with self.subTest(backend=backend):
                result = build.select_backend(original, backend)
                self.assertIn("CONFIG_BOARD_PIN=6\n", result)
                self.assertIn("CONFIG_STREAMOO_FLAC_" + symbol + "=y\n", result)
                self.assertEqual(sum(line.endswith("=y") for line in result.splitlines()), 1)
                self.assertEqual(build.select_backend(result, backend), result)

    def test_checks_actual_config_and_binary(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); (root / "config").mkdir()
            (root / "config/sdkconfig.h").write_text("#define CONFIG_STREAMOO_FLAC_DR 1\n")
            (root / "streamoo.bin").write_bytes(b"test dr_flac-0.13.4\0")
            self.assertEqual(build.verify(root, "dr")[0], 20)
            with self.assertRaises(RuntimeError):
                build.verify(root, "micro")
            (root / "streamoo.bin").write_bytes(b"micro-flac-0.3.0\0")
            with self.assertRaises(RuntimeError):
                build.verify(root, "dr")

    def test_rejects_ambiguous_generated_choice(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); (root / "config").mkdir()
            (root / "config/sdkconfig.h").write_text(
                "#define CONFIG_STREAMOO_FLAC_DR 1\n#define CONFIG_STREAMOO_FLAC_MICRO 1\n")
            with self.assertRaises(RuntimeError):
                build.verify(root, "dr")

    def test_flash_stops_if_build_produced_another_backend(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); (root / "tools").mkdir()
            (root / "tools/idf.py").write_text("")
            (root / "sdkconfig").write_text("CONFIG_STREAMOO_FLAC_MICRO=y\nCONFIG_BOARD_PIN=6\n")
            def fake_run(command, **kwargs):
                artifact = root / "build/flac-dr"; (artifact / "config").mkdir(parents=True)
                (artifact / "config/sdkconfig.h").write_text("#define CONFIG_STREAMOO_FLAC_MICRO 1\n")
                (artifact / "streamoo.bin").write_bytes(b"micro-flac-0.3.0\0")
            with patch.object(build, "ROOT", root), patch.dict(build.os.environ, {"IDF_PATH": str(root)}), \
                    patch.object(build.subprocess, "run", side_effect=fake_run) as run:
                with self.assertRaises(RuntimeError):
                    build.main(["dr", "flash", "--port", "/dev/unused"])
                self.assertEqual(run.call_count, 1)
                self.assertNotIn("flash", run.call_args.args[0])

    def test_build_and_flash_both_receive_persistent_config(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); (root / "tools").mkdir()
            (root / "tools/idf.py").write_text("")
            (root / "sdkconfig").write_text("CONFIG_STREAMOO_FLAC_MICRO=y\nCONFIG_BOARD_PIN=6\n")
            def fake_run(command, **kwargs):
                if command[-1] == "build":
                    artifact = root / "build/flac-dr"; (artifact / "config").mkdir(parents=True)
                    (artifact / "config/sdkconfig.h").write_text("#define CONFIG_STREAMOO_FLAC_DR 1\n")
                    (artifact / "streamoo.bin").write_bytes(b"dr_flac-0.13.4\0")
            with patch.object(build, "ROOT", root), patch.dict(build.os.environ, {"IDF_PATH": str(root)}), \
                    patch.object(build.subprocess, "run", side_effect=fake_run) as run, \
                    contextlib.redirect_stdout(io.StringIO()):
                self.assertEqual(build.main(["dr", "flash", "--port", "/dev/unused"]), 0)
                self.assertEqual(run.call_count, 2)
                for call in run.call_args_list:
                    self.assertIn("SDKCONFIG=" + str(root / "sdkconfig.flac-dr"), call.args[0])
                self.assertIn("CONFIG_BOARD_PIN=6", (root / "sdkconfig.flac-dr").read_text())
                self.assertEqual((root / "sdkconfig").read_text(), "CONFIG_STREAMOO_FLAC_MICRO=y\nCONFIG_BOARD_PIN=6\n")
