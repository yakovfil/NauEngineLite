// Copyright 2024 N-GINN LLC. All rights reserved.
// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved

#pragma once

#include <nau/math/dag_e3dColor.h>
#include <nau/math/math.h>

#include "nau/serialization/runtime_value_builder.h"

namespace nau::math
{
    template <typename To, typename From>
    NAU_FORCE_INLINE To bitwise_cast(const From& from)
    {
        static_assert(sizeof(To) == sizeof(From), "bitwise_cast: types of different sizes provided");
        static_assert(eastl::is_pointer<To>::value == eastl::is_pointer<From>::value,
                      "bitwise_cast: can't cast between pointer and non-pointer types");
        static_assert(sizeof(From) <= 16, "bitwise_cast: can't cast types of size > 16");
        To result;
        memcpy(&result, &from, sizeof(From));
        return result;
    }

#define FP_BITS(fp) (bitwise_cast<unsigned int, float>(fp))

    inline unsigned float2uchar(float p)
    {
        const float _n = p + 1.0f;
        unsigned i = FP_BITS(_n);
        if (i >= 0x40000000)
            i = 0xFF;
        else if (i <= 0x3F800000)
            i = 0;
        else
            i = (i >> 15) & 0xFF;
        return i;
    }

    // Colors used in Driver3d materials and lighting //

    struct Color4
    {
        float r, g, b, a;
        NAU_FORCE_INLINE Color4() = default;
        NAU_FORCE_INLINE Color4(float rr, float gg, float bb, float aa = 1)
        {
            r = rr;
            g = gg;
            b = bb;
            a = aa;
        }
        NAU_FORCE_INLINE Color4(const float* p)
        {
            r = p[0];
            g = p[1];
            b = p[2];
            a = p[3];
        }
        NAU_FORCE_INLINE Color4(E3DCOLOR c)
        {
            r = float(c.r) / 255.f;
            g = float(c.g) / 255.f;
            b = float(c.b) / 255.f;
            a = float(c.a) / 255.f;
        }

        NAU_FORCE_INLINE void set(float k)
        {
            r = k;
            g = k;
            b = k;
            a = k;
        }
        NAU_FORCE_INLINE void set(float _r, float _g, float _b, float _a)
        {
            r = _r;
            g = _g;
            b = _b;
            a = _a;
        }
        NAU_FORCE_INLINE void zero()
        {
            set(0);
        }

        NAU_FORCE_INLINE float& operator[](int i)
        {
            return (&r)[i];
        }
        NAU_FORCE_INLINE const float& operator[](int i) const
        {
            return (&r)[i];
        }

        NAU_FORCE_INLINE Color4 operator+() const
        {
            return *this;
        }
        NAU_FORCE_INLINE Color4 operator-() const
        {
            return Color4(-r, -g, -b, -a);
        }
        NAU_FORCE_INLINE Color4 operator*(float k) const
        {
            return Color4(r * k, g * k, b * k, a * k);
        }
        NAU_FORCE_INLINE Color4 operator/(float k) const
        {
            return operator*(1.0f / k);
        }
        NAU_FORCE_INLINE Color4 operator*(const Color4& c) const
        {
            return Color4(r * c.r, g * c.g, b * c.b, a * c.a);
        }
        NAU_FORCE_INLINE Color4 operator/(const Color4& c) const
        {
            return Color4(r / c.r, g / c.g, b / c.b, a / c.a);
        }
        NAU_FORCE_INLINE Color4 operator+(const Color4& c) const
        {
            return Color4(r + c.r, g + c.g, b + c.b, a + c.a);
        }
        NAU_FORCE_INLINE Color4 operator-(const Color4& c) const
        {
            return Color4(r - c.r, g - c.g, b - c.b, a - c.a);
        }
        NAU_FORCE_INLINE Color4& operator+=(const Color4& c)
        {
            r += c.r;
            g += c.g;
            b += c.b;
            a += c.a;
            return *this;
        }
        NAU_FORCE_INLINE Color4& operator-=(const Color4& c)
        {
            r -= c.r;
            g -= c.g;
            b -= c.b;
            a -= c.a;
            return *this;
        }
        NAU_FORCE_INLINE Color4& operator*=(const Color4& c)
        {
            r *= c.r;
            g *= c.g;
            b *= c.b;
            a *= c.a;
            return *this;
        }
        NAU_FORCE_INLINE Color4& operator/=(const Color4& c)
        {
            r /= c.r;
            g /= c.g;
            b /= c.b;
            a /= c.a;
            return *this;
        }
        NAU_FORCE_INLINE Color4& operator*=(float k)
        {
            r *= k;
            g *= k;
            b *= k;
            a *= k;
            return *this;
        }
        NAU_FORCE_INLINE Color4& operator/=(float k)
        {
            return operator*=(1.0f / k);
        }

        NAU_FORCE_INLINE bool operator==(const Color4& c) const
        {
            return (r == c.r && g == c.g && b == c.b && a == c.a);
        }
        NAU_FORCE_INLINE bool operator!=(const Color4& c) const
        {
            return (r != c.r || g != c.g || b != c.b || a != c.a);
        }

        NAU_FORCE_INLINE void clamp0()
        {
            if (r < 0)
                r = 0;
            if (g < 0)
                g = 0;
            if (b < 0)
                b = 0;
            if (a < 0)
                a = 0;
        }
        NAU_FORCE_INLINE void clamp1()
        {
            if (r > 1)
                r = 1;
            if (g > 1)
                g = 1;
            if (b > 1)
                b = 1;
            if (a > 1)
                a = 1;
        }
        NAU_FORCE_INLINE void clamp01()
        {
            if (r < 0)
                r = 0;
            else if (r > 1)
                r = 1;
            if (g < 0)
                g = 0;
            else if (g > 1)
                g = 1;
            if (b < 0)
                b = 0;
            else if (b > 1)
                b = 1;
            if (a < 0)
                a = 0;
            else if (a > 1)
                a = 1;
        }

        template <class T>
        static Color4 xyzw(const T& a)
        {
            return Color4(a.x, a.y, a.z, a.w);
        }
        template <class T>
        static Color4 xyz0(const T& a)
        {
            return Color4(a.x, a.y, a.z, 0);
        }
        template <class T>
        static Color4 xyz1(const T& a)
        {
            return Color4(a.x, a.y, a.z, 1);
        }
        template <class T>
        static Color4 rgb0(const T& a)
        {
            return Color4(a.r, a.g, a.b, 0);
        }
        template <class T>
        static Color4 rgb1(const T& a)
        {
            return Color4(a.r, a.g, a.b, 1);
        }
        template <class T>
        static Color4 rgbV(const T& a, float v)
        {
            return Color4(a.r, a.g, a.b, v);
        }

        template <class T>
        void set_xyzw(const T& v)
        {
            r = v.x, g = v.y, b = v.z, a = v.w;
        }
        template <class T>
        void set_xyz0(const T& v)
        {
            r = v.x, g = v.y, b = v.z, a = 0;
        }
        template <class T>
        void set_xyz1(const T& v)
        {
            r = v.x, g = v.y, b = v.z, a = 1;
        }

        nau::math::Vector4 vector4() const
        {
            return {r, g, b, a};
        }
    };

    NAU_FORCE_INLINE float rgbsum(Color4 c)
    {
        return c.r + c.g + c.b;
    }
    NAU_FORCE_INLINE float average(Color4 c)
    {
        return rgbsum(c) / 3.0f;
    }
    // NTSC brightness Weights: r=.299 g=.587 b=.114
    NAU_FORCE_INLINE float brightness(Color4 c)
    {
        return c.r * .299f + c.g * .587f + c.b * .114;
    }

    NAU_FORCE_INLINE float lengthSq(Color4 c)
    {
        return (c.r * c.r + c.g * c.g + c.b * c.b);
    }
    NAU_FORCE_INLINE float length(Color4 c)
    {
        return sqrtf(lengthSq(c));
    }

    NAU_FORCE_INLINE Color4 max(const Color4& a, const Color4& b)
    {
        return Color4(Vectormath::max(a.r, b.r), Vectormath::max(a.g, b.g), Vectormath::max(a.b, b.b), Vectormath::max(a.a, b.a));
    }
    NAU_FORCE_INLINE Color4 min(const Color4& a, const Color4& b)
    {
        return Color4(Vectormath::min(a.r, b.r), Vectormath::min(a.g, b.g), Vectormath::min(a.b, b.b), Vectormath::min(a.a, b.a));
    }
    // template <>
    NAU_FORCE_INLINE Color4 clamp(Color4 t, const Color4 min_val, const Color4 max_val)
    {
        return min(max(t, min_val), max_val);
    }

    NAU_FORCE_INLINE Color4 operator*(float k, Color4 c)
    {
        return c * k;
    }
    NAU_FORCE_INLINE Color4 color4(E3DCOLOR c)
    {
        return Color4(c);
    }

    NAU_FORCE_INLINE E3DCOLOR e3dcolor(const Color4& c)
    {
        return E3DCOLOR_MAKE(float2uchar(c.r), float2uchar(c.g), float2uchar(c.b), float2uchar(c.a));
    }
    NAU_FORCE_INLINE E3DCOLOR e3dcolor_swapped(const Color4& c)
    {
        return E3DCOLOR_MAKE_SWAPPED(float2uchar(c.r), float2uchar(c.g), float2uchar(c.b), float2uchar(c.a));
    }

    struct Color3
    {
        float r, g, b;

        NAU_FORCE_INLINE Color3() = default;
        NAU_FORCE_INLINE Color3(float ar, float ag, float ab)
        {
            r = ar;
            g = ag;
            b = ab;
        }
        NAU_FORCE_INLINE Color3(const float* p)
        {
            r = p[0];
            g = p[1];
            b = p[2];
        }
        NAU_FORCE_INLINE Color3(E3DCOLOR c)
        {
            r = float(c.r) / 255.f;
            g = float(c.g) / 255.f;
            b = float(c.b) / 255.f;
        }

        NAU_FORCE_INLINE void set(float k)
        {
            r = k;
            g = k;
            b = k;
        }
        NAU_FORCE_INLINE void set(float _r, float _g, float _b)
        {
            r = _r;
            g = _g;
            b = _b;
        }
        NAU_FORCE_INLINE void zero()
        {
            set(0);
        }

        NAU_FORCE_INLINE float& operator[](int i)
        {
            return (&r)[i];
        }
        NAU_FORCE_INLINE const float& operator[](int i) const
        {
            return (&r)[i];
        }

        NAU_FORCE_INLINE Color3 operator+() const
        {
            return *this;
        }
        NAU_FORCE_INLINE Color3 operator-() const
        {
            return Color3(-r, -g, -b);
        }
        NAU_FORCE_INLINE Color3 operator*(float a) const
        {
            return Color3(r * a, g * a, b * a);
        }
        NAU_FORCE_INLINE Color3 operator/(float a) const
        {
            return operator*(1.0f / a);
        }
        NAU_FORCE_INLINE Color3 operator*(const Color3& c) const
        {
            return Color3(r * c.r, g * c.g, b * c.b);
        }
        NAU_FORCE_INLINE Color3 operator/(const Color3& c) const
        {
            return Color3(r / c.r, g / c.g, b / c.b);
        }
        NAU_FORCE_INLINE Color3 operator+(const Color3& c) const
        {
            return Color3(r + c.r, g + c.g, b + c.b);
        }
        NAU_FORCE_INLINE Color3 operator-(const Color3& c) const
        {
            return Color3(r - c.r, g - c.g, b - c.b);
        }
        NAU_FORCE_INLINE Color3& operator+=(const Color3& c)
        {
            r += c.r;
            g += c.g;
            b += c.b;
            return *this;
        }
        NAU_FORCE_INLINE Color3& operator-=(const Color3& c)
        {
            r -= c.r;
            g -= c.g;
            b -= c.b;
            return *this;
        }
        NAU_FORCE_INLINE Color3& operator*=(const Color3& c)
        {
            r *= c.r;
            g *= c.g;
            b *= c.b;
            return *this;
        }
        NAU_FORCE_INLINE Color3& operator/=(const Color3& c)
        {
            r /= c.r;
            g /= c.g;
            b /= c.b;
            return *this;
        }
        NAU_FORCE_INLINE Color3& operator*=(float k)
        {
            r *= k;
            g *= k;
            b *= k;
            return *this;
        }
        NAU_FORCE_INLINE Color3& operator/=(float k)
        {
            return operator*=(1.0f / k);
        }

        NAU_FORCE_INLINE bool operator==(const Color3& c) const
        {
            return (r == c.r && g == c.g && b == c.b);
        }
        NAU_FORCE_INLINE bool operator!=(const Color3& c) const
        {
            return (r != c.r || g != c.g || b != c.b);
        }

        NAU_FORCE_INLINE void clamp0()
        {
            if (r < 0)
                r = 0;
            if (g < 0)
                g = 0;
            if (b < 0)
                b = 0;
        }
        NAU_FORCE_INLINE void clamp1()
        {
            if (r > 1)
                r = 1;
            if (g > 1)
                g = 1;
            if (b > 1)
                b = 1;
        }
        NAU_FORCE_INLINE void clamp01()
        {
            if (r < 0)
                r = 0;
            else if (r > 1)
                r = 1;
            if (g < 0)
                g = 0;
            else if (g > 1)
                g = 1;
            if (b < 0)
                b = 0;
            else if (b > 1)
                b = 1;
        }

        template <class T>
        static Color3 xyz(const T& a)
        {
            return Color3(a.x, a.y, a.z);
        }
        template <class T>
        static Color3 rgb(const T& a)
        {
            return Color3(a.r, a.g, a.b);
        }

        template <class T>
        void set_xyz(const T& v)
        {
            r = v.x, g = v.y, b = v.z;
        }
        template <class T>
        void set_rgb(const T& v)
        {
            r = v.r, g = v.g, b = v.b;
        }
    };

    NAU_FORCE_INLINE float rgbsum(const Color3& c)
    {
        return c.r + c.g + c.b;
    }
    NAU_FORCE_INLINE float average(const Color3& c)
    {
        return rgbsum(c) / 3.0f;
    }
    // NTSC brightness Weights: r=.299 g=.587 b=.114
    NAU_FORCE_INLINE float brightness(const Color3& c)
    {
        return c.r * .299f + c.g * .587f + c.b * .114;
    }

    NAU_FORCE_INLINE float lengthSq(const Color3& c)
    {
        return (c.r * c.r + c.g * c.g + c.b * c.b);
    }
    NAU_FORCE_INLINE float length(const Color3& c)
    {
        return sqrtf(lengthSq(c));
    }

    NAU_FORCE_INLINE Color3 max(const Color3& a, const Color3& b)
    {
        return Color3(Vectormath::max(a.r, b.r), Vectormath::max(a.g, b.g), Vectormath::max(a.b, b.b));
    }
    NAU_FORCE_INLINE Color3 min(const Color3& a, const Color3& b)
    {
        return Color3(Vectormath::min(a.r, b.r), Vectormath::min(a.g, b.g), Vectormath::min(a.b, b.b));
    }
    // template <>
    NAU_FORCE_INLINE Color3 clamp(Color3 t, const Color3 min_val, const Color3 max_val)
    {
        return min(max(t, min_val), max_val);
    }

    NAU_FORCE_INLINE Color3 operator*(float a, const Color3& c)
    {
        return c * a;
    }
    NAU_FORCE_INLINE Color3 color3(E3DCOLOR c)
    {
        return Color3(c);
    }

    NAU_FORCE_INLINE Color4 color4(const Color3& c, float a)
    {
        return Color4(c.r, c.g, c.b, a);
    }
    NAU_FORCE_INLINE Color3 color3(const Color4& c)
    {
        return Color3(c.r, c.g, c.b);
    }

    NAU_FORCE_INLINE E3DCOLOR e3dcolor(const Color3& c, unsigned a = 255)
    {
        return E3DCOLOR_MAKE(float2uchar(c.r), float2uchar(c.g), float2uchar(c.b), a);
    }
    NAU_FORCE_INLINE E3DCOLOR e3dcolor_swapped(const Color3& c, unsigned a = 255)
    {
        return E3DCOLOR_MAKE_SWAPPED(float2uchar(c.r), float2uchar(c.g), float2uchar(c.b), a);
    }

    class Color4RuntimeValue : public ser_detail::NativePrimitiveRuntimeValueBase<RuntimeReadonlyCollection>,
                               public RuntimeReadonlyDictionary
    {
        using BasePrimitiveValue = ser_detail::NativePrimitiveRuntimeValueBase<RuntimeReadonlyCollection>;

        NAU_CLASS_(Color4RuntimeValue, BasePrimitiveValue, RuntimeReadonlyDictionary)

    public:
        using Type = Color4;
        static constexpr bool IsReference = std::is_reference_v<Color4>;

        Color4RuntimeValue(Color4 color) :
            m_color(color)
        {
        }

        bool isMutable() const override
        {
            return true;
        }

    private:
        static consteval std::array<const char*, 4> getFieldNames()
        {
            return std::array<const char*, 4>{"r", "g", "b", "a"};
        }

    public:
        size_t getSize() const override
        {
            return getFieldNames().size();
        }

        RuntimeValue::Ptr getAt(size_t index) override
        {
            NAU_ASSERT(index < getSize());
            // be aware: getElem() actually can return FloatInVec, not floating point value.
            // without explicit cast m_vec.getElem() to float makeValueCopy's behavior is undefined !
            const float elem = m_color[index];
            return makeValueCopy(elem);
        }

        Result<> setAt(size_t index, const RuntimeValue::Ptr& value) override
        {
            auto castResult = runtimeValueCast<float>(value);
            NauCheckResult(castResult)

            value_changes_scope;
            m_color[static_cast<int>(index)] = *castResult;
            return ResultSuccess;
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
                const float elem = m_color[*index];
                return makeValueCopy(elem);
            }

            return nullptr;
        }

        Result<> setValue(std::string_view key, const RuntimeValue::Ptr& value) override
        {
            if (const auto index = getElementIndex(key))
            {
                value_changes_scope;

                const float elem = *runtimeValueCast<float>(value);
                m_color[static_cast<int>(*index)] = elem;

                return ResultSuccess;
            }

            return NauMakeError("Unknown vec elem ({})", key);
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

        Color4 m_color;
    };


    class Color3RuntimeValue : public ser_detail::NativePrimitiveRuntimeValueBase<RuntimeReadonlyCollection>,
                               public RuntimeReadonlyDictionary
    {
        using BasePrimitiveValue = ser_detail::NativePrimitiveRuntimeValueBase<RuntimeReadonlyCollection>;

        NAU_CLASS_(Color3RuntimeValue, BasePrimitiveValue, RuntimeReadonlyDictionary)

    public:
        using Type = Color3;
        static constexpr bool IsReference = std::is_reference_v<Color4>;

        Color3RuntimeValue(Color3 color) :
            m_color(color)
        {
        }

        bool isMutable() const override
        {
            return true;
        }

    private:
        static consteval auto getFieldNames()
        {
            return std::array<const char*, 3>{"r", "g", "b"};
        }

    public:
        size_t getSize() const override
        {
            return getFieldNames().size();
        }

        RuntimeValue::Ptr getAt(size_t index) override
        {
            NAU_ASSERT(index < getSize());
            // be aware: getElem() actually can return FloatInVec, not floating point value.
            // without explicit cast m_vec.getElem() to float makeValueCopy's behavior is undefined !
            const float elem = m_color[index];
            return makeValueCopy(elem);
        }

        Result<> setAt(size_t index, const RuntimeValue::Ptr& value) override
        {
            auto castResult = runtimeValueCast<float>(value);
            NauCheckResult(castResult)

            value_changes_scope;
            m_color[static_cast<int>(index)] = *castResult;
            return ResultSuccess;
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
                const float elem = m_color[*index];
                return makeValueCopy(elem);
            }

            return nullptr;
        }

        Result<> setValue(std::string_view key, const RuntimeValue::Ptr& value) override
        {
            if (const auto index = getElementIndex(key))
            {
                value_changes_scope;

                const float elem = *runtimeValueCast<float>(value);
                m_color[static_cast<int>(*index)] = elem;

                return ResultSuccess;
            }

            return NauMakeError("Unknown vec elem ({})", key);
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

        Color3 m_color;
    };


    inline ::nau::RuntimeValue::Ptr
    makeValueRef(const Color4& color, ::nau::IMemAllocator::Ptr allocator = nullptr)
    {
        using namespace nau;

        return rtti::createInstanceWithAllocator<Color4RuntimeValue>(std::move(allocator), color);
    }

    inline ::nau::RuntimeValue::Ptr makeValueCopy(Color4 v, ::nau::IMemAllocator::Ptr allocator = nullptr)
    {
        using namespace nau;

        return rtti::createInstanceWithAllocator<Color4RuntimeValue>(std::move(allocator), v);
    }

    inline ::nau::RuntimeValue::Ptr makeValueRef(const Color3& color, ::nau::IMemAllocator::Ptr allocator = nullptr)
    {
        using namespace nau;

        return rtti::createInstanceWithAllocator<Color3RuntimeValue>(std::move(allocator), color);
    }

    inline ::nau::RuntimeValue::Ptr makeValueCopy(Color3 v, ::nau::IMemAllocator::Ptr allocator = nullptr)
    {
        using namespace nau;

        return rtti::createInstanceWithAllocator<Color3RuntimeValue>(std::move(allocator), v);
    }

}  // namespace nau::math
