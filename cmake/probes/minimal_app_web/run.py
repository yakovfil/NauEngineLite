"""Accept the actual preset-built sample; never configure a substitute application."""
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


def accept(page, context, scenario, build):
    def wait(expression, timeout=15000):
        page.wait_for_function(expression, timeout=timeout, polling=10)

    wait('Module.nauRuntime?.status || window.result')
    metrics = {}
    if scenario != 'missing-host':
        wait("Module.nauRuntime?.status?.state === 'Running' && Module.nauRuntime.status.completedSteps >= 2 || window.result")
        assert page.evaluate("Module.nauRuntime.status.state === 'Running'"), page.evaluate('window.result')
        if scenario in ('success', 'runner-failure'):
            before = page.evaluate('({n:Module.nauRuntime.status.completedSteps,t:performance.now(),beats})')
            page.wait_for_timeout(5000)
            after = page.evaluate('({n:Module.nauRuntime.status.completedSteps,t:performance.now(),beats})')
            metrics['rate'] = (after['n'] - before['n']) * 1000 / (after['t'] - before['t'])
            metrics['foregroundHeartbeats'] = after['beats'] - before['beats']
            assert 8 <= metrics['rate'] <= 12, metrics
            assert metrics['foregroundHeartbeats'] >= 100, metrics
            wait("Number(document.getElementById('count').textContent) >= " + str(after['n']))
            assert page.locator('#status').inner_text() == 'Running'
            page.screenshot(path=str(build / 'sample-running.png'))
            other = context.new_page()
            other.bring_to_front()
            wait('document.hidden')
            page.wait_for_timeout(400)
            page.bring_to_front()
            wait('!document.hidden')
            current = page.evaluate('Module.nauRuntime.status.completedSteps')
            wait(f'Module.nauRuntime.status.completedSteps > {current + 2}')
            metrics['realVisibility'] = True
            # A copied UI snapshot is enough to exercise repeated Stop during cleanup.
            page.evaluate('''() => {
                const bridge = Module.nauRuntime, deliver = bridge.deliver;
                window.cleanupStopRequests = 0;
                bridge.deliver = function(snapshot) {
                    const accepted = deliver.call(this, snapshot);
                    if (accepted && this.status.state === 'Stopping' && !window.cleanupStopRequests) {
                        this.stop(); this.stop(); window.cleanupStopRequests = 2;
                    }
                    return accepted;
                };
            }''')
            page.locator('#stop').click()
        else:
            page.evaluate("document.getElementById('nau-window').remove()")
    wait('window.result !== null', 30000)
    result = page.evaluate('window.result')
    result.update(metrics)
    assert not result.get('abortReason'), result
    failed = scenario in ('missing-host', 'host-loss')
    assert result['code'] == int(failed), result
    status = result['status']
    assert status['state'] == ('Failed' if failed else 'Stopped'), result
    assert status['cleanupComplete'] and status['applicationWorker'], result
    assert result['isolated'] and result['hosts'] == 0, result
    history = result['history']
    assert all(a['sequence'] < b['sequence'] and a['completedSteps'] <= b['completedSteps']
               for a, b in zip(history, history[1:])), history
    terminal = next(i for i, s in enumerate(history) if s['state'] in ('Stopping', 'Failed', 'Stopped'))
    assert len({s['completedSteps'] for s in history[terminal:]}) == 1, history
    updates = [b for a, b in zip(history, history[1:]) if b['completedSteps'] > a['completedSteps']]
    assert all(b['completedSteps'] - a['completedSteps'] == 1 for a, b in zip(history, history[1:])
               if b['completedSteps'] > a['completedSteps']), 'Batched game counts'
    assert all(b['timestamp'] - a['timestamp'] >= 50 for a, b in zip(updates, updates[1:])), 'Catch-up burst'
    if failed:
        assert status['message'], status
        assert all(s['state'] == 'Failed' for s in history[terminal:]), history
        if scenario == 'missing-host':
            assert status['completedSteps'] == 0 and not any(s['state'] == 'Running' for s in history)
            assert status['phase'] in ('Pre-initialization', 'Initialization'), status
        else:
            assert status['phase'] == 'Running' and status['completedSteps'] >= 2, status
    else:
        result['cleanupStopRequests'] = page.evaluate('window.cleanupStopRequests')
        assert result['cleanupStopRequests'] == 2, result
    assert page.locator('#status').inner_text() == status['state']
    assert int(page.locator('#count').inner_text()) == status['completedSteps']
    retry = page.evaluate('Module.nauRuntime.start()')
    assert not retry['accepted'] and 'reload' in retry['message'].lower(), retry
    page.evaluate('Module.nauRuntime.stop(); Module.nauRuntime.stop()')
    page.wait_for_timeout(100)
    assert page.evaluate('Module.nauRuntime.status') == status, 'Final snapshot changed'
    if scenario == 'success':
        page.screenshot(path=str(build / 'sample-stopped.png'))
    return result


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--build', required=True, type=pathlib.Path)
    parser.add_argument('--scenario', choices=['success', 'missing-host', 'host-loss', 'runner-failure'], default='success')
    parser.add_argument('--chrome', default='C:/Program Files/Google/Chrome/Application/chrome.exe')
    args = parser.parse_args()
    build = args.build.resolve()
    result = {'scenario': args.scenario, 'build': str(build), 'runnerCode': 1}
    logs = []
    server = None
    try:
        bin_dir = build / 'bin'
        assert (bin_dir / 'MinimalAppSample.js').is_file(), 'Build the real MinimalAppSample first'
        (bin_dir / 'sample-acceptance.html').write_text(pathlib.Path(__file__).with_name('host.html').read_text(), encoding='utf-8')
        server = http.server.ThreadingHTTPServer(('127.0.0.1', 0), functools.partial(Handler, directory=str(bin_dir)))
        thread = threading.Thread(target=server.serve_forever, daemon=True)
        thread.start()
        with tempfile.TemporaryDirectory(prefix='nau-sample-chrome-') as profile:
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
                    try:
                        page.on('console', lambda message: logs.append(message.text))
                        page.on('pageerror', lambda error: logs.append(str(error)))
                        page.goto(f'http://127.0.0.1:{server.server_port}/sample-acceptance.html?scenario={args.scenario}')
                        result.update(accept(page, context, args.scenario, build))
                        result['browser'] = browser.version
                        assert args.scenario != 'runner-failure', 'Deliberately violated acceptance expectation'
                        result['runnerCode'] = 0
                    finally:
                        context.new_cdp_session(page).send('Browser.close')
            finally:
                try:
                    process.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    process.terminate()
                    process.wait(timeout=5)
    except Exception as error:
        result['error'] = str(error)
        result['runnerCode'] = 1
    finally:
        if server:
            server.shutdown()
            server.server_close()
            thread.join()
    result['logs'] = logs
    print(json.dumps(result, indent=2))
    return result['runnerCode']


if __name__ == '__main__':
    raise SystemExit(main())
