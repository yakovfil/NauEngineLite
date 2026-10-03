// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved

#include <algorithm>
#include <string>
#include <utility>

#include "nau/3d/dag_resizableTex.h"
#include "nau/math/dag_adjpow2.h"

namespace resptr_detail
{

    static key_t make_key(uint32_t width, uint32_t height)
    {
        return (width << 16) | height;
    }

    static void break_key(uint32_t widthHeight, uint32_t& width, uint32_t& height)
    {
        width = widthHeight >> 16;
        height = widthHeight - (width << 16);
    }

    void ResizableManagedTex::swap(ResizableManagedTex& other)
    {
        ManagedTex::swap(other);
        std::swap(mAliases, other.mAliases);
    }

    void ResizableManagedTex::resize(int width, int height)
    {
        NAU_ASSERT(this->mResource);
        if (!this->mResource)
            return;

        TextureInfo tex_info;
        this->mResource->getinfo(tex_info);
        key_t my_key = make_key(tex_info.w, tex_info.h);
        key_t new_key = make_key(width, height);

        if (my_key == new_key)
            return;

        char* texture_name = (char*)this->mResource->getTexName();
        std::string managed_name;
        managed_name = nau::utils::format("{}-{}x{}", texture_name, width, height);

        UniqueTex this_tex;
        this->ManagedTex::swap(this_tex);

        bool inserted = mAliases.emplace(my_key, std::move(this_tex)).second;
        NAU_ASSERT(inserted);
        G_UNUSED(inserted);

        const UniqueTex& bigest_texture = mAliases.rbegin()->second;
        const int max_mips = std::min(nau::math::get_log2w(width), nau::math::get_log2w(height)) + 1;
        int mip_levels = std::min(bigest_texture->level_count(), max_mips);

        auto found = mAliases.lower_bound(new_key);
        if (found == mAliases.end())
        {
            NAU_LOG_DEBUG("Resizing {} to larger size than it has. This texture will be recreated", texture_name);
            mAliases.clear();
            TexPtr tex = dag::create_tex(nullptr, width, height, tex_info.cflg, mip_levels, texture_name);
            this_tex = UniqueTex(std::move(tex), managed_name.c_str());
        }
        else if (found->first == new_key)
        {
            this_tex = std::move(found->second);
            mAliases.erase(found->first);
        }
        else
        {
            TexPtr tex = dag::alias_tex(found->second.getTex2D(), nullptr, width, height, tex_info.cflg, mip_levels, texture_name);
            if (tex)
            {
                d3d::resource_barrier({
                    {found->second.getTex2D(),               tex.get()},
                    {           RB_ALIAS_FROM, RB_ALIAS_TO_AND_DISCARD},
                    {                       0,                       0},
                    {                       0,                       0}
                });
                this_tex = UniqueTex(std::move(tex), managed_name.c_str());
            }
            else
            {
                NAU_LOG_ERROR("d3d::alias_tex() not supported");
                this_tex = std::move(found->second);
                mAliases.erase(found->first);
            }
        }

        this->ManagedTex::swap(this_tex);
        return;
    }

    void ResizableUnmanagedTex::swap(ResizableUnmanagedTex& other)
    {
        TexPtr::swap(other);
        std::swap(mAliases, other.mAliases);
    }

    void ResizableUnmanagedTex::resize(int width, int height)
    {
        NAU_ASSERT(this->get());
        if (!this->get())
            return;

        TextureInfo tex_info;
        this->get()->getinfo(tex_info);
        key_t my_key = make_key(tex_info.w, tex_info.h);
        key_t new_key = make_key(width, height);

        if (my_key == new_key)
            return;

        char* texture_name = (char*)this->get()->getTexName();

        TexPtr this_tex;
        this->TexPtr::swap(this_tex);

        bool inserted = mAliases.emplace(my_key, std::move(this_tex)).second;
        NAU_ASSERT(inserted);
        G_UNUSED(inserted);

        const TexPtr& bigest_texture = mAliases.rbegin()->second;
        const int max_mips = std::min(nau::math::get_log2w(width), nau::math::get_log2w(height)) + 1;
        int mip_levels = std::min(bigest_texture->level_count(), max_mips);

        auto reallocateTexture = [&, this]()
        {
            int realWidth = std::max<int>(width, tex_info.w);
            int realHeight = std::max<int>(height, tex_info.h);
            NAU_LOG_DEBUG("Resizing {} to larger size than it has. This texture will be recreated", texture_name);
            mAliases.clear();
            TexPtr tex = dag::create_tex(nullptr,
                                         realWidth,
                                         realHeight,
                                         tex_info.cflg, mip_levels, texture_name);
            bool inserted = mAliases.emplace(make_key(realWidth, realHeight), std::move(tex)).second;
            NAU_ASSERT(inserted);
        };

        auto found = mAliases.lower_bound(new_key);
        if (found != mAliases.end())
        {
            if (found->first != new_key)
            {
                uint32_t keyWidth;
                uint32_t keyHeight;
                break_key(found->first, keyWidth, keyHeight);
                if ((width > keyWidth) || (height > keyHeight))
                {
                    reallocateTexture();
                }
            }
        }
        else
        {
            reallocateTexture();
        }

        found = mAliases.lower_bound(new_key);
        NAU_ASSERT(found != mAliases.end());

        if (found->first == new_key)
        {
            this_tex = std::move(found->second);
            mAliases.erase(found->first);
        }
        else
        {
            TexPtr tex = dag::alias_tex(found->second.get(), nullptr, width, height, tex_info.cflg, mip_levels, texture_name);
            if (tex)
            {
                d3d::resource_barrier({
                    {found->second.get(),               tex.get()},
                    {      RB_ALIAS_FROM, RB_ALIAS_TO_AND_DISCARD},
                    {                  0,                       0},
                    {                  0,                       0}
                });
                this_tex = TexPtr(std::move(tex));
            }
            else
            {
                NAU_LOG_ERROR("d3d::alias_tex() not supported");
                this_tex = std::move(found->second);
                mAliases.erase(found->first);
            }
        }

        this->TexPtr::swap(this_tex);
    }

}  // namespace resptr_detail