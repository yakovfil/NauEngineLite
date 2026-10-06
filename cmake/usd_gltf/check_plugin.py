"""Check plugin loading and triangle import with the SDK's Python 3.10 bindings."""

import argparse
import base64
import json
import os
from pathlib import Path
import struct
import sys
import tempfile


parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("sdk", type=Path)
parser.add_argument("--deployment", type=Path, help="Check DLLs and plugins in a deployed editor")
args = parser.parse_args()
sdk = args.sdk.resolve()
runtime = args.deployment.resolve() if args.deployment else sdk
directories = [runtime] if args.deployment else [sdk / "bin", sdk / "lib"]
dll_handles = [os.add_dll_directory(str(path)) for path in directories]
# USD's native plugin loader also uses the process search path.
os.environ["PATH"] = os.pathsep.join(map(str, directories)) + os.pathsep + os.environ["PATH"]
plugin_root = runtime / "plugins" if args.deployment else sdk / "plugin/usd"
os.environ["PXR_PLUGINPATH_NAME"] = str(plugin_root)
sys.path.insert(0, str(runtime / "python" if args.deployment else sdk / "lib/python"))

from pxr import Plug, Sdf, Usd, UsdGeom

plugin = Plug.Registry().GetPluginWithName("usdGltf_plugin")
assert plugin, "usdGltf_plugin was not registered"
assert Path(plugin.path).is_relative_to(runtime), plugin.path
assert plugin.Load(), f"Could not load {plugin.path}"
print(f"Loaded {plugin.path}")

binary = struct.pack("<9f3H", 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 2)
document = {
    "asset": {"version": "2.0"},
    "scene": 0,
    "scenes": [{"nodes": [0]}],
    "nodes": [{"mesh": 0}],
    "meshes": [{"primitives": [{"attributes": {"POSITION": 0}, "indices": 1}]}],
    "buffers": [{"byteLength": len(binary)}],
    "bufferViews": [
        {"buffer": 0, "byteOffset": 0, "byteLength": 36},
        {"buffer": 0, "byteOffset": 36, "byteLength": 6},
    ],
    "accessors": [
        {"bufferView": 0, "componentType": 5126, "count": 3, "type": "VEC3",
         "min": [0, 0, 0], "max": [1, 1, 0]},
        {"bufferView": 1, "componentType": 5123, "count": 3, "type": "SCALAR"},
    ],
}
with tempfile.TemporaryDirectory(prefix="nau-usd-gltf-") as temporary:
    root = Path(temporary)
    data = json.dumps(document).encode("utf-8")
    data += b" " * (-len(data) % 4)
    padded_binary = binary + b"\0" * (-len(binary) % 4)
    chunks = (struct.pack("<II", len(data), 0x4E4F534A) + data
              + struct.pack("<II", len(padded_binary), 0x004E4942) + padded_binary)
    (root / "triangle.glb").write_bytes(struct.pack("<III", 0x46546C67, 2, 12 + len(chunks)) + chunks)
    document["buffers"][0]["uri"] = "data:application/octet-stream;base64," + base64.b64encode(binary).decode("ascii")
    (root / "triangle.gltf").write_text(json.dumps(document), encoding="utf-8")
    for extension in ("gltf", "glb"):
        assert Sdf.FileFormat.FindByExtension(extension), extension
        stage = Usd.Stage.Open(str(root / f"triangle.{extension}"))
        assert stage, extension
        meshes = [UsdGeom.Mesh(prim) for prim in stage.Traverse() if prim.IsA(UsdGeom.Mesh)]
        assert len(meshes) == 1, (extension, len(meshes))
        mesh = meshes[0]
        assert list(mesh.GetPointsAttr().Get()) == [(0, 0, 0), (1, 0, 0), (0, 1, 0)]
        assert list(mesh.GetFaceVertexCountsAttr().Get()) == [3]
        assert list(mesh.GetFaceVertexIndicesAttr().Get()) == [0, 1, 2]
        print(f"PASS: {extension} triangle geometry")
