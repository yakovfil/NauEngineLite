# Standard-library migration probes

From the engine directory:

```powershell
cmake -S cmake/probes/standard_library -B build/replace-eastl-baseline -G "Visual Studio 18 2026" -A x64 -T v145
cmake --build build/replace-eastl-baseline --config Debug
ctest --test-dir build/replace-eastl-baseline -C Debug --output-on-failure
```

Repeat build/CTest with Release. The former baseline target was retired with
the bundled library. Its captured observations are in the change's
`baseline-results.json`, and the replacement tests below cover the preserved
allocation, container and ownership contracts. The former trivial-assignment
failure remains documented in root `failed-tests.md`.

`test_resource_allocator` is independent of EASTL. It exercises the new generic
typed adapter against instrumented resources, including over-aligned values,
standard map node rebinding, unequal-resource move assignment and paired
deallocation. `resource_allocator_overflow` verifies fatal diagnostic behavior
in a child process; only this test executable disables Windows CRT dialogs.
Real array/pool/frame/stack tests live in test_runtime (StandardAllocator). DLL validation remains a separate pending migration task.

The resource passed to `nau::ResourceAllocator` is non-owning and must outlive its
containers. Its allocate/deallocate methods must honor alignment and paired
release. Copy/move assignment does not propagate resources; ordinary container
swap requires equal resources. No default resource or silent heap fallback is
provided by this primitive.

`test_lru_cache` verifies the standard-backed cache, including recency, duplicate
insertion, replacement/eviction callbacks, resize/clear, move-only values and
borrowed shader-like byte spans under an external lock. `lru_zero_capacity`
checks that both get and insert fail with a diagnostic at zero capacity, before
calling a creator. Empty queries, clear and resize remain valid at zero capacity.

`test_intrusive_ptr` verifies private and ADL reference hooks, copy/move/adopt,
const conversion, self-assignment, and exactly one final pooled-lease return.
`test_intrusive_list` verifies non-owning stable membership, standard iterator
concepts, unlink/reuse, clear and list destruction. Actual Windows timer
cancellation and shutdown are covered by test_runtime.

On Windows, `allocation_modules` loads two DLLs and verifies allocation origin,
aligned vector growth, rebound map nodes, unequal-resource moves and release on
another thread. Configure the same probe project with `-T ClangCL` in a separate
build directory to run against clang-cl and the supported MSVC STL. Modules must
remain loaded while their allocated storage is live.

`test_fixed_function` and `test_fixed_move_only_function` check bounded callable
copy/move lifetimes, zero backing allocation, forwarding and compile-time
capture limits. `fixed_move_only_empty` runs both empty invocation overloads in
child processes and requires the fatal diagnostic, including Release builds.
The copyable wrapper uses bad_function_call when exceptions are enabled and a
fatal diagnostic otherwise.
