"""Disposable staging and copied-server contract tests; standard library only."""
import argparse
import hashlib
import http.client
import json
import os
import pathlib
import shutil
import socket
import subprocess
import sys
import tempfile
import unittest
import urllib.parse

ENGINE = pathlib.Path(__file__).resolve().parents[3]
ASSETS = ENGINE / 'samples/minimalApp/web'


class PackageTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix='nau-package-contract-')
        self.root = pathlib.Path(self.temporary.name).resolve()
        self.assertFalse(self.root.is_relative_to(ENGINE.parent))
        self.inputs = self.root / 'inputs'
        self.inputs.mkdir()
        for name in ('MinimalAppSample.js', 'MinimalAppSample.wasm'):
            (self.inputs / name).write_text('disposable runtime input')

    def tearDown(self):
        self.temporary.cleanup()

    def stage(self, destination=None):
        return subprocess.run(['cmake', f'-DBUILD_ROOT={self.root.as_posix()}',
            f'-DDESTINATION={(destination or self.root / "package").as_posix()}',
            f'-DLOADER={(self.inputs / "MinimalAppSample.js").as_posix()}',
            f'-DWASM={(self.inputs / "MinimalAppSample.wasm").as_posix()}',
            f'-DASSETS={ASSETS.as_posix()}', '-DCONFIGURATION=Debug',
            '-P', str(ASSETS / 'stage.cmake')], capture_output=True, text=True)

    def test_staging_inventory_failure_and_boundaries(self):
        first = self.stage()
        self.assertEqual(first.returncode, 0, first.stderr)
        package = self.root / 'package'
        manifest = json.loads((package / 'manifest.json').read_text())
        for name, digest in manifest['files'].items():
            self.assertEqual(hashlib.sha256((package / name).read_bytes()).hexdigest(), digest)
        (package / 'obsolete.worker.js').write_text('obsolete')
        unrelated = self.root / 'unrelated.txt'
        unrelated.write_text('preserve')
        self.assertEqual(self.stage().returncode, 0)
        self.assertEqual(set(p.name for p in package.iterdir()), set(manifest['files']) | {'manifest.json'})
        self.assertEqual(json.loads((package / 'manifest.json').read_text()), manifest)
        for destination in (self.root, self.root.parent / 'outside-package', ASSETS):
            failure = self.stage(destination)
            self.assertNotEqual(failure.returncode, 0)
            self.assertIn('Unsafe package destination', failure.stderr)
        (self.inputs / 'MinimalAppSample.wasm').unlink()
        failure = self.stage()
        self.assertNotEqual(failure.returncode, 0)
        self.assertIn('Missing required package input', failure.stderr)
        self.assertEqual(json.loads((package / 'manifest.json').read_text()), manifest)
        (self.inputs / 'MinimalAppSample.wasm').write_text('disposable runtime input')
        (self.root / 'package.previous').mkdir()
        failure = self.stage()
        self.assertNotEqual(failure.returncode, 0)
        self.assertIn('Staging replacement blocked', failure.stderr)
        self.assertEqual(json.loads((package / 'manifest.json').read_text()), manifest)
        self.assertEqual(unrelated.read_text(), 'preserve')

    def test_copied_server(self):
        package = self.root / 'package'
        shutil.copytree(ASSETS, package)
        (package / 'MinimalAppSample.wasm').write_bytes(b'wasm MIME probe')
        outside = self.root / 'outside'
        outside.mkdir()
        (outside / 'secret.txt').write_text('must not be served')
        link = package / 'escape'
        if os.name == 'nt':
            junction = subprocess.run(['cmd.exe', '/d', '/c', 'mklink', '/J', str(link), str(outside)],
                                      capture_output=True, text=True)
            self.assertEqual(junction.returncode, 0, junction.stderr)
        else:
            link.symlink_to(outside, target_is_directory=True)
        environment = {key: value for key, value in os.environ.items()
                       if key.upper() in ('SYSTEMROOT', 'WINDIR', 'TEMP', 'TMP', 'COMSPEC', 'USERPROFILE')}
        for base in ('/', '/relocated/nau/'):
            process = subprocess.Popen([sys.executable, str(package / 'serve.py'), '--port', '0', '--base-path', base],
                cwd=self.root, env=environment, stdout=subprocess.PIPE, stderr=subprocess.DEVNULL, text=True)
            try:
                url = urllib.parse.urlsplit(process.stdout.readline().strip())
                self.assertEqual(url.path, base)
                connection = http.client.HTTPConnection(url.hostname, url.port, timeout=5)
                for path, expected in ((base, 200), (base + 'MinimalAppSample.wasm', 200),
                                       (base + '%2e%2e/secret', 403), (base + '%5csecret', 403),
                                       (base + 'escape/secret.txt', 403),
                                       (base + 'missing', 404)):
                    connection.request('GET', path)
                    response = connection.getresponse()
                    response.read()
                    self.assertEqual(response.status, expected, path)
                    self.assertEqual(response.headers['Cross-Origin-Opener-Policy'], 'same-origin')
                    self.assertEqual(response.headers['Cross-Origin-Embedder-Policy'], 'require-corp')
                    self.assertEqual(response.headers['Cache-Control'], 'no-store')
                    if path.endswith('.wasm'):
                        self.assertEqual(response.headers['Content-Type'], 'application/wasm')
                if base != '/':
                    connection.request('GET', '/index.html')
                    response = connection.getresponse()
                    response.read()
                    self.assertEqual(response.status, 404)
                conflict = subprocess.run([sys.executable, str(package / 'serve.py'), '--port', str(url.port)],
                    cwd=self.root, env=environment, capture_output=True, text=True, timeout=5)
                self.assertEqual(conflict.returncode, 1)
                self.assertIn('Choose another --port', conflict.stderr)
                connection.close()
            finally:
                process.terminate()
                process.wait(timeout=5)
                process.stdout.close()


if __name__ == '__main__':
    unittest.main()
