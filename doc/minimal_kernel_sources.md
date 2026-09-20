# Minimal kernel source selection

`cmake/NauMinimalKernelSources.cmake` lists common implementations and platform
backends for the production NauKernel target. Desktop source discovery is unchanged.

The common set retains application APIs; task/executor/timer/work-queue machinery;
runtime registration/disposal; services and static modules; memory and RTTI;
messaging; strings, JSON/native runtime values and math; logging; virtual filesystem
construction, directory/path APIs and memory streams. These support the existing
application_setup, application_impl, global_properties and background/window
services. RuntimeState initializes timers and the thread pool unconditionally.

Excluded groups are dataBlock, dag_ioSys, osApiWrappers, asset-pack and archive
filesystems, native filesystem mounting, nau_container and the optional Dagor
thread-pool adapter. The selected framework excludes default_application_delegate
and application_init_delegate, which contain asset-pack/native mount calls.
VirtualFileSystemImpl uses its mount abstraction rather than constructing those
backends. JSON/global properties use stream/runtime-value APIs rather than DataBlock.
The real executor comes from thread_pool_executor.cpp; the Dagor adapter is not used.

Removing those groups removes brotli, lzma, zlib, zstd and md5 from the minimal
dependency list. Common required source libraries remain at recorded revisions.

The first native link exposed createNativeFileStream references in file_helper
and global_properties_impl. Therefore Windows win_file.cpp remains selected:
native file streams cannot be removed merely because native filesystem mounting
is unused. The Wasm backend must supply explicit documented behavior for these
retained APIs. Native lifecycle and focused core link tests verify this boundary;
Wasm backend tests now verify explicit unsupported host operations, virtual
filesystem construction and in-memory global properties. See
[the core validation guide](wasm_core_portability.md).

Further source inspection excludes the frustum SIMD implementation and Dagor
stream adapters: no selected kernel/framework implementation references them.
Their consumers belong to the excluded asset/renderer paths.
