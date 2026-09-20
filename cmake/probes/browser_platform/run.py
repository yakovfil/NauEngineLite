"""Run production platform tests in an isolated, local Chrome fixture."""
import argparse
import functools
import http.server
import json
import pathlib
import subprocess
import tempfile
import threading
import time
from playwright.sync_api import sync_playwright

class Handler(http.server.SimpleHTTPRequestHandler):
    def end_headers(self):
        self.send_header('Cross-Origin-Opener-Policy', 'same-origin')
        self.send_header('Cross-Origin-Embedder-Policy', 'require-corp')
        super().end_headers()
    def log_message(self, *args):
        pass

parser = argparse.ArgumentParser()
parser.add_argument('--build', required=True, type=pathlib.Path)
parser.add_argument('--scenario', default='valid')
parser.add_argument('--dpr', type=float, default=1)
parser.add_argument('--chrome', default='C:/Program Files/Google/Chrome/Application/chrome.exe')
args = parser.parse_args()
bin_dir = args.build.resolve() / 'bin'
(bin_dir / 'index.html').write_text(pathlib.Path(__file__).with_name('fixture.html').read_text(), encoding='utf-8')
server = http.server.ThreadingHTTPServer(('127.0.0.1', 0), functools.partial(Handler, directory=str(bin_dir)))
thread = threading.Thread(target=server.serve_forever, daemon=True)
thread.start()
logs = []
result = {'code':1}
try:
    with tempfile.TemporaryDirectory(prefix='nau-platform-chrome-') as profile:
        process = subprocess.Popen([args.chrome, '--headless=new', f'--user-data-dir={profile}',
            '--remote-debugging-port=0', '--no-first-run', '--no-default-browser-check', 'about:blank'],
            stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        try:
            port_file = pathlib.Path(profile) / 'DevToolsActivePort'
            deadline = time.monotonic() + 15
            while not port_file.exists():
                if process.poll() is not None or time.monotonic() >= deadline:
                    raise RuntimeError('Chrome did not expose its local debugging port')
                time.sleep(.05)
            with sync_playwright() as playwright:
                # Playwright's default focus emulation keeps background pages visible.
                # Use Chrome's real visibility transitions for this acceptance test.
                browser = playwright.chromium.connect_over_cdp('http://127.0.0.1:' + port_file.read_text().splitlines()[0], no_defaults=True)
                context = browser.contexts[0]
                page = context.pages[0]
                session = context.new_cdp_session(page)
                session.send('Emulation.setDeviceMetricsOverride', {'width':800, 'height':600, 'deviceScaleFactor':args.dpr, 'mobile':False})
                page.on('console', lambda message: logs.append(message.text))
                page.on('pageerror', lambda error: logs.append(str(error)))
                page.goto(f'http://127.0.0.1:{server.server_port}/index.html?scenario={args.scenario}')
                if args.scenario in ('valid', 'hostloss', 'fixture-failure'):
                    page.wait_for_function('Module.visibilityReady || window.result', timeout=60000)
                    other = context.new_page()
                    other.bring_to_front()
                    page.wait_for_function('Module.visibilitySeen || window.result', timeout=15000, polling=50)
                    page.bring_to_front()
                page.wait_for_function('window.result !== null', timeout=60000)
                result = page.evaluate('window.result')
                result.update(scenario=args.scenario, dpr=page.evaluate('devicePixelRatio'), browser=browser.version)
                if not result.get('isolated') or result.get('hosts') != 0 or result.get('contextRequests') != 0:
                    result['code'] = 1
                if args.scenario == 'valid' and not result.get('hostConnected'):
                    result['code'] = 1
                session.send('Browser.close')
        finally:
            try: process.wait(timeout=10)
            except subprocess.TimeoutExpired:
                process.terminate()
                process.wait(timeout=10)
except Exception as error:
    result['code'] = 1
    result['error'] = str(error)
finally:
    server.shutdown()
    server.server_close()
    thread.join()
result['logs'] = logs
print(json.dumps(result, indent=2))
raise SystemExit(result['code'])
