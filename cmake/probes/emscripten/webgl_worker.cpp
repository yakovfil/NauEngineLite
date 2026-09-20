#include <GLES3/gl3.h>
#include <emscripten.h>
#include <emscripten/html5.h>
#include <emscripten/threading.h>

#include <chrono>
#include <thread>

namespace
{
    void waitForEvents(int milliseconds)
    {
#ifdef NAU_PROBE_BLOCKING_WORKER
        std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
#else
        emscripten_sleep(milliseconds);
#endif
    }

    void report(const char* phase, int width, int height, int pixel)
    {
        MAIN_THREAD_EM_ASM({
            window.probeEvents.push({phase: UTF8ToString($0), width: $1, height: $2, pixel: $3});
        },
                           phase, width, height, pixel);
    }

    bool draw(EMSCRIPTEN_WEBGL_CONTEXT_HANDLE context, int width, int height)
    {
        if (emscripten_is_webgl_context_lost(context))
        {
            report("draw-rejected-lost", 0, 0, 0);
            return false;
        }
        if (emscripten_set_canvas_element_size("#canvas", width, height) != EMSCRIPTEN_RESULT_SUCCESS)
        {
            return false;
        }
        glViewport(0, 0, width, height);
        glClearColor(0.08f, 0.42f, 0.85f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        unsigned char pixel[4] = {};
        glReadPixels(width / 2, height / 2, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
        if (glGetError() != GL_NO_ERROR || pixel[2] < 200 || pixel[0] > 30)
        {
            return false;
        }
        int actualWidth = 0;
        int actualHeight = 0;
        if (emscripten_get_canvas_element_size("#canvas", &actualWidth, &actualHeight) != EMSCRIPTEN_RESULT_SUCCESS || actualWidth != width || actualHeight != height)
        {
            return false;
        }
        if (emscripten_webgl_commit_frame() != EMSCRIPTEN_RESULT_SUCCESS)
        {
            return false;
        }
        report("draw", actualWidth, actualHeight, pixel[2]);
        return true;
    }
}  // namespace

int main()
{
    if (emscripten_is_main_browser_thread())
    {
        report("failed-main-thread", 0, 0, 0);
        return 1;
    }
    EmscriptenWebGLContextAttributes attributes;
    emscripten_webgl_init_context_attributes(&attributes);
    attributes.majorVersion = 2;
    attributes.explicitSwapControl = true;
    attributes.proxyContextToMainThread = EMSCRIPTEN_WEBGL_CONTEXT_PROXY_DISALLOW;
    const auto context = emscripten_webgl_create_context("#canvas", &attributes);
    if (context <= 0 || emscripten_webgl_make_context_current(context) != EMSCRIPTEN_RESULT_SUCCESS)
    {
        report("failed-context", 0, 0, 0);
        return 1;
    }
    // SDK 6.0.9's C callback helper targets the DOM canvas on the UI thread.
    // The transferred context emits loss on its worker-owned OffscreenCanvas.
    EM_ASM({
        Module.probeCanvas = GL.currentContext.GLctx.canvas;
        Module.probeLost = false;
        Module.probeLossListener = () =>
        {
            Module.probeLost = true;
        };
        Module.probeCanvas.addEventListener('webglcontextlost', Module.probeLossListener);
    });
    report("worker-context", 0, 0, 0);
    if (!draw(context, 320, 180))
    {
        report("failed-draw", 0, 0, 0);
        return 1;
    }
    waitForEvents(500);
    if (!draw(context, 640, 360))
    {
        report("failed-resize", 0, 0, 0);
        return 1;
    }
    waitForEvents(1000);
    const bool hasLossExtension = EM_ASM_INT({
        const extension = GL.currentContext.GLctx.getExtension('WEBGL_lose_context');
        if (!extension)
            return 0;
        extension.loseContext();
        return 1;
    });
    if (!hasLossExtension)
    {
        report("failed-loss-extension", 0, 0, 0);
        return 1;
    }
    const auto lossDetected = [context]()
    {
#ifdef NAU_PROBE_POLL_LOSS
        return emscripten_is_webgl_context_lost(context);
#else
        return EM_ASM_INT({ return Module.probeLost; }) != 0;
#endif
    };
    for (int i = 0; i < 100 && !lossDetected(); ++i)
    {
        waitForEvents(10);
    }
    if (!lossDetected())
    {
        report("failed-loss-notification", 0, 0, 0);
        return 1;
    }
    report("context-lost", 0, 0, 0);
    if (draw(context, 640, 360))
    {
        report("failed-draw-after-loss", 0, 0, 0);
        return 1;
    }
    EM_ASM({
        Module.probeCanvas.removeEventListener('webglcontextlost', Module.probeLossListener);
        delete Module.probeLossListener;
        delete Module.probeCanvas;
    });
    if (emscripten_webgl_destroy_context(context) != EMSCRIPTEN_RESULT_SUCCESS)
    {
        report("failed-destroy", 0, 0, 0);
        return 1;
    }
    report("disposed", 0, 0, 0);
    return 0;
}
