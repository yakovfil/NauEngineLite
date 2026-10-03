// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved

#include "nau/render/deferredRenderer.h"

#include <graphics_assets/material_asset.h>
#include <nau/assets/asset_ref.h>

#include <memory>
#include <string_view>

// TODO: Console variables
// CONSOLE_BOOL_VAL("render", debug_tiled_resolve, false);

namespace nau::render
{
    namespace
    {
        inline void initShaderVar(std::string_view name, size_t size)
        {
            if (nau::shader_globals::containsName(name))
            {
                return;
            }
            nau::shader_globals::addVariable(name, size);
        }
    }  // namespace

    ShadingResolver::ShadingResolver(std::unique_ptr<PostFxRenderer> resolveShading) :
        resolveShading(std::move(resolveShading))
    {
        initShaderVar("globtm_inv", sizeof(math::Matrix4));
        initShaderVar("gbuffer_tm", sizeof(math::Matrix4));
    }

    ShadingResolver::~ShadingResolver() = default;

    void ShadingResolver::resolve(BaseTexture* resolveTarget,
                                  const math::Matrix4& view_proj_tm,
                                  BaseTexture* depth_bounds_tex,
                                  ClearTarget clear_target,
                                  const math::Matrix4& gbufferTm,
                                  const math::RectInt* resolve_area)
    {
        if (!resolveShading)
        {
            return;
        }

        auto globTmInv = inverse(view_proj_tm);

        nau::shader_globals::setVariable("globtm_inv", &globTmInv);
        nau::shader_globals::setVariable("gbuffer_tm", &gbufferTm);

        TextureInfo info;
        resolveTarget->getinfo(info);

        SCOPE_RENDER_TARGET;
        d3d::set_render_target(resolveTarget, 0);
        if (depth_bounds_tex)
        {
            d3d::set_depth(depth_bounds_tex, DepthAccess::SampledRO);
        }
        if (resolve_area)
        {
            d3d::setview(resolve_area->left, resolve_area->top, resolve_area->right - resolve_area->left,
                         resolve_area->bottom - resolve_area->top, 0, 1);
            d3d::setscissor(resolve_area->left, resolve_area->top, resolve_area->right - resolve_area->left,
                            resolve_area->bottom - resolve_area->top);
        }

        if (clear_target == ClearTarget::Yes)
        {
            d3d::clearview(CLEAR_TARGET, 0, 0, 0);
        }

        resolveShading->render();
    }

    DeferredRenderTarget::DeferredRenderTarget(std::unique_ptr<render::ShadingResolver> resolveShading,
                                               const char* name,
                                               int w,
                                               int h,
                                               DeferredRT::StereoMode stereo_mode,
                                               uint32_t depthFmt) :
        DeferredRenderTarget(std::move(resolveShading), name, w, h, stereo_mode, 0, DefaultGBuffer::GBUF_NUM, g_defaultGBufferFormats, depthFmt)
    {
    }

    DeferredRenderTarget::DeferredRenderTarget(std::unique_ptr<render::ShadingResolver> resolveShading,
                                               const char* name_,
                                               const int w,
                                               const int h,
                                               DeferredRT::StereoMode stereo_mode,
                                               const unsigned msaaFlag,
                                               const int num_rt,
                                               const unsigned* texFmt,
                                               const uint32_t depth_fmt) :
        m_renderTargets(name_, w, h, stereo_mode, msaaFlag, num_rt, texFmt, depth_fmt),
        m_shadingResolver(std::move(resolveShading))
    {
    }

    DeferredRenderTarget::~DeferredRenderTarget() = default;

    void DeferredRenderTarget::resourceBarrier(ResourceBarrier barrier)
    {
        for (int i = 0; i < m_renderTargets.getRtNum(); ++i)
        {
            if (Texture* rt = m_renderTargets.getRt(i))
            {
                d3d::resource_barrier({rt, barrier, 0, 0});
            }
        }
        if (Texture* ds = m_renderTargets.getDepth())
        {
            d3d::resource_barrier({ds, barrier, 0, 0});
        }
    }

    void DeferredRenderTarget::debugRender(int mode)
    {
        if (m_debugRenderer == nullptr)
        {
            // TODO: create correct material for DeferredRT debugging.
            // MaterialAssetRef matDebugAssetRef{u8"file:/content/materials/deferred_rt_debug.nmat_json"};
            // MaterialAssetView::Ptr matDebug = co_await matDebugAssetRef.getAssetViewTyped<MaterialAssetView>();

            // m_debugRenderer = new PostFxRenderer(matDebug, d3d::create_render_state(rendState));
        }

        if (mode >= 0)
        {
            initShaderVar("show_gbuffer", sizeof(mode));
            nau::shader_globals::setVariable("show_gbuffer", &mode);

            NAU_ASSERT(mode < m_renderTargets.getRtNum());

            m_debugRenderer->getMaterial()->setProperty("deferred", "mode", mode);
            d3d::settex(0, m_renderTargets.getRt(mode));

            m_debugRenderer->render();
        }
    }
    void DeferredRenderTarget::resolve(BaseTexture* resolveTarget,
                                       CubeTexture* irradianceMap,
                                       CubeTexture* reflectionMap,
                                       const math::Matrix4& view_proj_tm,
                                       BaseTexture* depth_bounds_tex,
                                       ShadingResolver::ClearTarget clear_target,
                                       const math::Matrix4& gbufferTm,
                                       const math::RectInt* resolve_area)
    {
        d3d::set_sampler(STAGE_PS, 0, m_renderTargets.getDefaultSampler());

        d3d::settex(0, m_renderTargets.getRt(0));
        d3d::settex(1, m_renderTargets.getRt(1));
        d3d::settex(2, m_renderTargets.getRt(2));
        d3d::settex(3, m_renderTargets.getDepth());
        d3d::settex(4, irradianceMap);
        d3d::settex(5, reflectionMap);

        m_shadingResolver->resolve(resolveTarget, view_proj_tm, depth_bounds_tex, clear_target, gbufferTm, resolve_area);
    }

    void DeferredRenderTarget::flushResolve()
    {
        d3d::settex(0, nullptr);
        d3d::settex(1, nullptr);
        d3d::settex(2, nullptr);
        d3d::settex(3, nullptr);
        d3d::settex(4, nullptr);
        d3d::settex(5, nullptr);
        d3d::settex(8, nullptr); // remove csm texture
    }

}  // namespace nau::render