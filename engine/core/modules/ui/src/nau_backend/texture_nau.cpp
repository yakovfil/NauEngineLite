// Copyright 2024 N-GINN LLC. All rights reserved.
/****************************************************************************
 Copyright (c) 2018-2019 Xiamen Yaji Software Co., Ltd.

 http://www.cocos2d-x.org

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 ****************************************************************************/

#include "texture_nau.h"

#include <memory>
#include <optional>
#include <span>

#include "base/CCDirector.h"
#include "base/CCEventDispatcher.h"
#include "base/CCEventListenerCustom.h"
#include "base/CCEventType.h"
#include "base/ccMacros.h"
#include "nau/3d/dag_lockTexture.h"
#include "platform/CCPlatformConfig.h"
#include "utils_nau.h"

DAGOR_CC_BACKEND_BEGIN

bool requiresConversion(cocos2d::backend::PixelFormat format)
{
    switch (format)
    {
        case cocos2d::backend::PixelFormat::RGBA4444:
        case cocos2d::backend::PixelFormat::RGB888:
            return true;
        default:
            return false;
    }
}
std::unique_ptr<uint8_t[]> convertData(cocos2d::backend::PixelFormat format, uint8_t* data, std::size_t width, std::size_t height)
{
    switch (format)
    {
        case cocos2d::backend::PixelFormat::RGBA4444:
        {
            struct element
            {
                std::byte r : 4;
                std::byte g : 4;
                std::byte b : 4;
                std::byte a : 4;
            };
            std::unique_ptr<uint8_t[]> newData{new uint8_t[width * height * sizeof(element)]};

            memcpy_s(newData.get(), width * height * sizeof(element), data, width * height * sizeof(element));

            element* newElements = reinterpret_cast<element*>(newData.get());

            for (int i = 0; i < width * height; ++i)
            {
                std::byte a = newElements[i].a;
                newElements[i].a = newElements[i].r;
                newElements[i].r = newElements[i].g;
                newElements[i].g = newElements[i].b;
                newElements[i].b = a;
            }
            return std::move(newData);
        }

        case cocos2d::backend::PixelFormat::RGB888:
        {
            struct element
            {
                uint8_t r : 8;
                uint8_t g : 8;
                uint8_t b : 8;
                uint8_t a : 8;
            };
            std::unique_ptr<uint8_t[]> newData{new uint8_t[width * height * sizeof(element)]};

            for (int i = 0; i < width * height; ++i)
            {
                newData[i * 4] = data[i * 3];
                newData[i * 4 + 1] = data[i * 3 + 1];
                newData[i * 4 + 2] = data[i * 3 + 2];
                newData[i * 4 + 3] = 1;
            }

            return std::move(newData);
        }
        default:
            return nullptr;
    }
}

void TextureInfoNau::applySamplerDescriptor(const cocos2d::backend::SamplerDescriptor& descriptor)
{
    sampler = d3d::create_sampler(
        {cocos_utils::toNauMipMapMode(descriptor.minFilter),
         cocos_utils::toNauFilter(descriptor.minFilter),
         cocos_utils::toNauFilter(descriptor.magFilter),
         false,
         cocos_utils::toNauAddressMode(descriptor.sAddressMode),
         cocos_utils::toNauAddressMode(descriptor.tAddressMode)});
}

Texture2DNau::Texture2DNau(const cocos2d::backend::TextureDescriptor& descriptor) :
    Texture2DBackend(descriptor)
{
    updateTextureDescriptor(descriptor);
}

void Texture2DNau::updateTextureDescriptor(const cocos2d::backend::TextureDescriptor& descriptor)
{
    TextureBackend::updateTextureDescriptor(descriptor);
    _textureInfo.format = cocos_utils::toNauTypes(descriptor.textureFormat);
    _textureInfo.mipmapNum = descriptor.mipmapNum;

    updateSamplerDescriptor(descriptor.samplerDescriptor);

    if (_textureInfo.texture)
    {
        _textureInfo.texture->destroy();
    }
    auto flags = cocos_utils::toNauTypes(_textureFormat) | TEXCF_CLEAR_ON_CREATE;

    if (descriptor.textureUsage == cocos2d::backend::TextureUsage::RENDER_TARGET)
    {
        flags |= TEXCF_RTARGET;
    }
    else
    {
        flags |= TEXCF_DYNAMIC;
    }

    if (_width * _height == 0)
    {
        _textureInfo.texture = nullptr;
        return;
    }

    _textureInfo.texture = d3d::create_tex(nullptr, _width, _height,
                                           flags,
                                           _textureInfo.mipmapNum);
}

Texture2DNau::~Texture2DNau()
{
    if (_textureInfo.texture)
    {
        _textureInfo.texture->destroy();
    }
    _textureInfo.texture = 0;
}

void Texture2DNau::updateSamplerDescriptor(const cocos2d::backend::SamplerDescriptor& sampler)
{
    _textureInfo.applySamplerDescriptor(sampler);
}

template <size_t dataLength = 4>
void writeDataToTexture(Texture* texture, uint8_t* data,  //
                        std::optional<int> layer,
                        std::size_t level,
                        std::size_t width,
                        std::size_t height,
                        std::size_t xoffset,
                        std::size_t yoffset)
{
    struct element
    {
        std::byte data[dataLength];
    };

    auto image = LockedImage<Image2DView<element>>::lock_texture(texture, layer, level, TEXLOCK_WRITE);
    for (int row = yoffset; row < yoffset + height; ++row)
    {
        image.writeElems(std::span(reinterpret_cast<element*>(data) + (row - yoffset) * width, width), row, xoffset, width);
    }
}

void Texture2DNau::updateData(uint8_t* data, std::size_t width, std::size_t height, std::size_t level)
{
    ::TextureInfo info;
    _textureInfo.texture->getinfo(info, level);
    std::unique_ptr<uint8_t[]> convertedData;
    if (requiresConversion(_textureFormat))
    {
        convertedData = convertData(_textureFormat, data, width, height);
        data = convertedData.get();
    }

    const auto& desc = get_tex_format_desc(info.cflg & TEXFMT_MASK);
    switch (desc.bytesPerElement)
    {
        case 1:
            writeDataToTexture<1>(_textureInfo.texture, data, std::nullopt, level, width, height, 0, 0);
            break;
        case 2:
            writeDataToTexture<2>(_textureInfo.texture, data, std::nullopt, level, width, height, 0, 0);
            break;
        case 3:
            writeDataToTexture<3>(_textureInfo.texture, data, std::nullopt, level, width, height, 0, 0);
            break;
        case 4:
            writeDataToTexture<4>(_textureInfo.texture, data, std::nullopt, level, width, height, 0, 0);
            break;
        case 5:
            writeDataToTexture<5>(_textureInfo.texture, data, std::nullopt, level, width, height, 0, 0);
            break;
        case 16:
            writeDataToTexture<16>(_textureInfo.texture, data, std::nullopt, level, width, height, 0, 0);
            break;
        default:
            NAU_FAILURE_ALWAYS("Unsupported formate {}.", get_tex_format_name(desc.dagorTextureFormat));
            break;
    }
}

void Texture2DNau::updateCompressedData(uint8_t* data, std::size_t width, std::size_t height, std::size_t dataLen, std::size_t level)
{
    NAU_FAILURE_ALWAYS("Compressed formats are unsupported.");
}

void Texture2DNau::updateSubData(std::size_t xoffset, std::size_t yoffset, std::size_t width, std::size_t height, std::size_t level, uint8_t* data)
{
    ::TextureInfo info;
    _textureInfo.texture->getinfo(info, level);

    std::unique_ptr<uint8_t[]> convertedData;
    if (requiresConversion(_textureFormat))
    {
        convertedData = convertData(_textureFormat, data, width, height);
        data = convertedData.get();
    }

    const auto& desc = get_tex_format_desc(info.cflg & TEXFMT_MASK);
    switch (desc.bytesPerElement)
    {
        case 1:
            writeDataToTexture<1>(_textureInfo.texture, data, std::nullopt, level, width, height, xoffset, yoffset);
            break;
        case 2:
            writeDataToTexture<2>(_textureInfo.texture, data, std::nullopt, level, width, height, xoffset, yoffset);
            break;
        case 3:
            writeDataToTexture<3>(_textureInfo.texture, data, std::nullopt, level, width, height, xoffset, yoffset);
            break;
        case 4:
            writeDataToTexture<4>(_textureInfo.texture, data, std::nullopt, level, width, height, xoffset, yoffset);
            break;
        case 5:
            writeDataToTexture<5>(_textureInfo.texture, data, std::nullopt, level, width, height, xoffset, yoffset);
            break;
        case 16:
            writeDataToTexture<16>(_textureInfo.texture, data, std::nullopt, level, width, height, xoffset, yoffset);
            break;
        default:
            NAU_FAILURE_ALWAYS("Unsupported formate {}.", get_tex_format_name(desc.dagorTextureFormat));
            break;
    }
}

void Texture2DNau::updateCompressedSubData(std::size_t xoffset, std::size_t yoffset, std::size_t width, std::size_t height, std::size_t dataLen, std::size_t level, uint8_t* data)
{
    NAU_FAILURE_ALWAYS("Compressed formats are unsupported.");
}

void Texture2DNau::apply(int index) const
{
    d3d::settex(index, _textureInfo.texture);
    d3d::set_sampler(STAGE_PS, 0, _textureInfo.sampler);
}

void Texture2DNau::generateMipmaps()
{
    _textureInfo.texture->generateMips();
}

void Texture2DNau::getBytes(std::size_t x, std::size_t y, std::size_t width, std::size_t height, bool flipImage, std::function<void(const unsigned char*, std::size_t, std::size_t)> callback)
{
    ::TextureInfo info;
    _textureInfo.texture->getinfo(info, 0);

    NAU_ASSERT((info.cflg & TEXFMT_MASK) == TEXFMT_R8G8B8A8);

    const unsigned char* data = new unsigned char[(width - x) * (height - y) * sizeof(math::E3DCOLOR)];
    callback(data, width - x, height - y);
    delete[] data;

    // TODO: using render thread and optimal API.
}

TextureCubeNau::TextureCubeNau(const cocos2d::backend::TextureDescriptor& descriptor) :
    TextureCubemapBackend(descriptor)
{
    updateTextureDescriptor(descriptor);
    NAU_ASSERT(_width == _height);
    _textureType = cocos2d::backend::TextureType::TEXTURE_CUBE;
}

void TextureCubeNau::updateTextureDescriptor(const cocos2d::backend::TextureDescriptor& descriptor)
{
    TextureBackend::updateTextureDescriptor(descriptor);
    _textureInfo.format = cocos_utils::toNauTypes(descriptor.textureFormat);
    _textureInfo.mipmapNum = descriptor.mipmapNum;

    updateSamplerDescriptor(descriptor.samplerDescriptor);

    if (_textureInfo.texture)
    {
        _textureInfo.texture->destroy();
    }

    _textureInfo.texture = d3d::create_cubetex(_width, cocos_utils::toNauTypes(_textureFormat) | TEXCF_CLEAR_ON_CREATE | TEXCF_DYNAMIC, _textureInfo.mipmapNum);
}

TextureCubeNau::~TextureCubeNau()
{
    if (_textureInfo.texture)
    {
        _textureInfo.texture->destroy();
    }
    _textureInfo.texture = nullptr;
}

void TextureCubeNau::updateSamplerDescriptor(const cocos2d::backend::SamplerDescriptor& sampler)
{
    _textureInfo.applySamplerDescriptor(sampler);
}

void TextureCubeNau::apply(int index) const
{
    d3d::settex(index, _textureInfo.texture);
    d3d::set_sampler(STAGE_PS, 0, _textureInfo.sampler);
}

void TextureCubeNau::updateFaceData(cocos2d::backend::TextureCubeFace side, void* data)
{
    ::TextureInfo info;
    _textureInfo.texture->getinfo(info);

    std::unique_ptr<uint8_t[]> convertedData;
    if (requiresConversion(_textureFormat))
    {
        convertedData = convertData(_textureFormat, reinterpret_cast<uint8_t*>(data), _width, _height);
        data = convertedData.get();
    }

    const auto& desc = get_tex_format_desc(info.cflg & TEXFMT_MASK);
    switch (desc.bytesPerElement)
    {
        case 1:
            writeDataToTexture<1>(_textureInfo.texture, (uint8_t*)data, uint32_t(side), 0, _width, _height, 0, 0);
            break;
        case 2:
            writeDataToTexture<2>(_textureInfo.texture, (uint8_t*)data, uint32_t(side), 0, _width, _height, 0, 0);
            break;
        case 3:
            writeDataToTexture<3>(_textureInfo.texture, (uint8_t*)data, uint32_t(side), 0, _width, _height, 0, 0);
            break;
        case 4:
            writeDataToTexture<4>(_textureInfo.texture, (uint8_t*)data, uint32_t(side), 0, _width, _height, 0, 0);
            break;
        case 5:
            writeDataToTexture<5>(_textureInfo.texture, (uint8_t*)data, uint32_t(side), 0, _width, _height, 0, 0);
            break;
        case 16:
            writeDataToTexture<16>(_textureInfo.texture, (uint8_t*)data, uint32_t(side), 0, _width, _height, 0, 0);
            break;
        default:
            NAU_FAILURE_ALWAYS("Unsupported formate {}.", get_tex_format_name(desc.dagorTextureFormat));
            break;
    }
}

void TextureCubeNau::getBytes(std::size_t x, std::size_t y, std::size_t width, std::size_t height, bool flipImage, std::function<void(const unsigned char*, std::size_t, std::size_t)> callback)
{
    ::TextureInfo info;
    _textureInfo.texture->getinfo(info, 0);

    NAU_ASSERT((info.cflg & TEXFMT_MASK) == TEXFMT_R8G8B8A8);

    const unsigned char* data = new unsigned char[(width - x) * (height - y) * sizeof(math::E3DCOLOR)];
    callback(data, width - x, height - y);
    delete[] data;

    // TODO: using render thread and optimal API.
}

void TextureCubeNau::generateMipmaps()
{
    _textureInfo.texture->generateMips();
}

DAGOR_CC_BACKEND_END
