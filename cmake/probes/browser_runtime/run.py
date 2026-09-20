"""Real Chrome acceptance for the worker-owned application lifecycle."""
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
parser.add_argument('--scenario', default='success')
parser.add_argument('--chrome', default='C:/Program Files/Google/Chrome/Application/chrome.exe')
args = parser.parse_args()
bin_dir = args.build.resolve() / 'bin'
(bin_dir / 'index.html').write_text(pathlib.Path(__file__).with_name('fixture.html').read_text(), encoding='utf-8')
server = http.server.ThreadingHTTPServer(('127.0.0.1', 0), functools.partial(Handler, directory=str(bin_dir)))
thread = threading.Thread(target=server.serve_forever, daemon=True)
thread.start()
logs = []
result = {'code': 1}
exit_code = 1
try:
    with tempfile.TemporaryDirectory(prefix='nau-runtime-chrome-') as profile:
        process = subprocess.Popen([args.chrome, '--headless=new', f'--user-data-dir={profile}',
            '--remote-debugging-port=0', '--no-first-run', '--no-default-browser-check', 'about:blank'],
            stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        try:
            port_file = pathlib.Path(profile) / 'DevToolsActivePort'
            deadline = time.monotonic() + 15
            while not port_file.exists():
                if process.poll() is not None or time.monotonic() >= deadline:
                    raise RuntimeError('Chrome did not expose its debugging port')
                time.sleep(.05)
            with sync_playwright() as playwright:
                browser = playwright.chromium.connect_over_cdp('http://127.0.0.1:' + port_file.read_text().splitlines()[0], no_defaults=True)
                context = browser.contexts[0]
                page = context.pages[0]
                page.on('console', lambda message: logs.append(message.text))
                page.on('pageerror', lambda error: logs.append(str(error)))
                page.goto(f'http://127.0.0.1:{server.server_port}/index.html?scenario={args.scenario}')
                def wait(expression, timeout=15000):
                    page.wait_for_function(expression, timeout=timeout, polling=20)
                wait('Module.nauRuntime?.status || window.result')
                rate = None
                visibility = None
                stop_time = None
                if args.scenario in ('success', 'fixture-failure', 'pending', 'shutdownerror', 'hostloss', 'stall'):
                    wait("Module.nauRuntime?.status?.state === 'Running' || window.result")
                    if args.scenario in ('success', 'fixture-failure'):
                        before = page.evaluate('({n:Module.nauRuntime.status.completedSteps,t:performance.now(),beats})')
                        page.wait_for_timeout(5000)
                        after = page.evaluate('({n:Module.nauRuntime.status.completedSteps,t:performance.now(),beats})')
                        rate = (after['n'] - before['n']) * 1000 / (after['t'] - before['t'])
                        assert 8 <= rate <= 12, rate
                        assert after['beats'] - before['beats'] >= 100, 'UI heartbeat stalled'
                        other = context.new_page()
                        other.bring_to_front()
                        wait('document.hidden')
                        page.wait_for_timeout(300)
                        page.bring_to_front()
                        wait('!document.hidden')
                        visibility = True
                        page.wait_for_timeout(400)
                        count = page.evaluate('Module.nauRuntime.status.completedSteps')
                        wait(f'Module.nauRuntime.status.completedSteps > {count}')
                    if args.scenario == 'hostloss':
                        page.evaluate("document.getElementById('nau-window').remove()")
                    else:
                        stop_time = page.evaluate('(() => {const t=performance.now(); Module.nauRuntime.stop(); Module.nauRuntime.stop(); return t;})()')
                    if args.scenario in ('pending', 'stall'):
                        wait("Module.nauRuntime.status.state === 'Stopping' || Module.nauRuntime.status.state === 'Failed'")
                        stopped_steps = page.evaluate('Module.nauRuntime.status.completedSteps')
                        if args.scenario == 'stall':
                            other = context.new_page()
                            other.bring_to_front()
                            wait('document.hidden')
                            page.wait_for_timeout(400)
                            assert page.evaluate("Module.nauRuntime.status.state !== 'Failed'"), 'Hidden time counted as stalled progress'
                            page.bring_to_front()
                            wait('!document.hidden')
                            wait("Module.nauRuntime.status.state === 'Failed'")
                        else:
                            page.wait_for_timeout(150)
                        assert not page.evaluate('Module.nauRuntime.status.cleanupComplete')
                        assert page.evaluate('Module.nauRuntime.status.completedSteps') == stopped_steps
                        page.evaluate('Module.releaseWork()')
                elif args.scenario == 'stopstartup':
                    wait("Module.nauRuntime.status.phase === 'Pre-initialization'")
                    page.evaluate('Module.nauRuntime.stop(); Module.nauRuntime.stop()')
                    page.wait_for_timeout(100)
                    assert not page.evaluate('Module.nauRuntime.status.cleanupComplete')
                    page.evaluate('Module.releaseWork()')
                elif args.scenario == 'batchfailure':
                    wait("Module.nauRuntime.status.state === 'Failed'")
                    assert not page.evaluate('Module.nauRuntime.status.cleanupComplete')
                    page.evaluate('Module.releaseWork()')
                wait('window.result !== null', 30000)
                result = page.evaluate('window.result')
                result.update(scenario=args.scenario, browser=browser.version, rate=rate, visibility=visibility)
                if args.scenario == 'abort':
                    assert result['code'] != 0 and result['status']['state'] == 'Failed'
                    assert not result['status']['cleanupComplete']
                else:
                    expected_failure = args.scenario in ('immediate','deferred','shutdownerror','configfail','setupfail','hostloss','stall','batchfailure')
                    assert result['code'] == int(expected_failure), result
                    assert result['isolated'] and result['hosts'] == 0, result
                    status = result['status']
                    assert status['state'] == ('Failed' if expected_failure else 'Stopped'), status
                    assert status['cleanupComplete'] and status['applicationWorker'], status
                    expected_phase = {'immediate':'Initialization', 'deferred':'Initialization',
                        'batchfailure':'Pre-initialization', 'configfail':'Creation',
                        'setupfail':'Creation', 'hostloss':'Running', 'stall':'Cleanup',
                        'shutdownerror':'Cleanup'}.get(args.scenario)
                    if expected_phase:
                        assert status['phase'] == expected_phase, status
                    expected_objects = 0 if args.scenario == 'configfail' else 1
                    assert result['stats']['disposed'] == expected_objects, result['stats']
                    assert result['stats']['destroyed'] == expected_objects, result['stats']
                    history = result['history']
                    if args.scenario in ('success', 'fixture-failure'):
                        stopping = next(s for s in history if s['state'] == 'Stopping')
                        result['stopLatencyMs'] = stopping['timestamp'] - stop_time
                        assert result['stopLatencyMs'] < 90, result['stopLatencyMs']
                        assert result['stats']['workCallbacks'] == 60
                    assert all(a['sequence'] < b['sequence'] and a['completedSteps'] <= b['completedSteps'] for a, b in zip(history, history[1:]))
                    failed_at = next((i for i,s in enumerate(history) if s['state'] == 'Failed'), None)
                    if failed_at is not None:
                        assert all(s['state'] == 'Failed' for s in history[failed_at:])
                    assert all(0 <= sample['dt'] <= 100 for sample in result['measurements'])
                    samples = result['measurements']
                    assert all(b['time'] - a['time'] >= 50 for a,b in zip(samples, samples[1:])), 'Catch-up update burst'
                    assert not page.evaluate('Module.nauRuntime.start().accepted')
                    assert not page.evaluate('Module.nauRuntime.deliver(Module.nauRuntime.history[0])'), 'Late snapshot accepted'
                    page.evaluate('Module.nauRuntime.stop()')
                    if args.scenario in ('immediate','deferred','configfail','setupfail','batchfailure','stopstartup'):
                        assert status['completedSteps'] == 0
                    if args.scenario == 'stopstartup':
                        assert result['stats']['initialized'] == 0
                    exit_code = 0
                context.new_cdp_session(page).send('Browser.close')
        finally:
            try:
                process.wait(timeout=10)
            except subprocess.TimeoutExpired:
                process.terminate()
                process.wait(timeout=10)
except Exception as error:
    result['error'] = str(error)
    exit_code = 1
finally:
    server.shutdown()
    server.server_close()
    thread.join()
result['logs'] = logs
result['runnerCode'] = exit_code
print(json.dumps(result, indent=2))
raise SystemExit(exit_code)
