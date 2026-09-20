"""Pinned-SDK worker/canvas feasibility; deliberately not NauDemo acceptance."""
import functools
import argparse
import http.server
import json
from pathlib import Path
import shutil
import subprocess
import tempfile
import threading

from playwright.sync_api import sync_playwright


class Handler(http.server.SimpleHTTPRequestHandler):
    def end_headers(self):
        self.send_header("Cross-Origin-Opener-Policy", "same-origin")
        self.send_header("Cross-Origin-Embedder-Policy", "require-corp")
        super().end_headers()

    def log_message(self, *args):
        pass


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--blocking-worker", action="store_true", help="Match the current lifecycle's non-yielding C++ worker loop")
    parser.add_argument("--poll-loss", action="store_true", help="Detect context loss on its owner without waiting for browser event delivery")
    args = parser.parse_args()
    source = Path(__file__).resolve().parent
    engine = source.parents[2]
    compiler = shutil.which("em++")
    if not compiler:
        raise RuntimeError("Load emsdk_env.ps1 first")
    version = subprocess.check_output([compiler, "--version"], text=True)
    pin = (engine / "cmake/emscripten-sdk-version.txt").read_text().strip()
    if f" {pin} " not in version.splitlines()[0]:
        raise RuntimeError(f"Expected SDK {pin}: {version}")
    output = Path(tempfile.mkdtemp(prefix="webgl-worker-", dir=engine / "build"))
    print(f"Evidence: {output}", flush=True)
    results = {"compiler": version, "blockingWorker": args.blocking_worker, "pollLoss": args.poll_loss, "configurations": {}}
    try:
        for config, optimization in (("Debug", "-O0"), ("Release", "-O3")):
            directory = output / config
            directory.mkdir()
            command = [compiler, str(source / "webgl_worker.cpp"), "-std=c++20",
                       optimization, "-pthread", "-fno-exceptions", "-fno-rtti",
                       "-sPROXY_TO_PTHREAD=1", "-sPTHREAD_POOL_SIZE=2", "-sEXIT_RUNTIME=1",
                       "-sOFFSCREENCANVAS_SUPPORT=1", "-sOFFSCREENCANVASES_TO_PTHREAD=#canvas",
                       "-sMIN_WEBGL_VERSION=2", "-sMAX_WEBGL_VERSION=2",
                       "-sASSERTIONS=1", "-o", str(directory / "probe.js")]
            command.append("-DNAU_PROBE_BLOCKING_WORKER=1" if args.blocking_worker else "-sASYNCIFY=1")
            if args.poll_loss:
                command.append("-DNAU_PROBE_POLL_LOSS=1")
            if config == "Debug":
                command.append("-g")
            (directory / "command.json").write_text(json.dumps(command, indent=2))
            built = subprocess.run(command, capture_output=True, text=True, timeout=240)
            (directory / "build.log").write_text(built.stdout + built.stderr, encoding="utf-8")
            if built.returncode:
                raise RuntimeError(built.stdout + built.stderr)
            (directory / "index.html").write_text('''<!doctype html><meta charset="utf-8">
<title>Worker WebGL feasibility</title><canvas id="canvas" style="width:640px;height:360px"></canvas>
<script>
window.probeEvents = []; window.heartbeats = 0;
setInterval(() => ++window.heartbeats, 20);
var Module = {canvas: document.querySelector('#canvas'),
 onExit: code => window.exitCode = code,
 onAbort: reason => window.abortReason = String(reason)};
</script><script src="probe.js"></script>''', encoding="utf-8")
        server = http.server.ThreadingHTTPServer(("127.0.0.1", 0), functools.partial(Handler, directory=str(output)))
        thread = threading.Thread(target=server.serve_forever, daemon=True)
        thread.start()
        try:
            with sync_playwright() as playwright:
                browser = playwright.chromium.launch(channel="chrome", headless=False)
                results["browser"] = browser.version
                try:
                    for config in ("Debug", "Release"):
                        page = browser.new_page()
                        errors = []
                        page.on("pageerror", lambda error: errors.append(str(error)))
                        page.goto(f"http://127.0.0.1:{server.server_port}/{config}/")
                        page.wait_for_function("probeEvents.some(e => e.phase === 'draw' && e.width === 640)", timeout=20000)
                        page.locator("canvas").screenshot(path=str(output / config / "canvas.png"))
                        page.wait_for_function("window.exitCode !== undefined || window.abortReason !== undefined", timeout=20000)
                        result = page.evaluate("({events: probeEvents, exit: window.exitCode, abort: window.abortReason, heartbeats, connected: document.querySelector('canvas').isConnected, isolated: crossOriginIsolated})")
                        result["pageErrors"] = errors
                        results["configurations"][config] = result
                        assert result["exit"] == 0 and not result.get("abort"), result
                        assert not errors and result["isolated"] and result["connected"], result
                        assert result["heartbeats"] > 20, result
                        assert [e["phase"] for e in result["events"]] == ["worker-context", "draw", "draw", "context-lost", "draw-rejected-lost", "disposed"], result
                        assert [(e["width"], e["height"]) for e in result["events"] if e["phase"] == "draw"] == [(320, 180), (640, 360)], result
                        print(f"PASS {config}: worker WebGL2, pixels, resize, loss, disposal, page heartbeat", flush=True)
                        page.close()
                finally:
                    browser.close()
        finally:
            server.shutdown()
            server.server_close()
            thread.join()
    except Exception as error:
        results["failure"] = str(error)
        raise
    finally:
        (output / "results.json").write_text(json.dumps(results, indent=2), encoding="utf-8")


if __name__ == "__main__":
    main()
