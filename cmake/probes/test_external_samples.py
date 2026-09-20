"""Check explicit sample selection and missing-source behavior without a compiler."""
from pathlib import Path
import subprocess
import tempfile

ENGINE = Path(__file__).resolve().parents[2]
MODULE = ENGINE / 'cmake/NauSamples.cmake'


def main():
    with tempfile.TemporaryDirectory(prefix='nau samples ') as temporary:
        root = Path(temporary)
        samples = root / 'custom samples'
        for relative in ('CMakeLists.txt', 'minimalApp/CMakeLists.txt',
                         'minimalApp/app_sample_main_min.cpp',
                         'sample_common/CMakeLists.txt', 'sceneBase/CMakeLists.txt'):
            path = samples / relative
            path.parent.mkdir(parents=True, exist_ok=True)
            path.touch()
        def check(profile, enabled, selected, expected, demo=False):
            script = root / 'check.cmake'
            script.write_text(f'set(NAU_RUNTIME_PROFILE {profile})\n'
                              f'set(NAU_CORE_SAMPLES {enabled})\n'
                              f'set(NAU_DEMO_SOURCE_DIR "{"demo" if demo else ""}")\n'
                              f'set(NAU_SAMPLES_SOURCE_DIR "{selected.as_posix()}")\n'
                              f'include("{MODULE.as_posix()}")\n', encoding='utf-8')
            result = subprocess.run(['cmake', '-P', str(script)], capture_output=True, text=True)
            assert (result.returncode == 0) == expected, result.stdout + result.stderr
            if not expected:
                assert 'Missing required NauSamples input' in result.stderr
                assert 'git submodule update --init NauSamples' in ' '.join(result.stderr.split())
        for profile in ('desktop', 'minimal'):
            check(profile, 'ON', samples, True)
            check(profile, 'ON', root / 'absent', False)
            check(profile, 'OFF', root / 'absent', True)
        check('desktop', 'OFF', root / 'absent', False, demo=True)
        check('demo', 'ON', root / 'absent', True, demo=True)
        (samples / 'minimalApp/app_sample_main_min.cpp').unlink()
        check('minimal', 'ON', samples, False)
        print('PASS external sample path, spaces, missing inputs and optional profiles')


if __name__ == '__main__':
    main()
