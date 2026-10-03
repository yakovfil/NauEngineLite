// Copyright 2026 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include <string>
#if defined(_MSC_VER) && defined(_DEBUG)
    #include <crtdbg.h>
#endif

#include "nau/utils/dynamic_bit_vector.h"

void check(bool condition)
{
    if (!condition)
        std::abort();
}

int main()
{
#if defined(_MSC_VER)
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
    #if defined(_DEBUG)
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);
    #endif
#endif
    using Bits = nau::DynamicBitVector<>;
    static_assert(std::random_access_iterator<Bits::iterator>);
    static_assert(std::random_access_iterator<Bits::const_iterator>);
    Bits bits;
    bits.set(129, true);
    check(bits.size() == 130 && bits[129]);
    for (std::size_t index : {0, 31, 32, 63, 64, 127, 128})
        bits.set(index, true);
    auto original = bits;
    bits.resize(65);
    bits.resize(130);
    check(bits[0] && bits[63] && bits[64] && !bits[127] && !bits[129]);
    bits.resize(65);
    bits.resize(66, true);
    check(bits[65]);
    bits.resize(140, true);
    check(std::all_of(bits.begin() + 65, bits.end(), [](bool bit)
    {
        return bit;
    }));
    constexpr auto wordBits = std::numeric_limits<nau::BitVectorWord>::digits;
    check(bits.data()[139 / wordBits] == ((nau::BitVectorWord{1} << (140 % wordBits)) - 1));
    bits.fill(true);
    check(std::count(bits.begin(), bits.end(), true) == 140);
    std::fill(bits.begin(), bits.end(), false);
    check(std::count(bits.begin(), bits.end(), true) == 0);
    bits[0] = true;
    bits[1] = bits[0];
    check(bits[1]);
    const Bits& constant = bits;
    check(constant.test(1000, true) && !constant.test(1000));
    check(constant.at(1));
    Bits moved = std::move(original);
    check(original.empty() && moved.size() == 130 && moved[129]);
    bits.swap(moved);
    check(bits.size() == 130 && moved.size() == 140);
    bits.clear();
    bits.resize(130);
    check(std::count(bits.begin(), bits.end(), true) == 0);
    bits.push_back(true);
    check(bits.back());
    bits.pop_back();
    check(bits.size() == 130);
    // Network consumers serialize components conditionally in index order,
    // never the storage words. Keep a concrete logical fixture.
    bits.set(1, true);
    bits.set(64, true);
    bits.set(129, true);
    std::string serialized;
    for (std::size_t i = 0; i < bits.size(); ++i)
        if (bits.test(i))
            serialized += std::to_string(i) + ",";
    check(serialized == "1,64,129,");
    std::vector<bool> expected;
    bits.clear();
    unsigned seed = 17;
    for (int step = 0; step < 3000; ++step)
    {
        seed = seed * 1664525u + 1013904223u;
        const auto count = (seed >> 8) % 300;
        const bool value = (seed & 128) != 0;
        switch (seed % 3)
        {
            case 0:
                bits.resize(count, value);
                expected.resize(count, value);
                break;
            case 1:
                bits.set(count, value);
                if (count >= expected.size())
                    expected.resize(count + 1);
                expected[count] = value;
                break;
            default:
                bits.fill(value);
                std::fill(expected.begin(), expected.end(), value);
                break;
        }
        check(bits.size() == expected.size());
        check(std::equal(bits.begin(), bits.end(), expected.begin()));
    }
    std::puts("dynamic bits: boundaries, growth, tail, bulk operations and logical fixture passed");
}
