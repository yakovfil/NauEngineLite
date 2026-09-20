// Copyright 2026 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.
#include "nau/render/browser_renderer.h"

#include <GLES3/gl3.h>
#include <emscripten.h>
#include <emscripten/html5.h>
#include <emscripten/threading.h>

#include <algorithm>
#include <cmath>

#include "nau/scene/scene_object.h"

namespace nau
{
    struct BrowserRenderer::State
    {
        EMSCRIPTEN_WEBGL_CONTEXT_HANDLE context = 0;
        GLuint program = 0, vao = 0, buffers[3] = {};
        GLsizei count = 0;
        int width = 0, height = 0, limit = 0;
        unsigned frames = 0;
        bool stopped = false;
        Error::Ptr failure;
    };

    BrowserRenderer::BrowserRenderer() :
        m_state(std::make_unique<State>())
    {
    }
    BrowserRenderer::~BrowserRenderer()
    {
        stop().ignore();
    }

    Result<> BrowserRenderer::initialize(const CpuMeshView& mesh, std::string_view vertex, std::string_view fragment)
    {
        auto& state = *m_state;
        if (state.stopped || state.context || emscripten_is_main_browser_thread())
            return NauMakeError("Renderer initialization requires a fresh application-worker owner");
        if (mesh.positions.empty() || mesh.normals.size() != mesh.positions.size() || mesh.indices.empty())
            return NauMakeError("Renderer requires validated CPU mesh geometry and normals");
        const int canvasReady = MAIN_THREAD_EM_ASM_INT({
            const canvas = document.getElementById('canvas');
            return canvas instanceof HTMLCanvasElement && canvas.isConnected;
        });
        if (!canvasReady)
            return NauMakeError("Missing or invalid canvas host");
        EmscriptenWebGLContextAttributes attributes;
        emscripten_webgl_init_context_attributes(&attributes);
        attributes.majorVersion = 2;
        attributes.explicitSwapControl = true;
        attributes.proxyContextToMainThread = EMSCRIPTEN_WEBGL_CONTEXT_PROXY_DISALLOW;
        attributes.antialias = false;
        state.context = emscripten_webgl_create_context("#canvas", &attributes);
        if (state.context <= 0 || emscripten_webgl_make_context_current(state.context) != EMSCRIPTEN_RESULT_SUCCESS)
            return NauMakeError("WebGL 2 application-worker context unavailable");
        auto compile = [](GLenum type, std::string_view source) -> Result<GLuint>
        {
            const GLuint shader = glCreateShader(type);
            const char* text = source.data();
            const GLint length = static_cast<GLint>(source.size());
            glShaderSource(shader, 1, &text, &length);
            glCompileShader(shader);
            GLint okay = 0;
            glGetShaderiv(shader, GL_COMPILE_STATUS, &okay);
            if (!okay)
            {
                char message[2048] = {};
                glGetShaderInfoLog(shader, sizeof(message), nullptr, message);
                glDeleteShader(shader);
                return NauMakeError("Browser shader compilation failed: {}", message);
            }
            return shader;
        };
        auto vs = compile(GL_VERTEX_SHADER, vertex);
        if (!vs)
            return vs.getError();
        auto fs = compile(GL_FRAGMENT_SHADER, fragment);
        if (!fs)
        {
            glDeleteShader(*vs);
            return fs.getError();
        }
        state.program = glCreateProgram();
        glAttachShader(state.program, *vs);
        glAttachShader(state.program, *fs);
        glLinkProgram(state.program);
        glDeleteShader(*vs);
        glDeleteShader(*fs);
        GLint okay = 0;
        glGetProgramiv(state.program, GL_LINK_STATUS, &okay);
        if (!okay)
        {
            char message[2048] = {};
            glGetProgramInfoLog(state.program, sizeof(message), nullptr, message);
            return NauMakeError("Browser shader link failed: {}", message);
        }
        for (const char* name : {"model", "view", "projection"})
            if (glGetUniformLocation(state.program, name) < 0)
                return NauMakeError("Browser shader missing required uniform {}", name);
        glGenVertexArrays(1, &state.vao);
        glBindVertexArray(state.vao);
        glGenBuffers(3, state.buffers);
        for (unsigned index = 0; index < 2; ++index)
        {
            const auto& values = index == 0 ? mesh.positions : mesh.normals;
            glBindBuffer(GL_ARRAY_BUFFER, state.buffers[index]);
            glBufferData(GL_ARRAY_BUFFER, values.size() * sizeof(values[0]), values.data(), GL_STATIC_DRAW);
            glEnableVertexAttribArray(index);
            glVertexAttribPointer(index, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
        }
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, state.buffers[2]);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.indices.size() * sizeof(uint16_t), mesh.indices.data(), GL_STATIC_DRAW);
        state.count = static_cast<GLsizei>(mesh.indices.size());
        GLint viewport[2] = {};
        glGetIntegerv(GL_MAX_VIEWPORT_DIMS, viewport);
        glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE, &state.limit);
        state.limit = std::min({state.limit, viewport[0], viewport[1]});
        if (state.limit <= 0 || glGetError() != GL_NO_ERROR)
            return NauMakeError("Browser mesh upload or graphics limits failed");
        return poll();
    }

    Result<> BrowserRenderer::poll()
    {
        auto& state = *m_state;
        if (state.failure)
            return state.failure;
        if (state.stopped)
            return {};
        if (!state.context)
            return {};
        if (emscripten_is_webgl_context_lost(state.context))
            state.failure = NauMakeError("WebGL context lost; reload to retry");
        double size[3] = {};
        // Synchronous ordered snapshots; no page callback retains renderer memory.
        const int connected = MAIN_THREAD_EM_ASM_INT({
            const c = document.getElementById('canvas');
            if (!(c instanceof HTMLCanvasElement) || !c.isConnected)
                return 0;
            HEAPF64[$0 >> 3] = c.clientWidth;
            HEAPF64[($0 >> 3) + 1] = c.clientHeight;
            HEAPF64[($0 >> 3) + 2] = window.devicePixelRatio;
            return 1;
        },
                                                     size);
        if (!connected)
            state.failure = NauMakeError("Renderer canvas host lost");
        if (state.failure)
            return state.failure;
        if (!std::isfinite(size[2]) || size[2] <= 0)
            return state.failure = NauMakeError("Invalid canvas pixel ratio");
        const double width = std::round(size[0] * size[2]), height = std::round(size[1] * size[2]);
        const double scale = std::min(1.0, state.limit / std::max({1.0, width, height}));
        const int w = width > 0 ? std::max(1, int(width * scale)) : 0;
        const int h = height > 0 ? std::max(1, int(height * scale)) : 0;
        if (w != state.width || h != state.height)
        {
            state.width = w;
            state.height = h;
            if (w && h && emscripten_set_canvas_element_size("#canvas", w, h) != EMSCRIPTEN_RESULT_SUCCESS)
                return state.failure = NauMakeError("Canvas drawable resize failed");
        }
        return {};
    }

    Result<> BrowserRenderer::draw(const scene::SceneObject& mesh, const scene::SceneObject& camera)
    {
        auto& state = *m_state;
        if (state.stopped)
            return NauMakeError("Draw requested after renderer Stop");
        NauCheckResult(poll());
        if (!state.context || !state.program)
            return NauMakeError("Renderer is not ready");
        if (!state.width || !state.height)
            return {};
        const auto& lens = camera.getRootComponent<scene::CameraComponent>();
        const auto model = mesh.getWorldTransform().getMatrix();
        const auto view = math::inverse(camera.getWorldTransform().getMatrix());
        const float nearPlane = lens.getClipNearPlane(), farPlane = lens.getClipFarPlane();
        const float f = 1.f / std::tan(lens.getFov() * 0.00872664626f);
        const float aspect = float(state.width) / state.height;
        const float projection[16] = {f / aspect, 0, 0, 0, 0, f, 0, 0, 0, 0, (farPlane + nearPlane) / (nearPlane - farPlane), -1, 0, 0, 2 * farPlane * nearPlane / (nearPlane - farPlane), 0};
        float modelValues[16], viewValues[16];
        const auto copyMatrix = [](const math::Matrix4& matrix, float* values)
        {
            const math::Vector4 columns[] = {matrix.getCol0(), matrix.getCol1(), matrix.getCol2(), matrix.getCol3()};
            for (unsigned column = 0; column < 4; ++column)
            {
                values[column * 4] = float(columns[column].getX());
                values[column * 4 + 1] = float(columns[column].getY());
                values[column * 4 + 2] = float(columns[column].getZ());
                values[column * 4 + 3] = float(columns[column].getW());
            }
        };
        copyMatrix(model, modelValues);
        copyMatrix(view, viewValues);
        glViewport(0, 0, state.width, state.height);
        glEnable(GL_DEPTH_TEST);
        glClearColor(0.025f, 0.035f, 0.055f, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(state.program);
        glUniformMatrix4fv(glGetUniformLocation(state.program, "model"), 1, GL_FALSE, modelValues);
        glUniformMatrix4fv(glGetUniformLocation(state.program, "view"), 1, GL_FALSE, viewValues);
        glUniformMatrix4fv(glGetUniformLocation(state.program, "projection"), 1, GL_FALSE, projection);
        glBindVertexArray(state.vao);
        glDrawElements(GL_TRIANGLES, state.count, GL_UNSIGNED_SHORT, nullptr);
        if (glGetError() != GL_NO_ERROR || emscripten_webgl_commit_frame() != EMSCRIPTEN_RESULT_SUCCESS)
            return state.failure = NauMakeError("Browser draw submission failed");
        ++state.frames;
        MAIN_THREAD_EM_ASM({
            Module.nauRender = ({frames: $0, width: $1, height: $2, indices: $3, worker: true, model: Array.from(HEAPF32.subarray($4 >> 2, ($4 >> 2) + 16)), view: Array.from(HEAPF32.subarray($5 >> 2, ($5 >> 2) + 16)), projection: Array.from(HEAPF32.subarray($6 >> 2, ($6 >> 2) + 16))});
        },
                           state.frames, state.width, state.height, state.count, modelValues, viewValues, projection);
        return {};
    }

    Result<> BrowserRenderer::stop()
    {
        auto& state = *m_state;
        if (state.stopped)
            return {};
        state.stopped = true;
        if (state.context > 0)
        {
            if (!emscripten_is_webgl_context_lost(state.context))
            {
                glDeleteBuffers(3, state.buffers);
                glDeleteVertexArrays(1, &state.vao);
                if (state.program)
                    glDeleteProgram(state.program);
            }
            emscripten_webgl_make_context_current(0);
            const auto destroyed = emscripten_webgl_destroy_context(state.context);
            state.context = 0;
            if (destroyed != EMSCRIPTEN_RESULT_SUCCESS)
                return NauMakeError("Browser context destruction failed");
        }
        MAIN_THREAD_EM_ASM({if(Module.nauRender) Module.nauRender.stopped = true; });
        return {};
    }
}  // namespace nau
