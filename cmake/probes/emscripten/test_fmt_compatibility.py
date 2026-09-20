"""Build bundled fmt and retain compile-time format checks with the pinned SDK."""
import os
from pathlib import Path
import shutil
import subprocess
import tempfile


engine = Path(__file__).resolve().parents[3]
build = engine / "build"
build.mkdir(exist_ok=True)
results = Path(tempfile.mkdtemp(prefix="fmt-regression-", dir=build))
compiler = shutil.which("em++")
assert compiler, "Load emsdk_env.ps1 before running this test"
node = os.environ["EMSDK_NODE"]
include = engine / "engine/3rdparty_libs/fmt/include"


def run(name, command, success=True):
    result = subprocess.run(command, cwd=engine, capture_output=True, text=True, timeout=180)
    output = result.stdout + result.stderr
    (results / f"{name}.log").write_text(output, encoding="utf-8")
    assert (result.returncode == 0) == success, output
    return output


valid = results / "valid.cpp"
valid.write_text('''#include <fmt/format.h>
int main()
{
    return fmt::format(FMT_STRING("value={}"), 42) == "value=42" ? 0 : 1;
}
''', encoding="utf-8")
invalid = results / "invalid.cpp"
invalid.write_text('''#include <fmt/format.h>
int main()
{
    auto value = fmt::format(FMT_STRING("{:d}"), "not an integer");
}
''', encoding="utf-8")

for configuration in ("Debug", "Release"):
    run(f"{configuration}-build", ["powershell.exe", "-NoProfile", "-ExecutionPolicy", "Bypass",
        "-File", str(engine / "cmake/check_emscripten.ps1"), "-Probe", "-WithFmt",
        "-Configuration", configuration])
    probe = build / f"emscripten-probe-{configuration.lower()}-fmt"
    run(f"{configuration}-thread-runtime", [node, str(probe / "nau_emscripten_probe.js")])
    flags = ["-std=c++20", "-pthread", "-fno-exceptions", "-fno-rtti", "-I", str(include),
             "-O0" if configuration == "Debug" else "-O3"]
    archive = probe / "fmt" / ("libfmtd.a" if configuration == "Debug" else "libfmt.a")
    output = results / f"valid-{configuration}.js"
    run(f"{configuration}-valid", [compiler, *flags, str(valid), str(archive),
        "-sPROXY_TO_PTHREAD=1", "-sEXIT_RUNTIME=1", "-o", str(output)])
    run(f"{configuration}-format-runtime", [node, str(output)])
    diagnostic = run(f"{configuration}-invalid", [compiler, *flags, "-c", str(invalid),
        "-o", str(results / f"invalid-{configuration}.o")], success=False)
    assert "invalid format specifier" in diagnostic, diagnostic
    print(f"PASS {configuration}: bundled build, threaded runtime, FMT_STRING formatting, invalid-format rejection")
print(f"Logs: {results}")
