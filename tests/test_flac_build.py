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
    def test_tcp_window_sizes_mailbox_preserves_settings_and_is_idempotent(self):
        original='CONFIG_LWIP_TCP_MSS=1440\nCONFIG_LWIP_TCP_WND_DEFAULT=5760\nCONFIG_LWIP_TCP_RECVMBOX_SIZE=6\nCONFIG_BOARD_PIN=6\n'
        selected=build.select_tcp_window(original,23040)
        self.assertIn('CONFIG_LWIP_TCP_WND_DEFAULT=23040\n',selected)
        self.assertIn('CONFIG_LWIP_TCP_RECVMBOX_SIZE=18\n',selected)
        self.assertIn('CONFIG_BOARD_PIN=6\n',selected)
        self.assertEqual(build.select_tcp_window(selected,23040),selected)
        self.assertIn('CONFIG_LWIP_TCP_RECVMBOX_SIZE=25\n',build.select_tcp_window('CONFIG_LWIP_TCP_MSS=1024\n',23040))
        self.assertEqual(build.tcp_mailbox(32768,536),64)

    def test_tcp_profile_rejects_out_of_range_values(self):
        for window in (0,5759,32769):
            with self.assertRaises(RuntimeError):build.select_tcp_window('',window)
        for mss in (0,535,1461):
            with self.assertRaises(RuntimeError):build.tcp_mailbox(23040,mss)

    def test_generated_tcp_window_and_mailbox_are_required(self):
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp);(root/'config').mkdir();(root/'streamoo.bin').write_bytes(b'dr_flac-0.13.4\0')
            base='#define CONFIG_STREAMOO_FLAC_DR 1\n#define CONFIG_LWIP_TCP_MSS 1440\n'
            for window,mailbox in ((5760,18),(23040,6)):
                (root/'config/sdkconfig.h').write_text(base+f'#define CONFIG_LWIP_TCP_WND_DEFAULT {window}\n#define CONFIG_LWIP_TCP_RECVMBOX_SIZE {mailbox}\n')
                with self.assertRaises(RuntimeError):build.verify(root,'dr',tcp_window=23040)
            (root/'config/sdkconfig.h').write_text(base)
            with self.assertRaises(RuntimeError):build.verify(root,'dr',tcp_window=23040)
            (root/'config/sdkconfig.h').write_text(base+'#define CONFIG_LWIP_TCP_WND_DEFAULT 23040\n#define CONFIG_LWIP_TCP_RECVMBOX_SIZE 18\n')
            self.assertGreater(build.verify(root,'dr',tcp_window=23040)[0],0)

    def test_tcp_profile_blocks_wrong_flash_and_persists_successful_settings(self):
        for correct in (False,True):
            with self.subTest(correct=correct),tempfile.TemporaryDirectory() as temp:
                root=Path(temp);(root/'tools').mkdir();(root/'tools/idf.py').write_text('')
                original='CONFIG_LWIP_TCP_MSS=1440\nCONFIG_LWIP_TCP_WND_DEFAULT=5760\nCONFIG_LWIP_TCP_RECVMBOX_SIZE=6\nCONFIG_BOARD_PIN=6\n'
                (root/'sdkconfig').write_text(original)
                def fake_run(command,**kwargs):
                    selected=(root/'sdkconfig.flac-dr').read_text()
                    self.assertIn('CONFIG_LWIP_TCP_RECVMBOX_SIZE=18',selected)
                    self.assertIn('CONFIG_BOARD_PIN=6',selected)
                    if command[-1]=='build':
                        artifact=root/'build/flac-dr';(artifact/'config').mkdir(parents=True,exist_ok=True)
                        window=23040 if correct else 5760
                        (artifact/'config/sdkconfig.h').write_text('#define CONFIG_STREAMOO_FLAC_DR 1\n#define CONFIG_LWIP_TCP_MSS 1440\n'+f'#define CONFIG_LWIP_TCP_WND_DEFAULT {window}\n#define CONFIG_LWIP_TCP_RECVMBOX_SIZE 18\n')
                        (artifact/'streamoo.bin').write_bytes(b'dr_flac-0.13.4\0')
                with patch.object(build,'ROOT',root),patch.dict(build.os.environ,{'IDF_PATH':str(root)}), \
                        patch.object(build,'idf_python',return_value=root/'idf-venv/bin/python'), \
                        patch.object(build.subprocess,'run',side_effect=fake_run) as run,contextlib.redirect_stdout(io.StringIO()):
                    command=['dr','flash','--port','/dev/unused','--tcp-window','23040']
                    if correct:
                        self.assertEqual(build.main(command),0)
                        self.assertEqual(run.call_count,2)
                        self.assertEqual(build.main(['dr','build']),0)
                    else:
                        with self.assertRaises(RuntimeError):build.main(command)
                        self.assertEqual(run.call_count,1)
                    self.assertEqual((root/'sdkconfig').read_text(),original)

    def test_software_crypto_selection_is_idempotent_and_preserves_other_settings(self):
        original='CONFIG_MBEDTLS_HARDWARE_AES=y\nCONFIG_MBEDTLS_HARDWARE_SHA=y\nCONFIG_BOARD_PIN=6\n'
        selected=build.select_software_crypto(original)
        for symbol in ('AES','SHA'):
            self.assertIn('# CONFIG_MBEDTLS_HARDWARE_'+symbol+' is not set\n',selected)
        self.assertIn('CONFIG_BOARD_PIN=6\n',selected)
        self.assertEqual(build.select_software_crypto(selected),selected)
        self.assertIn('# CONFIG_MBEDTLS_HARDWARE_AES is not set',build.select_backend(selected,'dr'))

    def test_software_crypto_requires_generated_header_to_match(self):
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp);(root/'config').mkdir();(root/'streamoo.bin').write_bytes(b'dr_flac-0.13.4\0')
            for symbol in ('AES','SHA'):
                (root/'config/sdkconfig.h').write_text('#define CONFIG_STREAMOO_FLAC_DR 1\n#define CONFIG_MBEDTLS_HARDWARE_'+symbol+' 1\n')
                with self.assertRaises(RuntimeError):build.verify(root,'dr',True)
            (root/'config/sdkconfig.h').write_text('#define CONFIG_STREAMOO_FLAC_DR 1\n')
            self.assertGreater(build.verify(root,'dr',True)[0],0)

    def test_software_crypto_flag_stops_flash_if_sdk_reenables_hardware(self):
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp);(root/'tools').mkdir();(root/'tools/idf.py').write_text('')
            original='CONFIG_MBEDTLS_HARDWARE_AES=y\nCONFIG_MBEDTLS_HARDWARE_SHA=y\nCONFIG_BOARD_PIN=6\n'
            (root/'sdkconfig').write_text(original)
            def fake_run(command,**kwargs):
                self.assertNotIn('flash',command)
                config=(root/'sdkconfig.flac-dr').read_text()
                self.assertEqual(build.select_software_crypto(config),config)
                self.assertIn('CONFIG_BOARD_PIN=6',config)
                artifact=root/'build/flac-dr';(artifact/'config').mkdir(parents=True)
                (artifact/'config/sdkconfig.h').write_text('#define CONFIG_STREAMOO_FLAC_DR 1\n#define CONFIG_MBEDTLS_HARDWARE_SHA 1\n')
                (artifact/'streamoo.bin').write_bytes(b'dr_flac-0.13.4\0')
            with patch.object(build,'ROOT',root),patch.dict(build.os.environ,{'IDF_PATH':str(root)}), \
                    patch.object(build,'idf_python',return_value=root/'idf-venv/bin/python'), \
                    patch.object(build.subprocess,'run',side_effect=fake_run) as run,contextlib.redirect_stdout(io.StringIO()):
                with self.assertRaises(RuntimeError):build.main(['dr','flash','--software-crypto','--port','/dev/unused'])
                self.assertEqual(run.call_count,1)
                self.assertEqual((root/'sdkconfig').read_text(),original)

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
                    patch.object(build, "idf_python", return_value=root / "idf-venv/bin/python"), \
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
                    patch.object(build, "idf_python", return_value=root / "idf-venv/bin/python"), \
                    patch.object(build.subprocess, "run", side_effect=fake_run) as run, \
                    contextlib.redirect_stdout(io.StringIO()):
                self.assertEqual(build.main(["dr", "flash", "--port", "/dev/unused"]), 0)
                self.assertEqual(run.call_count, 2)
                for call in run.call_args_list:
                    self.assertIn("SDKCONFIG=" + str(root / "sdkconfig.flac-dr"), call.args[0])
                    self.assertEqual(call.args[0][0], str(root / "idf-venv/bin/python"))
                    self.assertEqual(call.kwargs["env"]["IDF_PYTHON_ENV_PATH"], str(root / "idf-venv"))
                self.assertIn("CONFIG_BOARD_PIN=6", (root / "sdkconfig.flac-dr").read_text())
                self.assertEqual((root / "sdkconfig").read_text(), "CONFIG_STREAMOO_FLAC_MICRO=y\nCONFIG_BOARD_PIN=6\n")

    def test_explicit_idf_environment_preserves_venv_symlink_path(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); (root / "bin").mkdir()
            (root / "bin/python").symlink_to(build.sys.executable)
            selected = build.idf_python(root / "unused-idf.py", {"IDF_PYTHON_ENV_PATH": str(root)})
            self.assertEqual(selected, root / "bin/python")
            self.assertNotEqual(str(selected), build.sys.executable)

    def test_missing_explicit_environment_does_not_fall_back_to_system_python(self):
        with tempfile.TemporaryDirectory() as tmp:
            with self.assertRaises(RuntimeError):
                build.idf_python(Path(tmp) / "idf.py", {"IDF_PYTHON_ENV_PATH": tmp})

    def test_discovers_only_matching_sdk_version(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); (root / "sdk/tools/cmake").mkdir(parents=True)
            (root / "sdk/tools/cmake/version.cmake").write_text("set(IDF_VERSION_MAJOR 6)\nset(IDF_VERSION_MINOR 1)\n")
            for version in ("5.5", "6.1"):
                executable = root / f"tools/python_env/idf{version}_py3.12_env/bin/python"
                executable.parent.mkdir(parents=True); executable.touch()
            selected = build.idf_python(root / "sdk/tools/idf.py", {"IDF_TOOLS_PATH": str(root / "tools")})
            self.assertEqual(selected, root / "tools/python_env/idf6.1_py3.12_env/bin/python")

    def test_changed_idf_python_preserves_previous_build_and_shared_sources(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); (root / "tools").mkdir()
            (root / "tools/idf.py").write_text("")
            (root / "sdkconfig").write_text("CONFIG_STREAMOO_FLAC_MICRO=y\n")
            artifact = root / "build/flac-dr"; artifact.mkdir(parents=True)
            (artifact / "CMakeCache.txt").write_text("PYTHON:FILEPATH=/previous-idf-venv/bin/python\n")
            (root / "managed_components").mkdir()
            (root / "managed_components/keep.h").write_text("untouched")
            def fake_run(command, **kwargs):
                self.assertTrue((root / "sdkconfig.flac-dr").exists())
                if command[-1] == "build":
                    self.assertTrue((root / "build/flac-dr-previous-python-1/CMakeCache.txt").exists())
                    (artifact / "config").mkdir(parents=True)
                    (artifact / "config/sdkconfig.h").write_text("#define CONFIG_STREAMOO_FLAC_DR 1\n")
                    (artifact / "streamoo.bin").write_bytes(b"dr_flac-0.13.4\0")
                else:
                    self.fail("Unexpected flash action")
            with patch.object(build, "ROOT", root), patch.dict(build.os.environ, {"IDF_PATH": str(root)}), \
                    patch.object(build, "idf_python", return_value=root / "idf-venv/bin/python"), \
                    patch.object(build.subprocess, "run", side_effect=fake_run) as run, \
                    contextlib.redirect_stdout(io.StringIO()):
                self.assertEqual(build.main(["dr", "build"]), 0)
                self.assertEqual([call.args[0][-1] for call in run.call_args_list], ["build"])
                self.assertTrue((root / "sdkconfig.flac-dr").exists())
                self.assertEqual((root / "managed_components/keep.h").read_text(), "untouched")
