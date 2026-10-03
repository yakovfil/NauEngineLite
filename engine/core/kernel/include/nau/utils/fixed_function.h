// Copyright 2026 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once

#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>

namespace nau
{
    template <std::size_t Capacity, typename Signature>
    class FixedFunction;

    // Copyable bounded storage. Like std::function, const invocation can invoke
    // a mutable target. No capture ever spills to heap storage.
    template <std::size_t Capacity, typename R, typename... Args>
    class FixedFunction<Capacity, R(Args...)>
    {
        static_assert(Capacity > 0);
        static constexpr std::size_t Alignment = 16;
        struct Operations
        {
            R (*invoke)(void*, Args...);
            void (*copy)(void*, const void*);
            void (*move)(void*, void*);
            void (*destroy)(void*) noexcept;
        };
        template <typename F>
        static const Operations* operations()
        {
            static const Operations ops{
                [](void* pointer, Args... args) -> R
            {
                if constexpr (std::is_void_v<R>)
                    std::invoke(*static_cast<F*>(pointer), std::forward<Args>(args)...);
                else
                    return std::invoke(*static_cast<F*>(pointer), std::forward<Args>(args)...);
            },
                [](void* to, const void* from)
            {
                std::construct_at(static_cast<F*>(to), *static_cast<const F*>(from));
            },
                [](void* to, void* from)
            {
                std::construct_at(static_cast<F*>(to), std::move(*static_cast<F*>(from)));
            },
                [](void* pointer) noexcept
            {
                std::destroy_at(static_cast<F*>(pointer));
            }};
            return &ops;
        }

    public:
        FixedFunction() noexcept = default;
        FixedFunction(std::nullptr_t) noexcept
        {
        }
        template <typename F>
        requires(!std::is_same_v<std::decay_t<F>, FixedFunction> &&
                 std::is_copy_constructible_v<std::decay_t<F>> &&
                 std::is_constructible_v<std::decay_t<F>, F> &&
                 std::is_invocable_r_v<R, std::decay_t<F>&, Args...> &&
                 sizeof(std::decay_t<F>) <= Capacity && alignof(std::decay_t<F>) <= Alignment)
        FixedFunction(F&& function)
        {
            using Target = std::decay_t<F>;
            if constexpr (std::is_pointer_v<Target> || std::is_member_pointer_v<Target>)
                if (function == nullptr)
                    return;
            std::construct_at(reinterpret_cast<Target*>(m_storage), std::forward<F>(function));
            m_operations = operations<Target>();
        }
        FixedFunction(const FixedFunction& other)
        {
            if (other.m_operations)
            {
                other.m_operations->copy(m_storage, other.m_storage);
                m_operations = other.m_operations;
            }
        }
        FixedFunction(FixedFunction&& other)
        {
            if (other.m_operations)
            {
                other.m_operations->move(m_storage, other.m_storage);
                m_operations = other.m_operations;
                other.reset();
            }
        }
        ~FixedFunction()
        {
            reset();
        }
        FixedFunction& operator=(const FixedFunction& other)
        {
            if (this != &other)
            {
                FixedFunction copy(other);
                *this = std::move(copy);
            }
            return *this;
        }
        FixedFunction& operator=(FixedFunction&& other)
        {
            if (this != &other)
            {
                reset();
                if (other.m_operations)
                {
                    other.m_operations->move(m_storage, other.m_storage);
                    m_operations = other.m_operations;
                    other.reset();
                }
            }
            return *this;
        }
        FixedFunction& operator=(std::nullptr_t) noexcept
        {
            reset();
            return *this;
        }
        void reset() noexcept
        {
            if (auto* ops = std::exchange(m_operations, nullptr))
                ops->destroy(m_storage);
        }
        void swap(FixedFunction& other)
        {
            if (this == &other)
                return;
            FixedFunction temporary(std::move(*this));
            *this = std::move(other);
            other = std::move(temporary);
        }
        explicit operator bool() const noexcept
        {
            return m_operations != nullptr;
        }
        bool operator==(std::nullptr_t) const noexcept
        {
            return !m_operations;
        }
        R operator()(Args... args) const
        {
            if (!m_operations)
            {
#if defined(__cpp_exceptions) || defined(_CPPUNWIND)
                throw std::bad_function_call();
#else
                std::fputs("Nau FixedFunction invocation requires a callable\n", stderr);
                std::abort();
#endif
            }
            return m_operations->invoke(m_storage, std::forward<Args>(args)...);
        }

    private:
        const Operations* m_operations = nullptr;
        alignas(Alignment) mutable std::byte m_storage[Capacity];
    };
}  // namespace nau
