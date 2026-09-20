"""Configure and audit the CPU-only stage of the demo profile; no runtime claim."""
import json
import os
from pathlib import Path
import subprocess
import tempfile
import shutil


def run(command, log):
    result = subprocess.run(command, capture_output=True, text=True, timeout=180)
    log.write_text(result.stdout + result.stderr, encoding="utf-8")
    if result.returncode:
        raise RuntimeError(f"Command failed; see {log}")


def main():
    engine = Path(__file__).resolve().parents[3]
    sdk = Path(os.environ["EMSDK"])
    manifest = json.loads((engine / "cmake/naudemo_sources.json").read_text())
    evidence = engine / "build/demo-profile-evidence"
    evidence.mkdir(parents=True, exist_ok=True)
    summaries = {}
    for platform in ("web", "native"):
        for config in ("Debug", "Release"):
            build = engine / f"build/{platform}-demo-cpu-{config.lower()}"
            query = build / ".cmake/api/v1/query/codemodel-v2"
            query.parent.mkdir(parents=True, exist_ok=True)
            query.touch()
            command = ["cmake", "-S", str(engine), "-B", str(build),
                       "-DNAU_RUNTIME_PROFILE=demo", "-DBUILD_SHARED_LIBS=OFF"]
            if platform == "web":
                command += ["-G", "Ninja", f"-DCMAKE_BUILD_TYPE={config}",
                            f"-DCMAKE_TOOLCHAIN_FILE={sdk}/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake"]
            else:
                command += ["-G", "Visual Studio 17 2022", "-A", "x64"]
            run(command, evidence / f"{platform}-{config}.log")
            reply = build / ".cmake/api/v1/reply"
            index = json.loads(max(reply.glob("index-*.json"), key=lambda p: p.stat().st_mtime).read_text())
            model = json.loads((reply / index["reply"]["codemodel-v2"]["jsonFile"]).read_text())
            configuration = next(c for c in model["configurations"] if c["name"] == config)
            targets = {t["name"]: json.loads((reply / t["jsonFile"]).read_text()) for t in configuration["targets"]}
            assert {"CoreScene", "CoreAssets", "NauFrameworkDemo", "NauKernel", "PlatformApp"} <= targets.keys()
            assert not set(manifest["excluded_targets"]) & targets.keys()
            assert "NauFrameworkMinimal" not in targets and "MinimalAppSample" not in targets
            for target in targets.values():
                directory = target["paths"]["source"].replace("\\", "/")
                if "3rdparty_libs/" in directory:
                    assert directory.split("3rdparty_libs/", 1)[1].split("/")[0] in manifest["dependencies"], directory
                if platform == "native":
                    flags = json.dumps(target.get("compileGroups", []))
                    assert "-pthread" not in flags and "EMSCRIPTEN" not in flags, flags
            source_groups = dict(manifest["sources"])
            if platform == "web":
                source_groups.update(manifest["browser_targets"])
            else:
                assert not set(manifest["browser_targets"]) & targets.keys()
            for name, sources in source_groups.items():
                actual = {s["path"].replace("\\", "/") for s in targets[name]["sources"] if s["path"].endswith(".cpp")}
                expected = set(sources)
                actual = {p.removeprefix(engine.as_posix() + "/") for p in actual}
                assert actual == expected, (name, actual ^ expected)
            summaries[f"{platform}-{config}"] = sorted(targets)
            print(f"PASS {platform} {config}: explicit sources and dependency graph", flush=True)

    # Exercise source diagnostics in a disposable source tree, never remove real dependencies.
    fixture = Path(tempfile.mkdtemp(prefix="demo-missing-source-", dir=engine / "build"))
    (fixture / "cmake").mkdir()
    for name in ("NauDemoRuntime.cmake", "NauMinimalRuntime.cmake", "naudemo_sources.json"):
        shutil.copy2(engine / "cmake" / name, fixture / "cmake" / name)
    for name in manifest["dependencies"]:
        path = fixture / "engine/3rdparty_libs" / name / "CMakeLists.txt"
        path.parent.mkdir(parents=True)
        path.touch()
    result = subprocess.run(["cmake", "-P", str(fixture / "cmake/NauDemoRuntime.cmake")], capture_output=True, text=True)
    assert result.returncode != 0 and "Missing demo runtime source:" in result.stderr, result.stderr
    (evidence / "missing-source.log").write_text(result.stdout + result.stderr, encoding="utf-8")
    before = subprocess.check_output(["git", "show", "HEAD:CMakePresets.json"], cwd=engine)
    previous = json.loads(before)
    current = json.loads((engine / "CMakePresets.json").read_text())
    for group in ("configurePresets", "buildPresets", "testPresets"):
        by_name = {p["name"]: p for p in current.get(group, [])}
        assert all(by_name[p["name"]] == p for p in previous.get(group, []))
    (evidence / "results.json").write_text(json.dumps(summaries, indent=2), encoding="utf-8")
    print("PASS missing-source diagnostics and unchanged preset definitions")


if __name__ == "__main__":
    main()
