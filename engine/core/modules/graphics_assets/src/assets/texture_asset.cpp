// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "../../include/graphics_assets/texture_asset.h"

#include <vector>

#include "nau/assets/texture_asset_accessor.h"

#define LOAD_TEXTURE_ASYNC

namespace nau
{
    namespace
    {

        inline uint32_t getDagorFormat(TinyImageFormat tinyFormat)
        {
            if (tinyFormat == TinyImageFormat_B8G8R8A8_UNORM)
            {
                return TEXFMT_DEFAULT;
            }
            else  if(tinyFormat == TinyImageFormat_R8G8B8A8_UNORM)
            {
                return TEXFMT_R8G8B8A8;
            }
            else if(tinyFormat == TinyImageFormat_R10G10B10A2_UNORM)
            {
                return TEXFMT_A2R10G10B10;
            }
            else if(tinyFormat == TinyImageFormat_R16G16B16A16_SFLOAT)
            {
                return TEXFMT_A16B16G16R16F;
            }
            else if(tinyFormat == TinyImageFormat_R16G16B16_SFLOAT)
            {
                return TEXFMT_A16B16G16R16F;
            }
            else if(tinyFormat == TinyImageFormat_R32G32B32A32_SFLOAT)
            {
                return TEXFMT_A32B32G32R32F;
            }
            else if(tinyFormat == TinyImageFormat_DXBC1_RGB_UNORM || tinyFormat == TinyImageFormat_DXBC1_RGBA_UNORM)
            {
                return TEXFMT_DXT1;
            }
            else if(tinyFormat == TinyImageFormat_DXBC3_UNORM)
            {
                return TEXFMT_DXT5;
            }
            else if(tinyFormat == TinyImageFormat_DXBC6H_SFLOAT)
            {
                return TEXFMT_BC6H;
            }
            else if(tinyFormat == TinyImageFormat_DXBC7_UNORM)
            {
                return TEXFMT_BC7;
            }

            NAU_FAILURE("Unknown Texture tiny Format: ({})", static_cast<std::underlying_type_t<TinyImageFormat>>(tinyFormat));

            return DXGI_FORMAT_UNKNOWN;
        }
    }  // namespace

    async::Task<nau::Ptr<TextureAssetView>> TextureAssetView::createFromAssetAccessor(nau::Ptr<> accessor)
    {
        using namespace nau::async;

        NAU_ASSERT(accessor);

#ifdef LOAD_TEXTURE_ASYNC
        // ASYNC_SWITCH_EXECUTOR should not be used here:
        // it is necessary to change the execution thread in any case, even if the execution is already taking place in the pool.
        co_await async::Executor::getDefault();
#endif

        auto& textureAccessor = accessor->as<ITextureAssetAccessor&>();
        auto textureAssetView = rtti::createInstance<TextureAssetView>();
        const auto& imageDesc = textureAccessor.getDescription();

        BaseTexture* tex = nullptr;

        const uint32_t           dagorFormat     = getDagorFormat(imageDesc.format);
        const TextureFormatDesc& dagorFormatDesc = get_tex_format_desc(dagorFormat);

        tex = d3d::create_tex(nullptr, imageDesc.width, imageDesc.height, dagorFormat, imageDesc.numMipmaps);


        for(int mipLevel = 0; mipLevel < imageDesc.numMipmaps; ++mipLevel)
        {
            TextureInfo info;
            int t = tex->getinfo(info, mipLevel);

            void* texDataPtr = nullptr;
            int stride;
            tex->lockimg(&texDataPtr, stride, mipLevel, TEXLOCK_WRITE);

            std::vector<DestTextureData> dstData(1);
            DestTextureData& data = dstData[0];

            data.outputBuffer = texDataPtr;
            data.rowsCount    = std::max(1, info.h / dagorFormatDesc.elementHeight);
            data.rowPitch     = stride;
            data.rowBytesSize = 0; // will use format's default row bytes size
            data.slicePitch   = 0;

            textureAccessor.copyTextureData(mipLevel, 1, dstData);

            tex->unlockimg();
        }

        textureAssetView->m_Texture = tex;

        co_return textureAssetView;
    }

}  // namespace nau

#ifdef LOAD_TEXTURE_ASYNC
    #undef LOAD_TEXTURE_ASYNC
#endif
