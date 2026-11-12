import io
from pathlib import Path
import struct
import subprocess
import tarfile
import tempfile
import unittest
from unittest.mock import patch

from package import check


class PackageTests(unittest.TestCase):
    def setUp(self):
        scratch = Path(__file__).resolve().parents[1] / 'tmp/package-tests'
        scratch.mkdir(parents=True, exist_ok=True)
        self.temporary = tempfile.TemporaryDirectory(dir=scratch)
        self.addCleanup(self.temporary.cleanup)
        self.archive = Path(self.temporary.name) / 'package.tgz'

    def package(self, qt=6, *, extra=None, mode=0o755, hard_float=True, ui=False):
        binary = bytearray(52)
        binary[:6] = b'\x7fELF\x01\x01'
        struct.pack_into('<H', binary, 18, 40)
        struct.pack_into('<I', binary, 36, 0x400 if hard_float else 0)
        if ui:
            binary.extend(b'_nh_settings_hook')
        library = ('usr/local/Kobo/' if qt == 5 else '') + 'imageformats/libnickelhome.so'
        files = [(library, bytes(binary), mode)]
        if qt == 5:
            files.extend((f'mnt/onboard/.adds/nickel-home/{name}', b'resource', 0o644)
                         for name in ('doc', 'default', 'uninstall'))
        if extra:
            files.append((extra, b'extra', 0o644))
        with tarfile.open(self.archive, 'w:gz') as archive:
            for name, data, permissions in files:
                entry = tarfile.TarInfo(name)
                entry.size = len(data)
                entry.mode = permissions
                archive.addfile(entry, io.BytesIO(data))

    def validate(self, qt=6, linked_qt=None):
        linked_qt = linked_qt or qt
        output = '\n'.join(f'(NEEDED) Shared library: [libQt{linked_qt}{name}.so.{linked_qt}]'
                           for name in ('Core', 'Gui', 'Widgets'))
        with patch('package.subprocess.run', return_value=subprocess.CompletedProcess([], 0, output)):
            check(self.archive, qt, 'readelf')

    def test_each_firmware_has_its_own_paths_and_dependencies(self):
        for qt in (5, 6):
            with self.subTest(qt=qt):
                self.package(qt)
                self.validate(qt)
                with self.assertRaisesRegex(ValueError, 'wrong Qt'):
                    self.validate(qt, 11-qt)
                with self.assertRaisesRegex(ValueError, 'unexpected package paths'):
                    self.validate(11-qt)

    def test_qt6_rejects_system_partition_resources(self):
        self.package(extra='mnt/onboard/.adds/nickel-home/default')
        with self.assertRaisesRegex(ValueError, 'unexpected package paths'):
            self.validate()

    def test_duplicate_plugin_is_rejected(self):
        self.package(extra='imageformats/libnickelhome.so')
        with self.assertRaisesRegex(ValueError, 'unexpected package paths'):
            self.validate()

    def test_wrong_abi_is_rejected(self):
        self.package(hard_float=False)
        with self.assertRaisesRegex(ValueError, 'hard-float'):
            self.validate()

    def test_interactive_settings_are_rejected(self):
        self.package(ui=True)
        with self.assertRaisesRegex(ValueError, 'interactive settings'):
            self.validate()

    def test_unsafe_or_nonexecutable_modes_are_rejected(self):
        for mode in (0o777, 0o644):
            with self.subTest(mode=mode):
                self.package(mode=mode)
                with self.assertRaises(ValueError):
                    self.validate()


if __name__ == '__main__':
    unittest.main()
