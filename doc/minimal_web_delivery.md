# Minimal web delivery

With external emsdk 6.0.9 activated, run from the engine directory:

```powershell
cmake --preset web-minimal-debug
cmake --build --preset web-minimal-debug
cmake --preset web-minimal-release
cmake --build --preset web-minimal-release
```

The default browser build includes `MinimalAppWebPackage`. Independent outputs
are `build/web-minimal-debug/package` and `build/web-minimal-release/package`.
Staging needs CMake, with no extra Python build dependency. It validates the
current loader/Wasm and authored files, stages a temporary sibling, generates
SHA-256 hashes and replaces only the owned package. Obsolete files disappear.
A failed replacement keeps the old manifest identifiable; inspect any retained
`package.staging` or `package.previous` before removing those owned leftovers
and retrying. Do not edit generated packages; edit `samples/minimalApp/web`.

Each package contains index.html, MinimalAppSample.js, MinimalAppSample.wasm,
serve.py, README.md and manifest.json. Worker support is embedded in the pinned
SDK loader. Debug information does not require external source or symbol fetches.

Copy the complete folder outside the checkout, then run:

```powershell
python C:/path/to/package/serve.py --port 8000 --base-path /relocated/nau/
```

Open the printed URL. Serving needs Python 3.9+ standard library and current
Chrome; it needs no SDK, CMake, Ninja, vcpkg or Playwright. Another port can be
selected if occupied. The server binds loopback, supplies COOP/COEP and Wasm
MIME, disables caching and bounds requests to the package. It is a local
development utility, not production hosting. Ctrl+C stops it.

The page shows real lifecycle state, completed updates and cleanup. Stop requests
asynchronous shutdown. Failed retains a diagnostic. Reload after either failure
or Stop; the page cannot recreate runtime services in place. A file URL or absent
isolation headers produces guidance before any runtime request. Missing JS/Wasm
produces a retained failure without claiming successful cleanup. Restore the
complete package and reload.

## Acceptance

Tests additionally require Python with Playwright and installed Chrome. Run:

```powershell
python cmake/probes/minimal_web_delivery/test_package.py -v
foreach ($configuration in @('debug', 'release')) {
    foreach ($scenario in @('success', 'host-loss', 'missing-headers', 'file-url',
                            'missing-loader', 'missing-wasm', 'abort-hook', 'server-startup', 'runner-failure')) {
        python cmake/probes/minimal_web_delivery/run.py --build "build/web-minimal-$configuration" --scenario $scenario
    }
}
```

All ordinary cases return zero. `runner-failure` must return one with exactly
`Deliberately violated acceptance expectation`, after application code zero;
another error does not count as an expected failure. `abort-hook` is explicitly
a page-hook simulation; real Wasm abort remains covered by the lifecycle suite.
The standard-library package test covers staging failures, stale inventories,
path rejection, server headers/MIME and occupied ports.

The runner copies outside the entire workspace, starts the copied server with
only basic OS environment variables and uses a nested URL. It records all page
and worker requests, requires successful Wasm loading and captures screenshots.
Success checks 8–12 Hz over five seconds, UI heartbeat, real hide/show, no catch-up
bursts, repeated Stop and stable final cleanup/reload behavior. JSON and PNG
evidence is under each build's `delivery-evidence/`; disposable copies are removed.
See the parent repository's
`openspec/changes/minimal-web-delivery-validation/validation.md` for measured
results, native regressions and source state. Fresh-checkout reproduction is
still a separate pending `emscripten-build` task.
