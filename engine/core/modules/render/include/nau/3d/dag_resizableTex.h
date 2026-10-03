// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#pragma once

#include <cstdint>
#include <utility>

#include "dag_resPtr.h"
#include "nau/utils/sorted_vector.h"

// dynamically resizable texture baked by single memory area (heap)
// resize to smaller size works only when heaps & resource aliasing are supported

namespace resptr_detail
{

    using key_t = uint32_t;

    class NAU_RENDER_EXPORT ResizableManagedTex : public ManagedTex
    {
    protected:
        nau::SortedVectorMap<key_t, UniqueTex> mAliases;

        void swap(ResizableManagedTex& other);
        ResizableManagedTex() = default;

    public:
        ResizableManagedTex(const ResizableManagedTex&) = delete;
        ResizableManagedTex& operator=(const ResizableManagedTex&) = delete;
        void resize(int width, int height);
    };

    class NAU_RENDER_EXPORT ResizableUnmanagedTex : public TexPtr
    {
    protected:
        void swap(ResizableUnmanagedTex& other);
        nau::SortedVectorMap<key_t, TexPtr> mAliases;

    public:
        ResizableUnmanagedTex() = default;
        ResizableUnmanagedTex(ResizableUnmanagedTex&& other) noexcept
        {
            this->swap(other);
        }
        ResizableUnmanagedTex& operator=(ResizableUnmanagedTex&& other)
        {
            ResizableUnmanagedTex(std::move(other)).swap(*this);
            return *this;
        }
        ResizableUnmanagedTex(TexPtr&& other) noexcept
        {
            this->TexPtr::swap(other);
        }
        ResizableUnmanagedTex& operator=(TexPtr&& other)
        {
            TexPtr(std::move(other)).swap(*this);
            return *this;
        }
        void close()
        {
            *this = ResizableUnmanagedTex();
        };

        void resize(int width, int height);
    };

    class ResizableManagedTexHolder : public ManagedResHolder<ResizableManagedTex>
    {
    protected:
        ResizableManagedTexHolder() = default;

    public:
        void resize(int width, int height)
        {
            this->ResizableManagedTex::resize(width, height);
            this->setVar();
        }
    };

    using ResizableTex = UniqueRes<ResizableManagedTex>;
    using ResizableTexHolder = ConcreteResHolder<UniqueRes<ResizableManagedTexHolder>>;

}  // namespace resptr_detail

using ResizableTex = resptr_detail::ResizableTex;
using ResizableTexHolder = resptr_detail::ResizableTexHolder;

using ResizableResPtrTex = resptr_detail::ResizableUnmanagedTex;