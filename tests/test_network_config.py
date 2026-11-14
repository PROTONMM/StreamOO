from pathlib import Path
import ctypes as C
import importlib.util
import json
import os
import secrets
import stat
import subprocess
import tempfile
import unittest
ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('private_network',ROOT/'tools/configure_network.py')
config=importlib.util.module_from_spec(spec);spec.loader.exec_module(config)
class NetworkConfigTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp=tempfile.TemporaryDirectory();cls.addClassCleanup(cls.temp.cleanup)
        base=Path(cls.temp.name); lib=base/'policy.so'
        subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror','-shared','-fPIC',
            '-Icomponents/network/include','components/network/streamoo_network_policy.c','-o',str(lib)],cwd=ROOT,check=True)
        cls.lib=C.CDLL(str(lib));cls.valid=cls.lib.streamoo_network_password_valid
        cls.valid.argtypes=[C.c_void_p,C.c_size_t];cls.valid.restype=C.c_bool
        cls.name=cls.lib.streamoo_network_ap_name;cls.name.argtypes=[C.c_void_p,C.c_void_p];cls.name.restype=C.c_bool
    def password(self): return secrets.token_urlsafe(18)
    def test_c_python_password_contract(self):
        for length in range(71):
            value='x'*length
            self.assertEqual(self.valid(value.encode(),length),config.validate_password(value))
        for char in ('\x00','\x1f','\x7f','ą','"','\\',' '):
            value='x'*8+char; data=value.encode()
            self.assertEqual(self.valid(data,len(data)),config.validate_password(value))
    def test_mac_name_and_bounds(self):
        for value in range(256):
            mac=(C.c_uint8*6)(0,1,2,value,255,0);out=(C.c_char*18)(*([b'!']*18))
            self.assertTrue(self.name(mac,C.cast(C.byref(out,1),C.c_void_p)))
            self.assertEqual(bytes(out)[1:17],f'StreamOO-{value:02X}FF00'.encode()+b'\0')
            self.assertEqual(bytes(out)[0:1],b'!');self.assertEqual(bytes(out)[17:18],b'!')
        self.assertFalse(self.name(None,None))
    def test_private_roundtrip_permissions(self):
        with tempfile.TemporaryDirectory() as d:
            path=Path(d)/'private/network.json'; password=self.password()
            config.save_private(path,password)
            self.assertEqual(config.load_password(path),password)
            self.assertEqual(stat.S_IMODE(path.stat().st_mode),0o600)
            self.assertEqual(list(path.parent.iterdir()),[path])
    def test_secret_path_in_repository_rejected(self):
        with self.assertRaises(ValueError): config.external_path(ROOT/'secrets/network.json')
    def test_generated_header_is_valid_c_without_output(self):
        with tempfile.TemporaryDirectory() as d:
            base=Path(d);path=base/'private.json';header=base/'network_secret.h';password=self.password()
            config.save_private(path,password)
            result=subprocess.run(['python3',str(ROOT/'tools/configure_network.py'),'--file',str(path),
                '--generate-header',str(header)],capture_output=True,text=True)
            self.assertEqual(result.returncode,0);self.assertEqual(result.stdout+result.stderr,'')
            self.assertEqual(stat.S_IMODE(header.stat().st_mode),0o600)
            (base/'test.c').write_text('#include "network_secret.h"\nint main(void) { return STREAMOO_NETWORK_SECRET_AVAILABLE && sizeof streamoo_ap_password>8 ? 0 : 1; }\n')
            subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror',str(base/'test.c'),'-o',str(base/'test')],check=True)
            subprocess.run([str(base/'test')],check=True)
    def test_missing_default_header_disables_radio(self):
        with tempfile.TemporaryDirectory() as d:
            header=Path(d)/'network_secret.h';config.generate_header(Path(d)/'missing.json',header)
            self.assertIn('AVAILABLE 0',header.read_text())
            self.assertNotIn('ap_password',header.read_text())
    def test_explicit_missing_file_is_error(self):
        with tempfile.TemporaryDirectory() as d:
            with self.assertRaises(OSError):config.generate_header(Path(d)/'missing.json',Path(d)/'header.h',True)
    def test_bad_json_error_is_redacted(self):
        with tempfile.TemporaryDirectory() as d:
            secret=self.password();path=Path(d)/'bad.json';path.write_text(secret)
            result=subprocess.run(['python3',str(ROOT/'tools/configure_network.py'),'--file',str(path),'--check'],capture_output=True,text=True)
            self.assertNotEqual(result.returncode,0);self.assertNotIn(secret,result.stdout+result.stderr)
            self.assertNotIn('Traceback',result.stderr)
    def test_bad_schema_is_rejected(self):
        with tempfile.TemporaryDirectory() as d:
            path=Path(d)/'bad.json'
            for value in ([],{}, {'version':2,'ap_password':self.password()}, {'version':1,'ap_password':None}):
                path.write_text(json.dumps(value))
                with self.assertRaises(ValueError):config.load_password(path)
    def test_generated_secret_cannot_target_tracked_source(self):
        with tempfile.TemporaryDirectory() as d:
            path=Path(d)/'private.json';config.save_private(path,self.password())
            with self.assertRaises(ValueError):config.generate_header(path,ROOT/'components/network/unwanted.h')
    def test_check_does_not_echo_password(self):
        with tempfile.TemporaryDirectory() as d:
            path=Path(d)/'private.json';password=self.password();config.save_private(path,password)
            result=subprocess.run(['python3',str(ROOT/'tools/configure_network.py'),'--file',str(path),'--check'],capture_output=True,text=True)
            self.assertEqual(result.returncode,0);self.assertNotIn(password,result.stdout+result.stderr)
