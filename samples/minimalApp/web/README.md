# Nau minimal web application

Copy this entire folder anywhere. Serving needs Python 3.9 or newer and current
Chrome; it needs no engine checkout, Emscripten SDK, CMake, Ninja or Playwright.

From this folder run `python serve.py --port 8000`, then open the printed URL.
From another directory use `python C:/path/to/package/serve.py --port 8000`.
If the port is occupied, select another, for example `--port 8001`.
For a nested URL use `--base-path /relocated/nau/`. Stop the server with Ctrl+C.

This loopback development server supplies COOP `same-origin`, COEP `require-corp`
and the WebAssembly MIME type. Opening index.html as a file, or using an ordinary
server without isolation headers, cannot run this threaded application. Use the
packaged server and its printed localhost URL. This utility is not a production
hosting or TLS configuration.

The non-rendering sample shows real worker lifecycle status and completed updates.
Stop requests asynchronous shutdown; wait for cleanup Complete and Stopped.
Failed retains its diagnostic; cleanup is only confirmed when the runtime reports
it. Reload the page to retry after stopping or failure. There is no in-page restart
or single-threaded fallback. Missing loader/Wasm errors require restoring the
complete package before reload.

manifest.json identifies the configuration, SDK and SHA-256 of every delivered
file except the manifest itself. SDK 6.0.9 embeds worker support in the loader;
there is no separate required worker script. Keep Debug and Release packages
separate. Browser automation dependencies are needed only for repository tests.
