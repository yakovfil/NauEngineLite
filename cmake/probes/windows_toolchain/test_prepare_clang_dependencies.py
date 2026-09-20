import tempfile
import unittest
from pathlib import Path

from prepare_clang_dependencies import (
    prepare, USD_ASSIGNMENT, USD_REPLACEMENT, TBB_ENUM, TBB_REPLACEMENT,
    GTEST_CAST, GTEST_REPLACEMENT,
)


class PreparationTests(unittest.TestCase):
    def test_preparation_preserves_newlines_and_original_backups(self):
        for newline in ('\n', '\r\n'):
            with self.subTest(newline=newline), tempfile.TemporaryDirectory() as directory:
                root = Path(directory)
                files = [
                    ('include/pxr/usd/sdf/childrenProxy.h', USD_ASSIGNMENT, USD_REPLACEMENT),
                    ('include/tbb/task.h', TBB_ENUM, TBB_REPLACEMENT),
                    ('googletest/include/gtest/gtest-printers.h', GTEST_CAST, GTEST_REPLACEMENT),
                ]
                for relative, old, new in files:
                    path = root / relative
                    path.parent.mkdir(parents=True, exist_ok=True)
                    path.write_bytes((old + '\n').replace('\n', newline).encode())
                prepare(root, root)
                prepare(root, root)
                for relative, old, new in files:
                    path = root / relative
                    self.assertEqual(path.read_bytes(), (new + '\n').replace('\n', newline).encode())
                    self.assertEqual(path.with_suffix('.h.nau-clang.bak').read_bytes(), (old + '\n').replace('\n', newline).encode())

    def test_unknown_second_header_does_not_modify_first(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            usd = root / 'include/pxr/usd/sdf/childrenProxy.h'
            usd.parent.mkdir(parents=True)
            usd.write_text(USD_ASSIGNMENT, encoding='utf-8')
            tbb = root / 'include/tbb/task.h'
            tbb.parent.mkdir(parents=True)
            tbb.write_text('unknown layout', encoding='utf-8')
            before = usd.read_bytes()
            with self.assertRaises(ValueError):
                prepare(root)
            self.assertEqual(usd.read_bytes(), before)
            self.assertFalse(usd.with_suffix('.h.nau-clang.bak').exists())


if __name__ == '__main__':
    unittest.main()
