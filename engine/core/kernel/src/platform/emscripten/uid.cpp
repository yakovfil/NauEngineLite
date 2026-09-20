// Copyright 2026 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "nau/utils/uid.h"

#include <random>

namespace nau
{
    Uid::Uid() noexcept :
        m_data{}
    {
    }

    Uid::Uid(std::array<uint8_t, 16> data) noexcept :
        m_data(data)
    {
    }

    Uid Uid::generate()
    {
        static std::mutex mutex;
        static std::random_device random;
        std::lock_guard lock(mutex);
        std::array<uint8_t, 16> data;
        for (auto& value : data)
        {
            value = static_cast<uint8_t>(random());
        }
        data[6] = (data[6] & 0x0f) | 0x40;
        data[8] = (data[8] & 0x3f) | 0x80;
        return Uid{data};
    }

    Result<Uid> Uid::parseString(std::string_view text)
    {
        if (text.size() != 36)
        {
            return NauMakeError("Invalid UID string length");
        }
        const auto hex = [](char c) -> int
        {
            if (c >= '0' && c <= '9')
                return c - '0';
            if (c >= 'a' && c <= 'f')
                return c - 'a' + 10;
            if (c >= 'A' && c <= 'F')
                return c - 'A' + 10;
            return -1;
        };
        std::array<uint8_t, 16> data;
        size_t byte = 0;
        for (size_t i = 0; i < text.size();)
        {
            if (i == 8 || i == 13 || i == 18 || i == 23)
            {
                if (text[i++] != '-')
                    return NauMakeError("Invalid UID separator");
                continue;
            }
            const int high = hex(text[i++]);
            const int low = hex(text[i++]);
            if (high < 0 || low < 0)
                return NauMakeError("Invalid UID hexadecimal digit");
            data[byte++] = static_cast<uint8_t>((high << 4) | low);
        }
        return Uid{data};
    }

    Result<> parse(std::string_view text, Uid& uid)
    {
        auto result = Uid::parseString(text);
        if (!result)
        {
            uid = Uid{};
            return result.getError();
        }
        uid = *result;
        return ResultSuccess;
    }

    std::string toString(const Uid& uid)
    {
        constexpr char Hex[] = "0123456789abcdef";
        std::string result;
        result.reserve(36);
        for (size_t i = 0; i < uid.m_data.size(); ++i)
        {
            if (i == 4 || i == 6 || i == 8 || i == 10)
                result += '-';
            result += Hex[uid.m_data[i] >> 4];
            result += Hex[uid.m_data[i] & 15];
        }
        return result;
    }

    size_t Uid::getHashCode() const
    {
        size_t value = 2166136261u;
        for (auto byte : m_data)
            value = (value ^ byte) * 16777619u;
        return value;
    }

    Uid::operator bool() const noexcept
    {
        return std::any_of(m_data.begin(), m_data.end(), [](uint8_t value)
        {
            return value != 0;
        });
    }

    bool operator==(const Uid& a, const Uid& b) noexcept
    {
        return a.m_data == b.m_data;
    }
    bool operator!=(const Uid& a, const Uid& b) noexcept
    {
        return !(a == b);
    }
    bool operator<(const Uid& a, const Uid& b) noexcept
    {
        return a.m_data < b.m_data;
    }
}  // namespace nau
