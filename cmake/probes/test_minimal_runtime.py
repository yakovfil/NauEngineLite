"""Audit configured native profile graphs and minimal dependency diagnostics.

Create .cmake/api/v1/query/codemodel-v2 in each build tree before configuring.
Run after configuring native-minimal-debug/release and desktop-platform-after.
"""

import argparse
import json
from pathlib import Path
import shutil
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[2]


def targets(build):
    reply = build / ".cmake/api/v1/reply"
    index = json.loads(max(reply.glob("index-*.json"), key=lambda p: p.stat().st_mtime).read_text())
    model = json.loads((reply / index["reply"]["codemodel-v2"]["jsonFile"]).read_text())
    return {
        target["name"]: json.loads((reply / target["jsonFile"]).read_text())
        for target in model["configurations"][0]["targets"]
    }


def source_names(target):
    return {Path(source["path"]).name for source in target.get("sources", [])}


def minimal_graph(build):
    graph = targets(build)
    assert {"NauKernel", "NauFrameworkMinimal", "PlatformApp", "MinimalAppSample", "NauMinimalLifecycleTests"} <= graph.keys()
    assert not {"NauFramework", "CoreScene", "Graphics", "ui", "VFX", "ShaderCompilerTool"} & graph.keys()
    allowed_dependencies = {
        "EABase", "EASTL", "tiny-utf8", "fmt", "utfcpp", "ModifiedSonyMath",
        "jsoncpp", "wyhash", "fast_float", "googletest",
    }
    for target in graph.values():
        directory = target["paths"]["source"]
        if directory.startswith("engine/3rdparty_libs/"):
            assert directory.split("/")[2] in allowed_dependencies, directory
        elif directory.startswith("engine/core/"):
            assert directory.startswith(("engine/core/kernel", "engine/core/app_framework", "engine/core/modules/platform_app")), directory
        elif directory.startswith("samples/"):
            assert directory == "samples/minimalApp", directory
        else:
            assert directory == ".", directory

    sources = {name for name in source_names(graph["NauFrameworkMinimal"]) if name.endswith(".cpp")}
    assert sources == {
        "application_impl.cpp", "application_setup.cpp", "background_work_service.cpp",
        "global_properties_impl.cpp", "logging_service.cpp", "platform_window_service.cpp",
        "main_loop_service.cpp", "concurrent_execution_container.cpp",
    }, sources
    sample = source_names(graph["MinimalAppSample"])
    assert "app_sample_main_min.cpp" in sample and "app_sample_main.cpp" not in sample
    for name in ("MinimalAppSample", "NauMinimalLifecycleTests"):
        assert "generated_static_modules_initialization.cpp" in source_names(graph[name])
    for name in ("NauFrameworkMinimal", "PlatformApp"):
        groups = graph[name]["compileGroups"]
        assert any("NAU_MINIMAL_RUNTIME=1" in {d["define"] for d in g.get("defines", [])} for g in groups)
        assert all("-pthread" not in str(g) for g in groups)
    print(f"PASS minimal graph: {build.name} ({len(graph)} targets)")


def desktop_graph(build):
    graph = targets(build)
    assert {"NauFramework", "CoreScene", "Graphics", "ui", "VFX"} <= graph.keys()
    assert "NauFrameworkMinimal" not in graph and "NauMinimalLifecycleTests" not in graph
    sample = source_names(graph["MinimalAppSample"])
    assert "app_sample_main.cpp" in sample and "app_sample_main_min.cpp" not in sample
    for name in ("NauFramework", "PlatformApp"):
        assert "NAU_MINIMAL_RUNTIME" not in str(graph[name].get("compileGroups", []))
    print(f"PASS desktop graph: {build.name}")


def diagnostics():
    scratch_root = ROOT / "build"
    scratch_root.mkdir(exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="minimal-profile-check-", dir=scratch_root) as temp:
        scratch = Path(temp).resolve()
        assert scratch.is_relative_to(scratch_root.resolve())
        script = scratch / "check.cmake"
        profile = ROOT / "cmake/NauMinimalRuntime.cmake"
        script.write_text(f'set(NAU_MINIMAL_MODULES Rendering CACHE STRING "")\ninclude("{profile.as_posix()}")\n')
        result = subprocess.run(["cmake", "-P", str(script)], capture_output=True, text=True)
        assert result.returncode and "Unsupported minimal modules 'Rendering'" in result.stderr, result.stderr

        # A separate source fixture tests a missing recorded source without moving
        # or removing dependencies in the working checkout.
        fixture = scratch / "cmake"
        fixture.mkdir()
        shutil.copy2(profile, fixture / profile.name)
        for dependency in ("EABase", "EASTL", "tiny-utf8"):
            path = scratch / "engine/3rdparty_libs" / dependency
            path.mkdir(parents=True)
            (path / "CMakeLists.txt").touch()
        script.write_text(f'include("{(fixture / profile.name).as_posix()}")\n')
        result = subprocess.run(["cmake", "-P", str(script)], capture_output=True, text=True)
        assert result.returncode and "Missing minimal runtime dependency: fmt" in result.stderr, result.stderr
    print("PASS unsupported-module and missing-source diagnostics")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--minimal", type=Path, action="append", required=True)
    parser.add_argument("--desktop", type=Path, required=True)
    args = parser.parse_args()
    for build in args.minimal:
        minimal_graph(build)
    desktop_graph(args.desktop)
    diagnostics()
