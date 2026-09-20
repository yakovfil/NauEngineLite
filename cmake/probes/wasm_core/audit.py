"""Compare production core sources and target options in configured native/Wasm graphs."""
import argparse
import json
from pathlib import Path


def graph(build):
    reply = build / ".cmake/api/v1/reply"
    index = json.loads(max(reply.glob("index-*.json"), key=lambda p: p.stat().st_mtime).read_text())
    model = json.loads((reply / index["reply"]["codemodel-v2"]["jsonFile"]).read_text())
    return [{t["name"]: json.loads((reply / t["jsonFile"]).read_text()) for t in c["targets"]} for c in model["configurations"]]


def sources(target):
    return {s["path"].replace("\\", "/").split("kernel/src/")[-1] for s in target["sources"] if s["path"].endswith(".cpp")}


parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--native", type=Path, required=True)
parser.add_argument("--wasm", type=Path, action="append", required=True)
args = parser.parse_args()
native_graphs = graph(args.native)
native_sources = sources(native_graphs[0]["NauKernel"])
native_common = {s for s in native_sources if "/windows/" not in s}
for native in native_graphs:
    assert not {"PlatformApp", "Graphics", "CoreScene", "MinimalAppSample", "NauMinimalLifecycleTests"} & native.keys()
    assert sources(native["NauKernel"]) == native_sources
    assert "-pthread" not in str(native["NauKernel"].get("compileGroups"))
    assert "_TARGET_PC_WIN=1" in str(native["NauKernel"].get("compileGroups"))
for build in args.wasm:
    wasm = graph(build)[0]
    assert {"NauKernel", "NauFrameworkMinimal", "NauCorePortabilityTests"} <= wasm.keys()
    assert not {"PlatformApp", "Graphics", "CoreScene", "MinimalAppSample", "NauMinimalLifecycleTests"} & wasm.keys()
    wasm_sources = sources(wasm["NauKernel"])
    assert not any("/windows/" in s for s in wasm_sources)
    assert {s for s in wasm_sources if "/emscripten/" not in s} == native_common
    commands = json.loads((build / "compile_commands.json").read_text())
    for entry in commands:
        command = entry["command"]
        assert "-pthread" in command and "-fno-exceptions" in command, command
        assert "_TARGET_PC_WIN" not in command and "/MD" not in command, command
        assert "emscripten" in command, command
    print(f"PASS {build.name}: {len(native_common)} common sources, {len(wasm_sources) - len(native_common)} Wasm backends, {len(commands)} source compile commands")
