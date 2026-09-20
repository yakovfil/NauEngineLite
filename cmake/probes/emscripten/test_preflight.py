"""Exercise preflight failures without modifying or replacing the installed SDK."""
import hashlib
import os
from pathlib import Path
import shutil
import subprocess
import tempfile


engine = Path(__file__).resolve().parents[3]
build = engine / "build"
build.mkdir(exist_ok=True)
fixtures = Path(tempfile.mkdtemp(prefix="emscripten-contract-", dir=build))
sdk = Path(os.environ["EMSDK"])
toolchain = sdk / "upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake"
config = sdk / ".emscripten"
before = hashlib.sha256(config.read_bytes()).hexdigest()
shell = ["powershell.exe", "-NoProfile", "-ExecutionPolicy", "Bypass", "-File"]


def check(name, command, success, diagnostic, env=None):
    result = subprocess.run(command, env=env, capture_output=True, text=True, timeout=90)
    output = result.stdout + result.stderr
    (fixtures / f"{name}.log").write_text(output, encoding="utf-8")
    assert (result.returncode == 0) == success, output
    assert diagnostic in output, output
    print(f"PASS {name}")


preflight = engine / "cmake/check_emscripten.ps1"
expected = (engine / "cmake/emscripten-sdk-version.txt").read_text().strip()
check("matching-sdk", [*shell, str(preflight)], True, "SDK preflight passed")
missing_env = dict(os.environ)
missing_env.pop("EMSDK", None)
check("missing-sdk", [*shell, str(preflight)], False, "EMSDK is missing", missing_env)
missing_files = dict(os.environ, EMSDK=str(fixtures / "absent-sdk"))
check("missing-files", [*shell, str(preflight)], False, "Missing SDK file", missing_files)

# Change the expected contract in a disposable fixture, leaving the real SDK intact.
for name in ("check_emscripten.ps1", "NauEmscriptenSdk.cmake"):
    shutil.copyfile(engine / "cmake" / name, fixtures / name)
(fixtures / "emscripten-sdk-version.txt").write_text("0.0.0\n", encoding="utf-8")
check("mismatched-sdk", [*shell, str(fixtures / preflight.name)], False, f"Detected emcc {expected}")
check("cmake-mismatched-sdk", ["cmake", f"-DCMAKE_TOOLCHAIN_FILE={toolchain}",
      "-P", str(fixtures / "NauEmscriptenSdk.cmake")], False, "Incompatible emcc")
check("cmake-direct-missing-sdk", ["cmake", "-S", str(engine), "-B",
      str(fixtures / "direct-missing"), "-DNAU_BUILD_BROWSER=ON"], False,
      "EMSDK is missing", missing_env)
check("cmake-native-cache", ["cmake", f"-DCMAKE_TOOLCHAIN_FILE={toolchain}",
      "-DCMAKE_CXX_COMPILER=cl.exe", "-P", str(engine / "cmake/NauEmscriptenSdk.cmake")],
      False, "Native or incompatible compiler cache")
assert hashlib.sha256(config.read_bytes()).hexdigest() == before, "SDK activation changed"
print(f"SDK activation unchanged. Logs: {fixtures}")
