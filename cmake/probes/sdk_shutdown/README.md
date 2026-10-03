# Relocated SDK shutdown probe

From the parent Nau checkout on Windows:

```powershell
python NauEngineLite/cmake/probes/sdk_shutdown/run.py --sdk C:/temp/relocated-sdk --output NauEngineLite/build/sdk-shutdown-check
```

The probe launches installed SceneBaseSample and inputDemo three times per
configuration, posts WM_CLOSE to the freshly created primary NAU window at
(10, 10), and requires a zero process exit within 20 seconds. inputDemo creates
a secondary window 200 pixels to the right whose close does not quit the app;
Process.CloseMainWindow can select that secondary window and produce a false
shutdown timeout. The recorded window inventory makes this selection reviewable.

Each run keeps its application output and structured exit/error result. A live
process is terminated only after the shutdown timeout. Use `--configuration
Debug` or `Release`, `--repetitions 1`, and `--close-delay 20` for focused
comparisons. A longer close delay is diagnostic evidence and does not replace
the default early-close checks. The probe does not provide human scene/input
acceptance, nor does zero exit prove that every application diagnostic is clean.

Use `--close-secondary-first` to check the other inputDemo close order: its
secondary window must disappear while the process remains alive, then closing
the primary window must finish shutdown normally. The default leaves the
secondary window open, exercising driver cleanup of remaining swapchains.

Pass `--consumer C:/temp/generated-project/StdSdkConsumer` to include the
generated consumer's `StdSdkConsumerMain.exe` in both configurations. Each
application runs from its own SDK or project root, which is recorded in JSON.
