// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.
// nau/service/hash_string.h


#pragma once
#include "nau/string/string.h"

namespace nau
{
    struct hash_string
    {
    private:
        string m_data;
        size_t m_stringHash;

    public:
        static constexpr size_t constHash(const char8_t* input, size_t len)
        {
            size_t hash = sizeof(size_t) == 8 ? 0xcbf29ce484222325 : 0x811c9dc5;
            const size_t prime = sizeof(size_t) == 8 ? 0x00000100000001b3 : 0x01000193;

            while(*input && (len--))
            {
                hash ^= static_cast<size_t>(*input);
                hash *= prime;
                ++input;
            }

            return hash;
        }

        hash_string(size_t hash) :
            m_data(""),
            m_stringHash(hash){

            };

        hash_string(string other) :
            m_data(std::move(other)),
            m_stringHash(constHash(m_data.c_str(), m_data.size())){

            };

        hash_string(const hash_string& other) :
            m_data(other.m_data),
            m_stringHash(other.m_stringHash){};

        hash_string(hash_string&& other) noexcept :
            m_data(other.m_data),
            m_stringHash(other.m_stringHash){

            };

        inline void operator=(const hash_string& rhs)
        {
            m_data = rhs.m_data;
            m_stringHash = rhs.m_stringHash;
        };

        inline void operator=(hash_string&& rhs)
        {
            m_data = std::move(rhs.m_data);
            m_stringHash = rhs.m_stringHash;
        };

        string toString() const
        {
            return m_data;
        }

        size_t toHash() const
        {
            return m_stringHash;
        }

        inline bool operator==(const hash_string& rhs) const
        {
            return m_stringHash == rhs.m_stringHash;
        };
        inline bool operator!=(const string& rhs) const
        {
            return !(*this == rhs);
        };
        operator size_t() const
        {
            return m_stringHash;
        }
    };

    namespace string_literals
    {
        [[nodiscard]] constexpr size_t operator"" _sh(const char8_t* _Str, size_t _Len)
        {
            return hash_string::constHash(_Str, _Len);
        }
    }  // namespace string_literals
}  // namespace nau

template <>
struct eastl::hash<nau::hash_string>
{
    size_t operator()(const nau::hash_string& s) const
    {
        return s.toHash();
    }
};

template <>
struct std::hash<nau::hash_string>
{
    size_t operator()(const nau::hash_string& s) const
    {
        return s.toHash();
    }
};