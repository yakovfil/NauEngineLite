"""Exercise native compiler guards without requiring every rejected compiler to be installed."""
from pathlib import Path
import subprocess
import tempfile
import unittest


class ToolchainTests(unittest.TestCase):
    def check(self, overrides, accepted, diagnostic=''):
        guard = Path(__file__).resolve().parents[2] / 'platformSpec/NauWindowsToolchain.cmake'
        values = dict(WIN32='TRUE', CMAKE_CXX_COMPILER_ID='MSVC', MSVC_VERSION='1951',
                      CMAKE_CXX_COMPILER_VERSION='19.51', CMAKE_SIZEOF_VOID_P='8',
                      CMAKE_GENERATOR='Visual Studio 18 2026', CMAKE_GENERATOR_PLATFORM='x64',
                      CMAKE_VS_PLATFORM_TOOLSET='v145', CMAKE_CXX_COMPILER_ARCHITECTURE_ID='x64')
        values.update(overrides)
        with tempfile.TemporaryDirectory() as directory:
            script = Path(directory) / 'check.cmake'
            script.write_text('\n'.join(f'set({key} "{value}")' for key, value in values.items())
                              + f'\ninclude("{guard.as_posix()}")\n')
            result = subprocess.run(['cmake', '-P', str(script)], capture_output=True, text=True)
        self.assertEqual(result.returncode == 0, accepted, result.stdout + result.stderr)
        if diagnostic:
            self.assertIn(diagnostic, result.stderr)

    def test_supported_vs_and_ninja(self):
        self.check({}, True)
        self.check(dict(CMAKE_GENERATOR='Ninja', CMAKE_VS_PLATFORM_TOOLSET=''), True)

    def test_obsolete_inputs(self):
        for values in (dict(CMAKE_GENERATOR='Visual Studio 17 2022'),
                       dict(CMAKE_VS_PLATFORM_TOOLSET='v143'), dict(MSVC_VERSION='1944'),
                       dict(CMAKE_GENERATOR='Ninja', MSVC_VERSION='1944'),
                       dict(CMAKE_VERSION='4.1'), dict(CMAKE_SIZEOF_VOID_P='4'),
                       dict(CMAKE_CXX_COMPILER_ARCHITECTURE_ID='ARM64')):
            with self.subTest(values=values):
                self.check(values, False, 'Nau requires Visual Studio 2026')

    def test_clang_cl_is_not_msvc_version(self):
        self.check(dict(CMAKE_GENERATOR='Ninja', CMAKE_CXX_COMPILER_ID='Clang', MSVC_VERSION='1944'), True)

    def test_browser_and_other_targets_bypass_windows_checks(self):
        self.check(dict(EMSCRIPTEN='TRUE', CMAKE_VERSION='3.24', MSVC_VERSION='0'), True)
        self.check(dict(WIN32='FALSE', MSVC_VERSION='0'), True)


if __name__ == '__main__':
    unittest.main()
