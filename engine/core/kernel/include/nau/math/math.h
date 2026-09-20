// Copyright 2024 N-GINN LLC. All rights reserved.
// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved

#pragma once

#include "vectormath/vec2d.hpp"

// clang-format off

#include "nau/math/dag_intrin.h"
#include "vectormath/vectormath.hpp"
#include "vectormath/vec2d.hpp"
#include "nau/serialization/runtime_value_builder.h"

// clang-format on

namespace nau::math
{
    using namespace Vectormath;

    using Vectormath::length;
    using Vectormath::lengthSqr;
    using Vectormath::max;
    using Vectormath::min;
    using Vectormath::normalize;

#if (VECTORMATH_CPU_HAS_SSE1_OR_BETTER && !VECTORMATH_FORCE_SCALAR_MODE)
    using namespace Vectormath::SSE::vector4int;
    using Vectormath::SSE::length;
    using Vectormath::SSE::lengthSqr;
    using Vectormath::SSE::normalize;
#elif !(VECTORMATH_CPU_HAS_NEON && !VECTORMATH_FORCE_SCALAR_MODE)
    using namespace Vectormath::Scalar::vector4int;
    using Vectormath::Scalar::length;
    using Vectormath::Scalar::lengthSqr;
    using Vectormath::Scalar::normalize;
#endif

    using vec2 = Vector2;

    using ivec2 = IVector2;
    using ivec3 = IVector3;

#ifdef MATH_USE_DOUBLE_PRECISION
    using vec3 = Vector3d;
    using vec4 = Vector4d;

    using mat3 = Matrix3d;
    using mat4 = Matrix4d;
#else
    using vec3 = Vector3;
    using vec4 = Vector4;

    using quat = Quat;

    using mat3 = Matrix3;
    using mat4 = Matrix4;
#endif

    using mat2 = Matrix2;

    inline float fsel(float a, float b, float c)
    {
        return (a >= 0.0f) ? b : c;
    }
    inline double fsel(double a, double b, double c)
    {
        return (a >= 0.0) ? b : c;
    }

#include <float.h>
    // msvc just does not optimize fast math
    NAU_FORCE_INLINE bool check_finite(float a)
    {
        return isfinite(a);
    }
    NAU_FORCE_INLINE bool check_nan(float a)
    {
        return isnan(a);
    }
    NAU_FORCE_INLINE bool check_finite(double a)
    {
        return isfinite(a);
    }
    NAU_FORCE_INLINE bool check_nan(double a)
    {
        return isnan(a);
    }
}  // namespace nau::math

namespace nau::math
{
    template <typename T>
    concept FloatCompatible = std::is_convertible_v<T, float>;

    // clang-format off
    template <typename T>
    concept LikeVec4 = requires(const T& vec) {
        { vec.getElem(int{}) } -> FloatCompatible;
        { vec.getX() } -> FloatCompatible;
        { vec.getY() } -> FloatCompatible;
        { vec.getZ() } -> FloatCompatible;
        { vec.getW() } -> FloatCompatible;
    } && requires(T& vec) {
        vec.setElem(int{}, float{});
        vec.setX(float{});
        vec.setY(float{});
        vec.setZ(float{});
        vec.setW(float{});
    };
    // clang-format on

    /**
     */
    template <typename T, size_t Size>
    class VecXRuntimeValue : public ser_detail::NativePrimitiveRuntimeValueBase<RuntimeReadonlyCollection>,
                             public RuntimeReadonlyDictionary
    {
        using BasePrimitiveValue = ser_detail::NativePrimitiveRuntimeValueBase<RuntimeReadonlyCollection>;
        using ThisType = nau::math::VecXRuntimeValue<T, Size>;

        NAU_CLASS_(ThisType, BasePrimitiveValue, RuntimeReadonlyDictionary)

    public:
        using Type = std::decay_t<T>;
        static constexpr bool IsReference = std::is_reference_v<T>;
        static constexpr bool IsMutable = !std::is_const_v<std::remove_reference_t<T>>;

        VecXRuntimeValue(T vec) :
            m_vec(vec)
        {
        }

        bool isMutable() const override
        {
            return IsMutable;
        }

        size_t getSize() const override
        {
            return getFieldNames().size();
        }

        RuntimeValue::Ptr getAt(size_t index) override
        {
            NAU_ASSERT(index < getSize());
            // be aware: getElem() actually can return FloatInVec, not floating point value.
            // without explicit cast m_vec.getElem() to float makeValueCopy's behavior is undefined !
            const float elem = m_vec.getElem(index);
            return makeValueCopy(elem);
        }

        Result<> setAt(size_t index, const RuntimeValue::Ptr& value) override
        {
            NAU_ASSERT(index < getSize());

            if constexpr (IsMutable)
            {
                auto castResult = runtimeValueCast<float>(value);
                NauCheckResult(castResult)

                value_changes_scope;
                m_vec.setElem(static_cast<int>(index), *castResult);
                return ResultSuccess;
            }
            else
            {
                NAU_ASSERT("Attempt to modify non mutable vec like value");
                return NauMakeError("Object not mutable");
            }
        }

        std::string_view getKey(size_t index) const override
        {
            NAU_ASSERT(index < getSize());

            return getFieldNames()[index];
        }

        RuntimeValue::Ptr getValue(std::string_view key) override
        {
            if (const auto index = getElementIndex(key))
            {
                const float elem = m_vec.getElem(*index);
                return makeValueCopy(elem);
            }

            return nullptr;
        }

        Result<> setValue(std::string_view key, const RuntimeValue::Ptr& value) override
        {
            if constexpr (IsMutable)
            {
                if (const auto index = getElementIndex(key))
                {
                    value_changes_scope;

                    const float elem = *runtimeValueCast<float>(value);
                    m_vec.setElem(static_cast<int>(*index), elem);

                    return ResultSuccess;
                }

                return NauMakeError("Unknown vec elem ({})", key);
            }
            else
            {
                NAU_ASSERT("Attempt to modify non mutable vec like value");
                return NauMakeError("Object not mutable");
            }
        }

        bool containsKey(std::string_view key) const override
        {
            const auto keys = getFieldNames();

            return std::any_of(keys.begin(), keys.end(), [key](const char* fieldName)
            {
                return strings::icaseEqual(key, fieldName);
            });
        }

    private:
        static consteval auto getFieldNames()
        {
            static_assert(2 <= Size && Size <= 4);

            if constexpr (Size == 2)
            {
                return std::array<const char*, Size>{"x", "y"};
            }
            else if constexpr (Size == 3)
            {
                return std::array<const char*, Size>{"x", "y", "z"};
            }
            else
            {
                return std::array<const char*, Size>{"x", "y", "z", "w"};
            }
        }

        static inline std::optional<size_t> getElementIndex(std::string_view key)
        {
            const auto keys = getFieldNames();
            auto index = std::find_if(keys.begin(), keys.end(), [key](const char* fieldName)
            {
                return strings::icaseEqual(key, fieldName);
            });

            if (index == keys.end())
            {
                NAU_FAILURE("Invalid field ({})", key);
                return std::nullopt;
            }

            return static_cast<size_t>(index - keys.begin());
        }

        T m_vec;
    };

    /**
     */
    template <typename T, size_t Size>
    class MatXRuntimeValue : public ser_detail::NativePrimitiveRuntimeValueBase<RuntimeReadonlyCollection>,
                             public RuntimeReadonlyDictionary
    {
        using BasePrimitiveValue = ser_detail::NativePrimitiveRuntimeValueBase<RuntimeReadonlyCollection>;
        using ThisType = nau::math::MatXRuntimeValue<T, Size>;

        NAU_CLASS_(ThisType, BasePrimitiveValue, RuntimeReadonlyDictionary)

    public:
        using Type = std::decay_t<T>;
        static constexpr bool IsReference = std::is_reference_v<T>;
        static constexpr bool IsMutable = !std::is_const_v<std::remove_reference_t<T>>;

        MatXRuntimeValue(T mat) :
            m_mat(mat)
        {
        }

        bool isMutable() const override
        {
            return IsMutable;
        }

        size_t getSize() const override
        {
            return getFieldNames().size();
        }

        RuntimeValue::Ptr getAt(size_t index) override
        {
            NAU_ASSERT(index < getSize());
            const auto elem = m_mat.getCol(index);
            return makeValueCopy(elem);
        }

        Result<> setAt(size_t index, const RuntimeValue::Ptr& value) override
        {
            NAU_ASSERT(index < getSize());

            if constexpr (IsMutable)
            {
                auto castResult = runtimeValueCast<ColType>(value);
                NauCheckResult(castResult)

                value_changes_scope;
                m_mat.setCol(static_cast<int>(index), *castResult);
                return ResultSuccess;
            }
            else
            {
                NAU_ASSERT("Attempt to modify non mutable mat like value");
                return NauMakeError("Object not mutable");
            }
        }

        std::string_view getKey(size_t index) const override
        {
            NAU_ASSERT(index < getSize());

            return getFieldNames()[index];
        }

        RuntimeValue::Ptr getValue(std::string_view key) override
        {
            if (const auto index = getElementIndex(key))
            {
                const auto elem = m_mat.getCol(*index);
                return makeValueCopy(elem);
            }

            return nullptr;
        }

        Result<> setValue(std::string_view key, const RuntimeValue::Ptr& value) override
        {
            if constexpr (IsMutable)
            {
                if (const auto index = getElementIndex(key))
                {
                    value_changes_scope;

                    const auto elem = *runtimeValueCast<ColType>(value);
                    m_mat.setCol(static_cast<int>(*index), elem);

                    return ResultSuccess;
                }

                return NauMakeError("Unknown mat elem ({})", key);
            }
            else
            {
                NAU_ASSERT("Attempt to modify non mutable mat like value");
                return NauMakeError("Object not mutable");
            }
        }

        bool containsKey(std::string_view key) const override
        {
            const auto keys = getFieldNames();

            return std::any_of(keys.begin(), keys.end(), [key](const char* fieldName)
            {
                return strings::icaseEqual(key, fieldName);
            });
        }

    private:
        static consteval auto getFieldNames()
        {
            static_assert(3 <= Size && Size <= 4);

            if constexpr (Size == 3)
            {
                return std::array<const char*, Size>{"Col0", "Col1", "Col2"};
            }
            else
            {
                return std::array<const char*, Size>{"Col0", "Col1", "Col2", "Col3"};
            }
        }

        static inline std::optional<size_t> getElementIndex(std::string_view key)
        {
            const auto keys = getFieldNames();
            auto index = std::find_if(keys.begin(), keys.end(), [key](const char* fieldName)
            {
                return strings::icaseEqual(key, fieldName);
            });

            if (index == keys.end())
            {
                NAU_FAILURE("Invalid field ({})", key);
                return std::nullopt;
            }

            return static_cast<size_t>(index - keys.begin());
        }

        T m_mat;

        using ColType = std::remove_const_t<decltype(m_mat.getCol0())>;
    };
}  // namespace nau::math

// this should be refactored in future
// namespace must be same where actual type (not alias) is defined
namespace Vectormath
{
    // Vector2

    inline nau::RuntimeValue::Ptr makeValueRef(Vector2& vec, nau::IMemAllocator::Ptr allocator = nullptr)
    {
        using namespace nau;
        using VecValue = math::VecXRuntimeValue<Vector2&, 2>;

        return rtti::createInstanceWithAllocator<VecValue>(std::move(allocator), vec);
    }

    inline nau::RuntimeValue::Ptr makeValueRef(const Vector2& v, nau::IMemAllocator::Ptr allocator = nullptr)
    {
        using namespace nau;
        using VecValue = math::VecXRuntimeValue<const Vector2&, 2>;

        return rtti::createInstanceWithAllocator<VecValue>(std::move(allocator), v);
    }

    inline nau::RuntimeValue::Ptr makeValueCopy(Vector2 v, nau::IMemAllocator::Ptr allocator = nullptr)
    {
        using namespace nau;
        using VecValue = math::VecXRuntimeValue<Vector2, 2>;

        return rtti::createInstanceWithAllocator<VecValue>(std::move(allocator), v);
    }

    // IVector2

    inline nau::RuntimeValue::Ptr makeValueRef(IVector2& vec, nau::IMemAllocator::Ptr allocator = nullptr)
    {
        using namespace nau;
        using VecValue = math::VecXRuntimeValue<IVector2&, 2>;

        return rtti::createInstanceWithAllocator<VecValue>(std::move(allocator), vec);
    }

    inline nau::RuntimeValue::Ptr makeValueRef(const IVector2& v, nau::IMemAllocator::Ptr allocator = nullptr)
    {
        using namespace nau;
        using VecValue = math::VecXRuntimeValue<const IVector2&, 2>;

        return rtti::createInstanceWithAllocator<VecValue>(std::move(allocator), v);
    }

    inline nau::RuntimeValue::Ptr makeValueCopy(IVector2 v, nau::IMemAllocator::Ptr allocator = nullptr)
    {
        using namespace nau;
        using VecValue = math::VecXRuntimeValue<IVector2, 2>;

        return rtti::createInstanceWithAllocator<VecValue>(std::move(allocator), v);
    }

    // IVector4

    inline nau::RuntimeValue::Ptr makeValueRef(IVector4& vec, nau::IMemAllocator::Ptr allocator = nullptr)
    {
        using namespace nau;
        using VecValue = math::VecXRuntimeValue<IVector4&, 4>;

        return rtti::createInstanceWithAllocator<VecValue>(std::move(allocator), vec);
    }

    inline nau::RuntimeValue::Ptr makeValueRef(const IVector4& v, nau::IMemAllocator::Ptr allocator = nullptr)
    {
        using namespace nau;
        using VecValue = math::VecXRuntimeValue<const IVector4&, 4>;

        return rtti::createInstanceWithAllocator<VecValue>(std::move(allocator), v);
    }

    inline nau::RuntimeValue::Ptr makeValueCopy(IVector4 v, nau::IMemAllocator::Ptr allocator = nullptr)
    {
        using namespace nau;
        using VecValue = math::VecXRuntimeValue<IVector4, 4>;

        return rtti::createInstanceWithAllocator<VecValue>(std::move(allocator), v);
    }
}  // namespace Vectormath

#if (VECTORMATH_CPU_HAS_SSE1_OR_BETTER && !VECTORMATH_FORCE_SCALAR_MODE)
namespace Vectormath::SSE
#elif (VECTORMATH_CPU_HAS_NEON && !VECTORMATH_FORCE_SCALAR_MODE)
namespace Vectormath::Neon
#else
namespace Vectormath::Scalar
#endif
{
    // Vector3

    inline nau::RuntimeValue::Ptr makeValueRef(Vector3& vec, nau::IMemAllocator::Ptr allocator = nullptr)
    {
        using namespace nau;
        using VecValue = math::VecXRuntimeValue<Vector3&, 3>;

        return rtti::createInstanceWithAllocator<VecValue>(std::move(allocator), vec);
    }

    inline nau::RuntimeValue::Ptr makeValueRef(const Vector3& v, nau::IMemAllocator::Ptr allocator = nullptr)
    {
        using namespace nau;
        using VecValue = math::VecXRuntimeValue<const Vector3&, 3>;

        return rtti::createInstanceWithAllocator<VecValue>(std::move(allocator), v);
    }

    inline nau::RuntimeValue::Ptr makeValueCopy(Vector3 v, nau::IMemAllocator::Ptr allocator = nullptr)
    {
        using namespace nau;
        using VecValue = math::VecXRuntimeValue<Vector3, 3>;

        return rtti::createInstanceWithAllocator<VecValue>(std::move(allocator), v);
    }

    // LikeVec4

    template <nau::math::LikeVec4 T>
    nau::RuntimeValue::Ptr makeValueRef(T& vec, nau::IMemAllocator::Ptr allocator = nullptr)
    {
        using namespace nau;
        using VecValue = math::VecXRuntimeValue<T&, 4>;

        return rtti::createInstanceWithAllocator<VecValue>(std::move(allocator), vec);
    }

    template <nau::math::LikeVec4 T>
    nau::RuntimeValue::Ptr makeValueRef(const T& v, nau::IMemAllocator::Ptr allocator = nullptr)
    {
        using namespace nau;
        using VecValue = math::VecXRuntimeValue<const T&, 4>;

        return rtti::createInstanceWithAllocator<VecValue>(std::move(allocator), v);
    }

    template <nau::math::LikeVec4 T>
    nau::RuntimeValue::Ptr makeValueCopy(T v, nau::IMemAllocator::Ptr allocator = nullptr)
    {
        using namespace nau;
        using VecValue = math::VecXRuntimeValue<T, 4>;

        return rtti::createInstanceWithAllocator<VecValue>(std::move(allocator), v);
    }

    // Matrix3

    inline nau::RuntimeValue::Ptr makeValueRef(Matrix3& mat, nau::IMemAllocator::Ptr allocator = nullptr)
    {
        using namespace nau;
        using MatValue = math::MatXRuntimeValue<Matrix3&, 3>;

        return rtti::createInstanceWithAllocator<MatValue>(std::move(allocator), mat);
    }

    inline nau::RuntimeValue::Ptr makeValueRef(const Matrix3& m, nau::IMemAllocator::Ptr allocator = nullptr)
    {
        using namespace nau;
        using MatValue = math::MatXRuntimeValue<const Matrix3&, 3>;

        return rtti::createInstanceWithAllocator<MatValue>(std::move(allocator), m);
    }

    inline nau::RuntimeValue::Ptr makeValueCopy(Matrix3 m, nau::IMemAllocator::Ptr allocator = nullptr)
    {
        using namespace nau;
        using MatValue = math::MatXRuntimeValue<Matrix3, 3>;

        return rtti::createInstanceWithAllocator<MatValue>(std::move(allocator), m);
    }

    // Matrix4

    inline nau::RuntimeValue::Ptr makeValueRef(Matrix4& mat, nau::IMemAllocator::Ptr allocator = nullptr)
    {
        using namespace nau;
        using MatValue = math::MatXRuntimeValue<Matrix4&, 4>;

        return rtti::createInstanceWithAllocator<MatValue>(std::move(allocator), mat);
    }

    inline nau::RuntimeValue::Ptr makeValueRef(const Matrix4& m, nau::IMemAllocator::Ptr allocator = nullptr)
    {
        using namespace nau;
        using MatValue = math::MatXRuntimeValue<const Matrix4&, 4>;

        return rtti::createInstanceWithAllocator<MatValue>(std::move(allocator), m);
    }

    inline nau::RuntimeValue::Ptr makeValueCopy(Matrix4 m, nau::IMemAllocator::Ptr allocator = nullptr)
    {
        using namespace nau;
        using MatValue = math::MatXRuntimeValue<Matrix4, 4>;

        return rtti::createInstanceWithAllocator<MatValue>(std::move(allocator), m);
    }
}  // namespace Vectormath::SSE
