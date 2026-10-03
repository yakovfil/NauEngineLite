"""Launch relocated SDK samples and close their primary window with bounded shutdown."""
import argparse
import ctypes
from ctypes import wintypes
import json
from pathlib import Path
import subprocess
import time


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sdk', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--consumer', type=Path, help='Generated StdSdkConsumer project root to check as well')
    parser.add_argument('--configuration', choices=['Debug', 'Release', 'Both'], default='Both')
    parser.add_argument('--repetitions', type=int, default=3)
    parser.add_argument('--close-delay', type=float, default=5)
    parser.add_argument('--close-secondary-first', action='store_true',
                        help='Close inputDemo secondary window before requesting application shutdown')
    args = parser.parse_args()
    args.sdk = args.sdk.resolve()
    args.output.mkdir(parents=True, exist_ok=True)
    user32 = ctypes.WinDLL('user32', use_last_error=True)
    callback_type = ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)
    user32.EnumWindows.argtypes = [callback_type, wintypes.LPARAM]
    user32.GetWindowThreadProcessId.argtypes = [wintypes.HWND, ctypes.POINTER(wintypes.DWORD)]
    user32.GetWindowTextW.argtypes = [wintypes.HWND, wintypes.LPWSTR, ctypes.c_int]
    user32.GetWindowRect.argtypes = [wintypes.HWND, ctypes.POINTER(wintypes.RECT)]
    user32.PostMessageW.argtypes = [wintypes.HWND, wintypes.UINT, wintypes.WPARAM, wintypes.LPARAM]

    def windows(pid):
        result = []

        @callback_type
        def visit(handle, _):
            owner = wintypes.DWORD()
            user32.GetWindowThreadProcessId(handle, ctypes.byref(owner))
            if owner.value == pid:
                title = ctypes.create_unicode_buffer(256)
                user32.GetWindowTextW(handle, title, len(title))
                if title.value == 'NAU':
                    rect = wintypes.RECT()
                    user32.GetWindowRect(handle, ctypes.byref(rect))
                    result.append(dict(handle=handle, title=title.value, x=rect.left, y=rect.top))
            return True

        user32.EnumWindows(visit, 0)
        return result

    results = []
    configs = ('Debug', 'Release') if args.configuration == 'Both' else (args.configuration,)
    for config in configs:
        samples = ('SceneBaseSample', 'inputDemo', 'StdSdkConsumerMain') if args.consumer else ('SceneBaseSample', 'inputDemo')
        for sample in samples:
            for repeat in range(args.repetitions):
                entry = dict(configuration=config, sample=sample, repetition=repeat, passed=False)
                process = None
                try:
                    root = args.consumer.resolve() if sample == 'StdSdkConsumerMain' else args.sdk
                    binary = root / 'bin' / config / (sample + '.exe')
                    entry['binary'] = str(binary)
                    entry['workingDirectory'] = str(root)
                    with (args.output / f'{config}-{sample}-{repeat}.log').open('w', encoding='utf-8') as log:
                        process = subprocess.Popen([str(binary)], cwd=root, stdout=log, stderr=subprocess.STDOUT)
                        time.sleep(args.close_delay)
                        assert process.poll() is None, 'Sample exited before the close request'
                        entry['windows'] = windows(process.pid)
                        if args.close_secondary_first and sample == 'inputDemo':
                            secondary = next(window for window in entry['windows'] if window['x'] == 210 and window['y'] == 10)
                            assert user32.PostMessageW(secondary['handle'], 0x10, 0, 0), 'Secondary WM_CLOSE could not be posted'
                            deadline = time.monotonic() + 5
                            while any(window['handle'] == secondary['handle'] for window in windows(process.pid)):
                                assert time.monotonic() < deadline, 'Secondary window did not close'
                                assert process.poll() is None, 'Closing the secondary window stopped the application'
                                time.sleep(0.05)
                            entry['secondaryClosedBeforeShutdown'] = True
                        # Fresh samples create the primary window at (10, 10); inputDemo
                        # creates its secondary, non-quitting window 200 pixels to the right.
                        primary = next(window for window in entry['windows'] if window['x'] == 10 and window['y'] == 10)
                        assert user32.PostMessageW(primary['handle'], 0x10, 0, 0), 'WM_CLOSE could not be posted'
                        entry['exitCode'] = process.wait(timeout=20)
                        assert entry['exitCode'] == 0, f"Abnormal shutdown: {entry['exitCode']:#x}"
                        entry['passed'] = True
                except Exception as error:
                    entry['error'] = str(error)
                finally:
                    if process is not None and process.poll() is None:
                        process.terminate()
                        process.wait(timeout=5)
                        entry['forcedTermination'] = True
                results.append(entry)
                (args.output / 'results.json').write_text(json.dumps(results, indent=2), encoding='utf-8')
                print(json.dumps(entry), flush=True)
    return 0 if results and all(entry['passed'] for entry in results) else 1


if __name__ == '__main__':
    raise SystemExit(main())
