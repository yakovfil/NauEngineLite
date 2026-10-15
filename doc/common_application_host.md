# Common application host

Use the owning runner for native and browser applications:

```cpp
#include "nau/app/run_application.h"

int main()
{
    return nau::runApplication(std::make_unique<MyApplication>(),
        {.scheduling = nau::ApplicationScheduling::Presentation});
}
```

`MyApplication` derives from `nau::ApplicationDelegate`. Configure basic properties
in `configureApplication()`, provide the consumer's module list through
`getModulesListString()`, and register application services in `initializeServices()`.
Configure static module registration in the consumer build when using a static
runtime. The framework creates the platform window service and the delegate adapter.

After required services initialize, the host invokes `onApplicationInitialized()`
and begins `startupApplication()` once. That asynchronous task is part of startup:
Running and application updates wait for its successful completion. Pending startup
continues executor and lifecycle progress. Startup can await service presentation
without depending on application update callbacks. Stop prevents later startup
actions and preserves resources until accepted work settles.

## Update phases

```text
onApplicationPreUpdate(dt)
          |
          v
optional scene manager update
          |
          v
onApplicationPostUpdate(dt)
```

The callbacks run on the application owner thread with equal elapsed time. They
also run in non-rendering applications without a scene manager. Pre-update is
suitable for application changes that scene components should see in that step.
Post-update follows the scene manager call; it is not a GPU completion barrier or
a guarantee that asynchronous component work has finished. Ordering among unrelated
services within either phase is not guaranteed.

The default pre-update callback does nothing. Default post-update calls the legacy
`onApplicationStep(dt)` once, preserving existing delegates' phase. Overriding
explicit post-update replaces that forwarding. Call the base post-update method
explicitly if both behaviors are wanted. If pre-update requests Stop, the host
does not dispatch the application's post-update callback.

## Scheduling, ownership and errors

`ApplicationScheduling::Default` retains native timing and runs browser updates
as fast as the worker event loop permits, yielding between iterations without an
engine-imposed delay. Hidden documents pause updates and poll lifecycle work every
50 ms. Browser scheduling and GPU backpressure can still limit throughput; the
submission rate is not necessarily the display refresh rate.
`Presentation` retains the native loop policy and selects
cooperative frame-driven browser scheduling. Browser resume uses bounded elapsed
time and discards missed presentation opportunities. The native mode does not
promise display synchronization or a fixed frame rate.

The runner owns the delegate until safe application/service teardown. Browser
applications do not need a static delegate: ownership moves into the persistent
host session before scheduler handoff. The existing borrowed low-level browser
runner remains available and retains its caller-managed lifetime requirements
and legacy fixed cadence unless cooperative frame-driven or uncapped updates are requested.

Native execution returns zero after an orderly stop and nonzero after a checked
creation, startup or cleanup failure. Cooperative browser execution hands control
to the browser event loop, so it does not synchronously return the eventual result.
Observe `Module.nauRuntime.status` and the runtime exit code. Call
`Module.nauRuntime.stop()` for asynchronous, idempotent Stop. Another browser run
requires page reload. Fatal aborts cannot promise cleanup.

`foregroundProgressTimeoutMs` controls the browser foreground progress deadline.
A timeout reports failure while preserving live resources until work can settle;
it does not count as successful teardown.

Rebuild delegates and framework libraries together after this API change. Existing
delegate source remains supported, but added virtual methods change the vtable.
