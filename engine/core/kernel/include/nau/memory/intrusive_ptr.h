// Copyright 2026 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once

#include <concepts>
#include <cstddef>
#include <utility>

namespace nau
{
    template <typename T>
    void intrusive_ptr_add_ref(T* pointer)
    {
        pointer->AddRef();
    }
    template <typename T>
    void intrusive_ptr_release(T* pointer)
    {
        pointer->Release();
    }

    // Ownership and synchronization remain in the object's hooks. No control
    // block is allocated; false adopts one reference already held by the caller.
    template <typename T>
    class IntrusivePtr
    {
    public:
        using element_type = T;
        IntrusivePtr() noexcept = default;
        IntrusivePtr(std::nullptr_t) noexcept
        {
        }
        IntrusivePtr(T* pointer, bool addReference = true) :
            m_pointer(pointer)
        {
            if (m_pointer && addReference)
                intrusive_ptr_add_ref(m_pointer);
        }
        IntrusivePtr(const IntrusivePtr& other) :
            IntrusivePtr(other.get())
        {
        }
        template <typename U>
        requires std::convertible_to<U*, T*>
        IntrusivePtr(const IntrusivePtr<U>& other) :
            IntrusivePtr(other.get())
        {
        }
        IntrusivePtr(IntrusivePtr&& other) noexcept :
            m_pointer(other.detach())
        {
        }
        template <typename U>
        requires std::convertible_to<U*, T*>
        IntrusivePtr(IntrusivePtr<U>&& other) noexcept :
            m_pointer(other.detach())
        {
        }
        ~IntrusivePtr()
        {
            if (m_pointer)
                intrusive_ptr_release(m_pointer);
        }
        IntrusivePtr& operator=(IntrusivePtr other) noexcept
        {
            swap(other);
            return *this;
        }
        void reset(T* pointer = nullptr, bool addReference = true)
        {
            IntrusivePtr(pointer, addReference).swap(*this);
        }
        T* detach() noexcept
        {
            return std::exchange(m_pointer, nullptr);
        }
        T* get() const noexcept
        {
            return m_pointer;
        }
        T* operator->() const noexcept
        {
            return m_pointer;
        }
        T& operator*() const noexcept
        {
            return *m_pointer;
        }
        explicit operator bool() const noexcept
        {
            return m_pointer != nullptr;
        }
        void swap(IntrusivePtr& other) noexcept
        {
            std::swap(m_pointer, other.m_pointer);
        }
        template <typename U>
        bool operator==(const IntrusivePtr<U>& other) const noexcept
        {
            return m_pointer == other.get();
        }
        bool operator==(std::nullptr_t) const noexcept
        {
            return m_pointer == nullptr;
        }

    private:
        T* m_pointer = nullptr;
    };

    template <typename T>
    void swap(IntrusivePtr<T>& first, IntrusivePtr<T>& second) noexcept
    {
        first.swap(second);
    }
}  // namespace nau
