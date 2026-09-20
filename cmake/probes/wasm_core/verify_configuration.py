"""Exercise isolated SDK and C++20 coroutine rejection without altering SDK state."""
import os
from pathlib import Path
import subprocess
import tempfile

engine = Path(__file__).resolve().parents[3]
sdk = Path(os.environ["EMSDK"])
with tempfile.TemporaryDirectory(prefix="core-negative-", dir=engine / "build") as folder:
    scratch = Path(folder).resolve()
    assert scratch.is_relative_to((engine / "build").resolve())
    env = dict(os.environ)
    env.pop("EMSDK", None)
    result = subprocess.run([
        "cmake", "-S", str(engine / "cmake/probes/wasm_core"), "-B", str(scratch / "missing-sdk"),
        "-G", "Ninja", f"-DCMAKE_TOOLCHAIN_FILE={sdk.as_posix()}/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake",
    ], env=env, capture_output=True, text=True, timeout=30)
    assert result.returncode and "EMSDK is missing" in result.stderr, result.stderr
    result = subprocess.run([
        str(sdk / "upstream/emscripten/em++.exe"), "-std=c++17", "-fsyntax-only", "-x", "c++",
        str(engine / "engine/core/kernel/include/nau/async/cpp_coroutine.h"),
    ], capture_output=True, text=True, timeout=30)
    assert result.returncode and "Nau requires C++20 coroutine support" in result.stderr, result.stderr
print("PASS isolated missing-SDK and unsupported-coroutine diagnostics")
