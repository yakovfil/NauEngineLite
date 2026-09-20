"""Acceptance of copied delivery assets and server, never a replacement page."""
import argparse
import functools
import hashlib
import http.server
import importlib.util
import json
import os
import pathlib
import shutil
import subprocess
import sys
import tempfile
import threading
import time
import urllib.request
import urllib.parse
from playwright.sync_api import sync_playwright

HERE = pathlib.Path(__file__).resolve().parent
WORKSPACE = HERE.parents[3]
spec = importlib.util.spec_from_file_location('sample_acceptance', HERE.parent / 'minimal_app_web/run.py')
sample = importlib.util.module_from_spec(spec)
spec.loader.exec_module(sample)

OBSERVE = '''() => {
    window.result = null;
    window.beats = 0;
    setInterval(() => ++window.beats, 10);
    let module;
    Object.defineProperty(window, 'Module', {configurable: true,
        get() { return module; },
        set(value) {
            module = value;
            if (value.deliveryObserved) return;
            value.deliveryObserved = true;
            const exit = value.onExit;
            value.onExit = function(code) {
                exit?.(code);
                window.result = {code, status: value.nauRuntime?.status,
                    history: value.nauRuntime?.history, beats: window.beats,
                    isolated: crossOriginIsolated,
                    hosts: value.nauPlatformHosts?.size || 0};
            };
        }
    });
}'''


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--build', type=pathlib.Path, required=True)
    parser.add_argument('--scenario', default='success', choices=[
        'success', 'host-loss', 'missing-headers', 'file-url', 'missing-loader',
        'missing-wasm', 'abort-hook', 'server-startup', 'runner-failure'])
    parser.add_argument('--chrome', default='C:/Program Files/Google/Chrome/Application/chrome.exe')
    args = parser.parse_args()
    build = args.build.resolve()
    evidence = build / 'delivery-evidence'
    evidence.mkdir(exist_ok=True)
    result = {'scenario': args.scenario, 'runnerCode': 1, 'network': [], 'workers': [], 'logs': []}
    server_process = server = chrome = None
    try:
        with tempfile.TemporaryDirectory(prefix='nau-delivery-') as temporary:
            external = pathlib.Path(temporary).resolve()
            assert not external.is_relative_to(WORKSPACE), 'Copy must be outside the entire workspace'
            package = external / 'package'
            shutil.copytree(build / 'package', package)
            result['copy'] = str(package)
            manifest = json.loads((package / 'manifest.json').read_text())
            assert set(p.name for p in package.iterdir()) == set(manifest['files']) | {'manifest.json'}
            for name, digest in manifest['files'].items():
                assert hashlib.sha256((package / name).read_bytes()).hexdigest() == digest, name
            result['manifest'] = manifest
            if args.scenario in ('missing-loader', 'missing-wasm'):
                (package / ('MinimalAppSample.js' if args.scenario == 'missing-loader' else 'MinimalAppSample.wasm')).unlink()
            environment = {key: value for key, value in os.environ.items()
                           if key.upper() in ('SYSTEMROOT', 'WINDIR', 'TEMP', 'TMP', 'COMSPEC', 'USERPROFILE')}
            result['serverEnvironmentKeys'] = sorted(environment)
            if args.scenario == 'missing-headers':
                server = http.server.ThreadingHTTPServer(('127.0.0.1', 0),
                    functools.partial(http.server.SimpleHTTPRequestHandler, directory=str(external)))
                threading.Thread(target=server.serve_forever, daemon=True).start()
                url = f'http://127.0.0.1:{server.server_port}/package/'
            elif args.scenario == 'file-url':
                url = (package / 'index.html').as_uri()
            else:
                server_process = subprocess.Popen([sys.executable, str(package / 'serve.py'),
                    '--port', '0', '--base-path', '/relocated/nau/'], cwd=external, env=environment,
                    stdout=subprocess.PIPE, stderr=subprocess.DEVNULL, text=True)
                url = server_process.stdout.readline().strip()
                assert url.startswith('http://127.0.0.1:'), 'Packaged server did not print a launch URL'
                with urllib.request.urlopen(url) as response:
                    assert response.headers['Cross-Origin-Opener-Policy'] == 'same-origin'
                    assert response.headers['Cross-Origin-Embedder-Policy'] == 'require-corp'
                if args.scenario == 'server-startup':
                    conflict = subprocess.run([sys.executable, str(package / 'serve.py'), '--port',
                        str(urllib.parse.urlsplit(url).port)], cwd=external, env=environment,
                        capture_output=True, text=True, timeout=5)
                    assert conflict.returncode == 1 and 'Choose another --port' in conflict.stderr, conflict
                    result['diagnostic'] = conflict.stderr.strip()
            result['url'] = url
            profile = external / 'chrome'
            chrome = subprocess.Popen([args.chrome, '--headless=new', f'--user-data-dir={profile}',
                '--remote-debugging-port=0', '--no-first-run', '--no-default-browser-check', 'about:blank'],
                stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            port_file = profile / 'DevToolsActivePort'
            deadline = time.monotonic() + 15
            while not port_file.exists():
                assert chrome.poll() is None and time.monotonic() < deadline, 'Chrome debugging startup failed'
                time.sleep(.05)
            with sync_playwright() as playwright:
                browser = playwright.chromium.connect_over_cdp('http://127.0.0.1:' + port_file.read_text().splitlines()[0], no_defaults=True)
                context = browser.contexts[0]
                page = context.pages[0]
                try:
                    result['browser'] = browser.version
                    context.on('request', lambda request: result['network'].append({'event': 'request', 'url': request.url, 'type': request.resource_type}))
                    context.on('response', lambda response: result['network'].append({'event': 'response', 'url': response.url, 'status': response.status, 'contentType': response.headers.get('content-type')}))
                    page.on('worker', lambda worker: result['workers'].append({'url': worker.url}))
                    page.on('console', lambda message: result['logs'].append(message.text))
                    page.on('pageerror', lambda error: result['logs'].append(str(error)))
                    page.add_init_script('(' + OBSERVE + ')()')
                    page.goto(url)
                    if args.scenario in ('success', 'host-loss', 'runner-failure', 'server-startup'):
                        result.update(sample.accept(page, context, 'success' if args.scenario == 'server-startup' else args.scenario, evidence))
                        assert result['workers'], 'No worker startup observed'
                        loader_requests = [item for item in result['network'] if item['event'] == 'request' and item['url'].endswith('/MinimalAppSample.js')]
                        assert len(loader_requests) >= len(result['workers']) + 1, 'Worker loader requests were not captured'
                        assert any(item['event'] == 'response' and item['url'].endswith('.wasm') and item['status'] == 200 for item in result['network']), 'Missing successful Wasm response'
                    elif args.scenario == 'abort-hook':
                        page.wait_for_function("Module.nauRuntime?.status.state === 'Running'")
                        page.evaluate("Module.onAbort('Controlled page-hook simulation')")
                        page.wait_for_timeout(200)
                        assert page.locator('#status').inner_text() == 'Failed'
                        assert 'Controlled page-hook simulation' in page.locator('#message').inner_text()
                        assert page.locator('#cleanup').inner_text() == 'Not confirmed'
                        result['simulation'] = 'Page hook only; not a real Wasm fatal abort'
                    else:
                        page.wait_for_function("document.getElementById('status').textContent === 'Failed'")
                        page.wait_for_timeout(300)
                        diagnostic = page.locator('#message').inner_text()
                        expected = {'missing-headers': 'cross-origin isolated', 'file-url': 'Local file URLs',
                                    'missing-loader': 'Could not load MinimalAppSample.js', 'missing-wasm': 'aborted'}[args.scenario]
                        assert expected in diagnostic, diagnostic
                        assert page.locator('#stop').is_disabled()
                        assert page.locator('#cleanup').inner_text() == 'Not confirmed'
                        assert not page.evaluate("Module.nauRuntime?.history.some(s => ['Running','Stopped'].includes(s.state))")
                        result['diagnostic'] = diagnostic
                        if args.scenario in ('missing-headers', 'file-url'):
                            assert not result['workers']
                            assert not any(item['url'].endswith(('.js', '.wasm')) for item in result['network'])
                    if args.scenario != 'file-url':
                        assert all(item['url'].startswith(url) for item in result['network'] if item['url'].startswith(('http:', 'https:'))), result['network']
                    assert all(worker['url'].startswith((url, 'blob:', 'data:')) for worker in result['workers'])
                    page.screenshot(path=str(evidence / (args.scenario + '.png')), full_page=True)
                    assert args.scenario != 'runner-failure', 'Deliberately violated acceptance expectation'
                    result['runnerCode'] = 0
                finally:
                    context.new_cdp_session(page).send('Browser.close')
                    chrome.wait(timeout=10)
                    if server_process:
                        server_process.terminate()
                        server_process.wait(timeout=5)
                    if server:
                        server.shutdown()
                        server.server_close()
    except Exception as error:
        result['error'] = str(error)
    finally:
        for process in (chrome, server_process):
            if process and process.poll() is None:
                process.terminate()
                process.wait(timeout=5)
        if server:
            server.server_close()
    (evidence / (args.scenario + '.json')).write_text(json.dumps(result, indent=2), encoding='utf-8')
    print(json.dumps({key: value for key, value in result.items() if key not in ('network', 'history', 'logs', 'manifest')}, indent=2))
    return result['runnerCode']


if __name__ == '__main__':
    raise SystemExit(main())
