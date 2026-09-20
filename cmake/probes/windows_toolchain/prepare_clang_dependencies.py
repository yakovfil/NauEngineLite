"""Prepare the recorded USD/TBB and Google Test headers for clang-cl 22.

No dependency binaries, version checks, or compiler diagnostics are changed.
Original files are backed up once; reruns accept the exact patched form only.
"""
import argparse
from pathlib import Path


USD_ASSIGNMENT = '''        template <class U>
        _ValueProxy& operator=(const U& x)
        {
            _owner->_Set(*_pos, x);
            return *this;
        }
'''
USD_REPLACEMENT = '''        // clang-cl: upstream USD removed this uninstantiable assignment.
#if !defined(__clang__)
''' + USD_ASSIGNMENT + '''#endif
'''
TBB_ENUM = '    enum kind_type {'
TBB_REPLACEMENT = '''    // Match the Windows producer's 32-bit signed enum representation while
    // permitting TBB's existing internal state values (2 through 4).
#if defined(__clang__) && defined(_WIN32)
    enum kind_type : int {
#else
    enum kind_type {
#endif'''
GTEST_CAST = '''inline void PrintTo(char8_t c, ::std::ostream* os) {
  PrintTo(ImplicitCast_<char32_t>(c), os);
}'''
GTEST_REPLACEMENT = GTEST_CAST.replace('ImplicitCast_<char32_t>', 'static_cast<char32_t>')


def patched_bytes(path, old, new):
    original = path.read_bytes()
    newline = b'\r\n' if b'\r\n' in original else b'\n'
    old_bytes = old.encode().replace(b'\n', newline)
    new_bytes = new.encode().replace(b'\n', newline)
    if original.count(new_bytes) == 1:
        return original, original
    if original.count(old_bytes) != 1:
        raise ValueError(f'Unrecognized header; left unchanged: {path}')
    return original, original.replace(old_bytes, new_bytes)


def prepare(usd_root=None, googletest_root=None):
    patches = []
    if usd_root:
        patches.extend([
            (usd_root / 'include/pxr/usd/sdf/childrenProxy.h', USD_ASSIGNMENT, USD_REPLACEMENT),
            (usd_root / 'include/tbb/task.h', TBB_ENUM, TBB_REPLACEMENT),
        ])
    if googletest_root:
        patches.append((googletest_root / 'googletest/include/gtest/gtest-printers.h', GTEST_CAST, GTEST_REPLACEMENT))
    # Validate every input before writing any header.
    updates = [(path, *patched_bytes(path, old, new)) for path, old, new in patches]
    for path, original, patched in updates:
        if original == patched:
            print(f'Already prepared: {path}')
            continue
        backup = path.with_suffix(path.suffix + '.nau-clang.bak')
        if backup.exists() and backup.read_bytes() != original:
            raise ValueError(f'Backup differs from original: {backup}')
    for path, original, patched in updates:
        if original != patched:
            path.with_suffix(path.suffix + '.nau-clang.bak').write_bytes(original)
            path.write_bytes(patched)
            print(f'Prepared: {path}')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--usd-root', type=Path)
    parser.add_argument('--googletest-root', type=Path)
    args = parser.parse_args()
    if not args.usd_root and not args.googletest_root:
        parser.error('Specify at least one dependency root')
    prepare(args.usd_root, args.googletest_root)
