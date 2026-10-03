// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved

#include "imguiRenderer.h"

#include <imgui.h>
#include <nau/app/application.h>
#include <nau/assets/asset_descriptor.h>
#include <nau/assets/asset_manager.h>
#include <nau/shaders/shader_globals.h>

#include "nau/3d/dag_render.h"
#include "nau/image/dag_texPixel.h"
#include "nau/memory/heap_byte_allocator.h"

constexpr int NUM_VERTEX_CHANNELS = 3;

static int imgui_mvp_VarIds[4];
static int imgui_texVarId = -1;

DagImGuiRenderer::DagImGuiRenderer()
{
    const auto mvp = nau::math::Matrix4::identity();
    nau::shader_globals::addVariable("mvp", sizeof(mvp), &mvp);

    d3d::SamplerInfo sampInfo;
    sampler = d3d::create_sampler(sampInfo);
}

void DagImGuiRenderer::setBackendFlags(ImGuiIO& io)
{
    io.BackendRendererName = "imgui_impl_dagor";

    // We can honor the ImDrawCmd::VtxOffset field, allowing for large meshes.
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
}

void DagImGuiRenderer::createAndSetFontTexture(ImGuiIO& io)
{
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

    nau::HeapByteAllocator tmpAlloc;
    TexImage32* img = TexImage32::create(width, height, &tmpAlloc);
    memcpy(img->getPixels(), pixels, width * height * 4);
    fontTex = d3d::create_tex(img, width, height, TEXFMT_R8G8B8A8, 1, u8"imgui_font");
    memfree(img, &tmpAlloc);
    io.Fonts->TexID = reinterpret_cast<ImTextureID>(fontTex);
}

void DagImGuiRenderer::render(ImDrawData* draw_data)
{
    NAU_ASSERT(fontTex);
    if (!m_imguiDefaultMaterial)
    {
        auto getMaterial = [this]() -> nau::async::Task<nau::MaterialAssetView::Ptr>
        {
            co_await nau::getApplication().getExecutor();
            co_return co_await m_imguiDefaultMaterialRef.getAssetViewTyped<nau::MaterialAssetView>();
        };
        auto task = getMaterial();
        nau::async::wait(task);
        m_imguiDefaultMaterial = task.result();
    }
    NAU_ASSERT(m_imguiDefaultMaterial);
    // Avoid rendering when minimized
    if (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f)
    {
        return;
    }

    if (draw_data->TotalVtxCount == 0)
    {
        return;
    }

    // Create and grow vertex/index buffers if needed
    if (!vb || vbSize < draw_data->TotalVtxCount)
    {
        vb = d3d::create_vb(sizeof(ImDrawVert) * (draw_data->TotalVtxCount + 5000), SBCF_DYNAMIC | SBCF_CPU_ACCESS_WRITE, u8"imgui_vb");
        NAU_ASSERT(vb);
        vbSize = draw_data->TotalVtxCount + 5000;
    }
    if (!ib || ibSize < draw_data->TotalIdxCount)
    {
        ib = d3d::create_ib(sizeof(ImDrawIdx) * (draw_data->TotalIdxCount + 10000), SBCF_DYNAMIC | SBCF_CPU_ACCESS_WRITE, u8"imgui_ib");
        NAU_ASSERT(ib);
        ibSize = draw_data->TotalIdxCount + 10000;
    }

    // Upload vertex/index data into a single contiguous GPU buffer
    ImDrawVert* vbdata = nullptr;
    ImDrawIdx* ibdata = nullptr;
    bool vbLockSuccess =
        vb->lock(0, draw_data->TotalVtxCount * sizeof(ImDrawVert), (void**)&vbdata, VBLOCK_WRITEONLY | VBLOCK_DISCARD);
    NAU_ASSERT(vbLockSuccess && vbdata != nullptr);
    G_UNUSED(vbLockSuccess);
    bool ibLockSuccess =
        ib->lock(0, draw_data->TotalIdxCount * sizeof(ImDrawIdx), (void**)&ibdata, VBLOCK_WRITEONLY | VBLOCK_DISCARD);
    NAU_ASSERT(ibLockSuccess && ibdata != nullptr);
    G_UNUSED(ibLockSuccess);
    for (int n = 0; n < draw_data->CmdListsCount; n++)
    {
        const ImDrawList* cmdList = draw_data->CmdLists[n];
        memcpy(vbdata, cmdList->VtxBuffer.Data, cmdList->VtxBuffer.Size * sizeof(ImDrawVert));
        memcpy(ibdata, cmdList->IdxBuffer.Data, cmdList->IdxBuffer.Size * sizeof(ImDrawIdx));
        vbdata += cmdList->VtxBuffer.Size;
        ibdata += cmdList->IdxBuffer.Size;
    }
    vb->unlock();
    ib->unlock();

    const float L = draw_data->DisplayPos.x;
    const float R = draw_data->DisplayPos.x + draw_data->DisplaySize.x;
    const float T = draw_data->DisplayPos.y;
    const float B = draw_data->DisplayPos.y + draw_data->DisplaySize.y;
    // clang-format off
  const nau::math::Matrix4 mvp = {
    {2.0f / (R - L), 0.0f, 0.0f, 0.0f},
    {0.0f, 2.0f / (T - B), 0.0f, 0.0f},
    {0.0f, 0.0f, 0.5f, 0.0f},
    {(R + L) / (L - R), (T + B) / (B - T), 0.5f, 1.0f}
  };
    // clang-format on

    nau::shader_globals::setVariable("mvp", &mvp);

    d3d::setview(0, 0, draw_data->DisplaySize.x, draw_data->DisplaySize.y, 0.0f, 1.0f);
    d3d::setvsrc(0, vb, vStride);
    d3d::setind(ib);

    m_imguiDefaultMaterial->bind();

    int globalIdxOffset = 0;
    int globalVtxOffset = 0;
    ImVec2 clipOff = draw_data->DisplayPos;
    for (int n = 0; n < draw_data->CmdListsCount; n++)
    {
        const ImDrawList* cmdList = draw_data->CmdLists[n];
        for (int cmd_i = 0; cmd_i < cmdList->CmdBuffer.Size; cmd_i++)
        {
            const ImDrawCmd* pcmd = &cmdList->CmdBuffer[cmd_i];
            if (pcmd->UserCallback != nullptr)
            {
                // User callback, registered via ImDrawList::AddCallback()
                // (ImDrawCallback_ResetRenderState is a special callback value used by the user to request the renderer to reset render
                // state.)
                if (pcmd->UserCallback != ImDrawCallback_ResetRenderState)
                    pcmd->UserCallback(cmdList, pcmd);
            }
            else
            {
                nau::math::RectInt scissorRect;
                scissorRect.left = pcmd->ClipRect.x - clipOff.x;
                scissorRect.top = pcmd->ClipRect.y - clipOff.y;
                scissorRect.right = pcmd->ClipRect.z - clipOff.x;
                scissorRect.bottom = pcmd->ClipRect.w - clipOff.y;
                const int w = scissorRect.right - scissorRect.left;
                const int h = scissorRect.bottom - scissorRect.top;
                if (w > 0 && h > 0)
                {
                    d3d::setscissor(scissorRect.left, scissorRect.top, w, h);

                    BaseTexture* tex = reinterpret_cast<BaseTexture*>(pcmd->TextureId);
                    d3d::settex(0, tex);
                    d3d::set_sampler(STAGE_PS, 0, sampler);

                    d3d::drawind(PRIM_TRILIST, pcmd->IdxOffset + globalIdxOffset, pcmd->ElemCount / 3, pcmd->VtxOffset + globalVtxOffset);
                }
            }
        }
        globalIdxOffset += cmdList->IdxBuffer.Size;
        globalVtxOffset += cmdList->VtxBuffer.Size;
    }
}
