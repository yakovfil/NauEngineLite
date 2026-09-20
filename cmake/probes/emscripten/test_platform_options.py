"""Check preset isolation and generated options; does not validate the Nau runtime."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess


engine = Path(__file__).resolve().parents[3]
build = engine / "build"
build.mkdir(exist_ok=True)


def run(command, log, env=None, expected=0):
    result = subprocess.run(command, cwd=engine, env=env, capture_output=True, text=True, timeout=180)
    output = result.stdout + result.stderr
    (build / log).write_text(output, encoding="utf-8")
    assert result.returncode == expected, output
    return output


def cache(path):
    result = {}
    for line in path.read_text(encoding="utf-8").splitlines():
        if line and not line.startswith(("#", "//")) and "=" in line:
            key, value = line.split("=", 1)
            result[key.split(":", 1)[0]] = value
    return result


parser = argparse.ArgumentParser()
parser.add_argument("--native-before", type=Path)
parser.add_argument("--native-after", type=Path)
args = parser.parse_args()
presets = json.loads((engine / "CMakePresets.json").read_text())
baseline = json.loads(subprocess.check_output(
    ["git", "show", "HEAD:CMakePresets.json"], cwd=engine, text=True))
for group in ("configurePresets", "buildPresets"):
    native = lambda entries: [p for p in entries if not p["name"].startswith(("web-", "web_", "native-minimal-", "native_minimal_"))]
    assert native(presets[group]) == native(baseline[group]), "Desktop presets changed"
print("PASS desktop preset definitions unchanged")

native_hash = None
if args.native_before:
    native_hash = hashlib.sha256((args.native_before / "CMakeCache.txt").read_bytes()).hexdigest()

browser_env = dict(os.environ, VCPKG_ROOT=str(build / "unavailable-vcpkg"),
                   CMAKE_PREFIX_PATH=str(build / "unavailable-desktop-dependencies"))
for configuration in ("Debug", "Release"):
    preset = f"web-minimal-{configuration.lower()}"
    output = run(["cmake", "--preset", preset], f"{preset}-configure.log", browser_env)
    assert "Build files have been written" in output
    assert "Configure for Emscripten/(wasm32)" in output
    assert "Running vcpkg install" not in output
    values = cache(build / preset / "CMakeCache.txt")
    assert values["CMAKE_GENERATOR"] == "Ninja"
    assert values["CMAKE_BUILD_TYPE"] == configuration
    assert values["NAU_RUNTIME_PROFILE"] == "minimal"
    assert values["EMSCRIPTEN_SYSTEM_PROCESSOR"] == "wasm32"
    assert values.get("CMAKE_SYSTEM_VERSION") != "10.0"
    assert "vcpkg" not in values["CMAKE_TOOLCHAIN_FILE"].lower()
    for output_type, directory in (("RUNTIME", "bin"), ("LIBRARY", "lib"), ("ARCHIVE", "lib")):
        assert Path(values[f"CMAKE_{output_type}_OUTPUT_DIRECTORY"]) == build / preset / directory
    assert any(p["name"] == preset and p["configurePreset"] == preset for p in presets["buildPresets"])
    print(f"PASS {preset} configures the production minimal graph independently")

    # The production platform/options helpers compile the standalone probe and a
    # real bundled C library. No mock Nau framework is introduced here.
    output = run(["powershell.exe", "-NoProfile", "-ExecutionPolicy", "Bypass", "-File",
                  str(engine / "cmake/check_emscripten.ps1"), "-Probe", "-Configuration", configuration],
                 f"platform-{configuration.lower()}-build.log", browser_env)
    probe = build / f"emscripten-probe-{configuration.lower()}"
    platform = (probe / "platform.txt").read_text()
    assert "target=wasm32" in platform and "emscripten=ON" in platform
    assert all(f"{flag}=OFF" in platform for flag in ("windows", "win32", "win64"))
    commands = json.loads((probe / "compile_commands.json").read_text())
    assert any(p["file"].endswith("xxhash.c") for p in commands)
    assert any(p["file"].endswith("probe.cpp") for p in commands)
    forbidden = ("-D_WIN32", "-DWIN32", "-D_WIN64", "-D_M_IX86", "-D_M_X64", "-D_TARGET_PC",
                 "WIN32_LEAN_AND_MEAN", "NOMINMAX", "-mavx", "-mfma", " /GR", " /EH",
                 " /MD", " /MT", " /Zc:", " /wd", " /nologo", " -WX")
    for entry in commands:
        command = entry["command"]
        assert "-pthread" in command and not any(flag in command for flag in forbidden), command
        if entry["file"].endswith(".cpp"):
            assert "-std=c++20" in command and "-fno-rtti" in command
        if entry["file"].endswith(".c"):
            assert "-fno-rtti" not in command and "-std=c++20" not in command
        assert ("-O0" if configuration == "Debug" else "-O3") in command
    generated = run(["ninja", "-C", str(probe), "-t", "commands"], f"platform-{configuration.lower()}-commands.log")
    link = next(line for line in generated.splitlines() if " -o nau_emscripten_probe.js" in line)
    assert "-pthread" in link and "-sPROXY_TO_PTHREAD=1" in link and "xxHash" in link
    if configuration == "Debug":
        assert "-sASSERTIONS=2" in link and "-g3" in link
    assert (probe / "nau_emscripten_probe.wasm").is_file()
    print(f"PASS {configuration} C/C++ compile and application link options")

if args.native_before and args.native_after:
    before = cache(args.native_before / "CMakeCache.txt")
    after = cache(args.native_after / "CMakeCache.txt")
    keys = [key for key in before if key.startswith(("CMAKE_C_", "CMAKE_CXX_")) or key in (
        "CMAKE_GENERATOR", "CMAKE_GENERATOR_PLATFORM", "CMAKE_GENERATOR_TOOLSET", "CMAKE_GENERATOR_INSTANCE",
        "CMAKE_TOOLCHAIN_FILE", "CMAKE_SYSTEM_VERSION", "CMAKE_CONFIGURATION_TYPES", "BUILD_SHARED_LIBS",
        "VCPKG_TARGET_TRIPLET", "VCPKG_HOST_TRIPLET", "NAU_CORE_TESTS", "NAU_CORE_TOOLS", "NAU_CORE_SAMPLES")]
    for key in keys:
        assert before[key] == after.get(key), (key, before[key], after.get(key))
    assert all("EMSCRIPTEN" not in key for key in after), "Browser SDK leaked into native cache"
    assert hashlib.sha256((args.native_before / "CMakeCache.txt").read_bytes()).hexdigest() == native_hash
    print(f"PASS {len(keys)} native compiler/cache settings unchanged; original cache untouched")
