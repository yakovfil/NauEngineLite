# Portable asset packages

Include `nau/io/asset_pack_file_system.h`. `openAssetPackFileSystem(path)` opens an archive through a libc file stream, including Emscripten preloaded files. `createAssetPackFileSystem(stream, length, options)` accepts an owned seekable stream and its explicit byte length. Both return `Result<IFileSystem::Ptr>`; check the result before mounting the filesystem through VFS.

The stream factory takes exclusive use of the input cursor. Open files and streams retain the backing storage after the original owner or filesystem is released. Independent streams have independent positions; backing seek/read pairs are serialized. Each individual stream follows the usual single-caller cursor contract. Reads stop at entry boundaries, seek clamps to the file range, and writes are rejected. No memory-mapping feature is advertised.

The format is the established version 0.1 `nau-vfs-pack` container: textual headers, an additional separator newline, JSON index, then uncompressed blobs. Offsets in the index are relative to the payload. `readAssetPackIndex` validates the index and returns absolute stream offsets. Empty compression markers and the historical single-NUL uncompressed marker are supported. Other versions and compression methods fail with diagnostics. The existing Windows path-based factory remains available for legacy mapped access.

Default limits are 4096 header bytes, 8 MiB index bytes, 65536 entries and 4096 bytes per path. `AssetPackReadOptions` can set these limits explicitly. The archive length and all offset/size arithmetic are checked before platform integer conversion. Invalid kind, malformed JSON, missing fields, duplicate normalized paths, traversal, drive-qualified paths, file/directory conflicts and out-of-bounds payloads fail before a filesystem is published.

The reader is part of the standalone kernel and has no dependency on concrete feature modules. Package construction tools are supplied externally. Preloaded archives occupy browser memory; this API provides no remote streaming or compression.
