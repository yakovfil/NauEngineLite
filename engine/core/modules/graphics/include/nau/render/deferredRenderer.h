// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#pragma once

#include <array>
#include <memory>

#include "dag_postFxRenderer.h"
#include "nau/math/dag_e3dColor.h"
#include "nau/render/deferredRT.h"
#include "nau/shaders/shader_globals.h"

extern const int USE_DEBUG_GBUFFER_MODE;

namespace nau::render
{
    class ShadingResolver
    {
    public:
        enum class ClearTarget
        {
            No,
            Yes
        };

        ShadingResolver(std::unique_ptr<PostFxRenderer> resolveShading);
        ~ShadingResolver();

        PostFxRenderer* getResolveShading() const
        {
            return resolveShading.get();
        }

        void resolve(BaseTexture* resolveTarget,
                     const math::Matrix4& view_proj_tm,
                     BaseTexture* depth_bounds_tex = nullptr,
                     ShadingResolver::ClearTarget clear_target = ShadingResolver::ClearTarget::No,
                     const math::Matrix4& gbufferTm = math::Matrix4::identity(),
                     const math::RectInt* resolve_area = nullptr);

    private:
        std::unique_ptr<PostFxRenderer> resolveShading;
    };

    class DeferredRenderTarget
    {
    public:
        DeferredRenderTarget(std::unique_ptr<render::ShadingResolver> resolveShading,
                             const char* name,
                             int w,
                             int h,
                             DeferredRT::StereoMode stereo_mode,
                             unsigned msaaFlag,
                             int numRt,
                             const unsigned* texFmt,
                             uint32_t depthFmt);

        static inline unsigned g_defaultGBufferFormats[] = {
            TEXFMT_A8R8G8B8 | TEXCF_SRGBREAD | TEXCF_SRGBWRITE,
            TEXFMT_A32B32G32R32F,
            TEXFMT_R8};

        enum DefaultGBuffer
        {
            GBUF_ALBEDO_AO,
            GBUF_NORMAL_ROUGH_MET,
            GBUF_MATERIAL,
            GBUF_NUM
        };

        DeferredRenderTarget(std::unique_ptr<render::ShadingResolver> resolveShading,
                             const char* name,
                             int w,
                             int h,
                             DeferredRT::StereoMode stereo_mode,
                             uint32_t depthFmt);

        ~DeferredRenderTarget();
        DeferredRenderTarget(const DeferredRenderTarget&) = delete;
        DeferredRenderTarget& operator=(const DeferredRenderTarget&) = delete;

        void resolve(BaseTexture* resolveTarget, CubeTexture* irradianceMap, CubeTexture* reflectionMap, const math::Matrix4& view_proj_tm, BaseTexture* depth_bounds_tex = nullptr, ShadingResolver::ClearTarget clear_target = ShadingResolver::ClearTarget::No, const math::Matrix4& gbufferTm = math::Matrix4::identity(), const math::RectInt* resolve_area = nullptr);
        void flushResolve();

        void resourceBarrier(ResourceBarrier barrier);

        inline void setRt()
        {
            m_renderTargets.setRt();
        }
        inline void changeResolution(const int w, const int h)
        {
            m_renderTargets.changeResolution(w, h);
        }
        void debugRender(int show_gbuffer = USE_DEBUG_GBUFFER_MODE);

        // returns true if 32 bit depth buffer was created
        inline uint32_t recreateDepth(uint32_t fmt)
        {
            return m_renderTargets.recreateDepth(fmt);
        }
        inline int getWidth() const
        {
            return m_renderTargets.getWidth();
        }
        inline int getHeight() const
        {
            return m_renderTargets.getHeight();
        }
        inline Texture* getDepth() const
        {
            return m_renderTargets.getDepth();
        }
        inline TEXTUREID getDepthId() const
        {
            return m_renderTargets.getDepthId();
        }
        inline const TexPtr& getDepthAll() const
        {
            return m_renderTargets.getDepthAll();
        }
        inline PostFxRenderer* getResolveShading() const
        {
            return m_shadingResolver->getResolveShading();
        }
        inline Texture* getRt(uint32_t idx) const
        {
            return m_renderTargets.getRt(idx);
        }
        inline TEXTUREID getRtId(uint32_t idx) const
        {
            return m_renderTargets.getRtId(idx);
        }
        inline const TexPtr& getRtAll(uint32_t idx) const
        {
            return m_renderTargets.getRtAll(idx);
        }
        inline uint32_t getRtNum() const
        {
            return m_renderTargets.getRtNum();
        }

    protected:
        DeferredRT m_renderTargets;
        std::unique_ptr<ShadingResolver> m_shadingResolver;
        PostFxRenderer* m_debugRenderer = nullptr;
    };

}  // namespace nau::render