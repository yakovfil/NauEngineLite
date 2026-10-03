// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#pragma once

#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <new>
#include <type_traits>
#include <utility>

namespace nau
{

    /**
     * \brief A class template for a fixed-size move-only function object.
     * Preserves const-correctness and does not require the function object
     * to be copyable.
     * \details Use FixedMoveOnlyFunction<16, void() const> for a const
     * callable function object and FixedMoveOnlyFunction<16, void()> for
     * a non-const callable one.
     *
     * \tparam Signature The type-erased signature of the function.
     * \tparam size The size of the in-place storage in bytes.
     */
    template <size_t size, typename Signature>
    class FixedMoveOnlyFunction;

    namespace detail
    {

        template <size_t size, typename CallSignature>
        struct FixedMoveOnlyFunctionBase
        {
            using ManagerSignature = void(void*, void*);

            static constexpr size_t align = 16;

            [[noreturn]] static void emptyCall() noexcept
            {
                std::fputs("Nau FixedMoveOnlyFunction invocation requires a callable\n", stderr);
                std::abort();
            }

            template <typename F>
            static void relocateImpl(void* from, void* to)
            {
                if (to != nullptr)
                    new(to) F{(F&&)*static_cast<F*>(from)};

                static_cast<F*>(from)->~F();
            }

            FixedMoveOnlyFunctionBase() :
                call{nullptr},
                relocate{nullptr}
            {
            }

            template <typename F>
            FixedMoveOnlyFunctionBase(F&& func_object) :
                call{nullptr}
            {
                using UnqualF = std::remove_cvref_t<F>;

                static_assert(sizeof(UnqualF) <= size, "Function object is too big!");
                static_assert(alignof(UnqualF) <= align, "Function object over-aligned!");

                relocate = &relocateImpl<UnqualF>;
                new(storage) UnqualF{(F&&)func_object};
            }

            FixedMoveOnlyFunctionBase(const FixedMoveOnlyFunctionBase&) = delete;
            FixedMoveOnlyFunctionBase& operator=(const FixedMoveOnlyFunctionBase&) = delete;

            FixedMoveOnlyFunctionBase(FixedMoveOnlyFunctionBase&& other) :
                call{other.call},
                relocate{other.relocate}  //-V1077
            {
                if (other.call == nullptr)
                    return;

                relocate(&other.storage, storage);
                other.call = nullptr;
            }

            FixedMoveOnlyFunctionBase& operator=(FixedMoveOnlyFunctionBase&& other)
            {
                if (this == &other)
                    return *this;

                if (call != nullptr)
                    relocate(&storage, nullptr);

                call = other.call;

                if (other.call == nullptr)
                    return *this;

                relocate = other.relocate;
                relocate(&other.storage, storage);

                other.call = nullptr;

                return *this;
            }

            ~FixedMoveOnlyFunctionBase()
            {
                if (call != nullptr)
                    relocate(&storage, nullptr);
            }

            void reset()
            {
                if (call != nullptr)
                    relocate(&storage, nullptr);

                call = nullptr;
            }

            explicit operator bool() const
            {
                return call != nullptr;
            }

            // call should be hot
            CallSignature* call;
            ManagerSignature* relocate;
            // On win32, we still want this to be aligned to 16 bytes
            alignas(align) char storage[size];
        };

        template <typename>
        struct IsFixedMoveOnlyFunction : public std::false_type
        {
        };

        template <size_t size, typename Signature>
        struct IsFixedMoveOnlyFunction<FixedMoveOnlyFunction<size, Signature>> : public std::true_type
        {
        };

        template <typename T>
        constexpr bool is_fixed_move_only_function_v = IsFixedMoveOnlyFunction<T>::value;

    }  // namespace detail

    template <size_t size, typename Ret, typename... Args>
    class FixedMoveOnlyFunction<size, Ret(Args...)> : private detail::FixedMoveOnlyFunctionBase<size, Ret(void*, Args...)>
    {
        using Base = detail::FixedMoveOnlyFunctionBase<size, Ret(void*, Args...)>;

        template <typename F>
        static Ret callImpl(void* func_object, Args... args)
        {
            if constexpr (std::is_void_v<Ret>)
                std::invoke(*static_cast<F*>(func_object), std::forward<Args>(args)...);
            else
                return std::invoke(*static_cast<F*>(func_object), std::forward<Args>(args)...);
        }

        using Base::call;
        using Base::storage;

    public:
        FixedMoveOnlyFunction() = default;

        template <typename F>
        requires(!detail::is_fixed_move_only_function_v<std::decay_t<F>> &&
                 std::is_invocable_r_v<Ret, std::decay_t<F>&, Args...> &&
                 std::is_constructible_v<std::decay_t<F>, F> &&
                 sizeof(std::decay_t<F>) <= size && alignof(std::decay_t<F>) <= Base::align)
        FixedMoveOnlyFunction(F&& func_object) :
            Base((F&&)func_object)
        {
            call = &callImpl<std::decay_t<F>>;
        }

        FixedMoveOnlyFunction& operator=(std::nullptr_t)
        {
            this->reset();
            return *this;
        }
        using Base::operator bool;

        Ret operator()(Args... args)
        {
            if (!call)
                Base::emptyCall();
            return call(storage, std::forward<Args>(args)...);
        }
    };

    template <size_t size, typename Ret, typename... Args>
    class FixedMoveOnlyFunction<size, Ret(Args...) const> : private detail::FixedMoveOnlyFunctionBase<size, Ret(void const*, Args...)>
    {
        using Base = detail::FixedMoveOnlyFunctionBase<size, Ret(void const*, Args...)>;

        template <typename F>
        static Ret callImpl(void const* func_object, Args... args)
        {
            // The "const" word on the next line is central to preserving const-correctness.
            if constexpr (std::is_void_v<Ret>)
                std::invoke(*static_cast<F const*>(func_object), std::forward<Args>(args)...);
            else
                return std::invoke(*static_cast<F const*>(func_object), std::forward<Args>(args)...);
        }

        using Base::call;
        using Base::storage;

    public:
        FixedMoveOnlyFunction() :
            Base()
        {
        }

        template <typename F>
        requires(!detail::is_fixed_move_only_function_v<std::decay_t<F>> &&
                 std::is_invocable_r_v<Ret, const std::decay_t<F>&, Args...> &&
                 std::is_constructible_v<std::decay_t<F>, F> &&
                 sizeof(std::decay_t<F>) <= size && alignof(std::decay_t<F>) <= Base::align)
        FixedMoveOnlyFunction(F&& func_object) :
            Base((F&&)func_object)
        {
            call = &callImpl<std::decay_t<F>>;
        }

        FixedMoveOnlyFunction& operator=(std::nullptr_t)
        {
            this->reset();
            return *this;
        }
        using Base::operator bool;

        Ret operator()(Args... args) const
        {
            if (!call)
                Base::emptyCall();
            return call(storage, ((Args&&)args)...);
        }
    };

}  // namespace nau
