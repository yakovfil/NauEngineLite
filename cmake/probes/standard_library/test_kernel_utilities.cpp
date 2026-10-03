// Copyright 2026 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.
#include <array>
#include <cstdlib>
#include <cstring>
#include <vector>

#include "nau/generic/dag_patchTab.h"
#include "nau/kernel/kernel_config.h"
#include "nau/utils/span.h"
#include "nau/utils/dag_stlqsort.h"
#include "nau/utils/enum_utils.h"

void check(bool value)
{
    if (!value)
        std::abort();
}
enum class Signed : signed char
{
    Negative = -5
};
enum class Unsigned : unsigned long long
{
    High = 1ull << 63
};
static_assert(nau::to_underlying(Signed::Negative) == -5);
static_assert(nau::to_underlying(Unsigned::High) == (1ull << 63));
static_assert(std::is_same_v<decltype(nau::to_underlying(Signed::Negative)), signed char>);
static_assert(noexcept(nau::to_underlying(Signed::Negative)));
static_assert(sizeof(PatchableTab<int>) == 16);
static_assert(std::is_standard_layout_v<PatchableTab<int>>);
NAU_FORCE_INLINE int increment(int n)
{
    return n + 1;
}
int main()
{
    check(increment(1) == 2);
    int items[] = {3, 5, 7};
    auto view = make_span(items);
    static_assert(std::is_same_v<decltype(view), std::span<int>>);
    check(view.subspan(1).front() == 5 && view.size_bytes() == sizeof(items));
    view[1] = 9;
    check(items[1] == 9 && make_span_const(items)[1] == 9);
    std::vector<int> owned{11, 13};
    check(make_span(owned).data() == owned.data());
    check(make_span_const(owned).back() == 13);
    PatchableTab<int> empty;
    check(empty.empty() && empty.begin() == empty.end());
    alignas(16) std::array<int, 8> blob{0, 0, 10, 20, 30, 0, 0, 0};
    PatchableTab<int> tab;
    // The serialized prefix is an offset/count pair of 32-bit words.
    std::array<std::uint32_t, 4> encoded{8, 3, 0, 0};
    std::memcpy(&tab, encoded.data(), sizeof(tab));
    tab.patch(blob.data());
    check(tab.size() == 3 && tab.size_bytes() == 12 && tab.front() == 10 && tab.back() == 30);
    check(tab.data() == blob.data() + 2 && tab.cend() - tab.cbegin() == 3);
    auto clone = blob;
    tab.rebase(clone.data(), blob.data());
    check(tab.data() == clone.data() + 2 && tab[1] == 20);
    tab.init(blob.data(), 2);
    check(tab.size() == 2 && tab.data() == blob.data());
    for (int count : {0, 1, 2, 7, 24, 129, 4096})
    {
        std::vector<int> values;
        for (int i = 0; i < count; ++i)
            values.push_back((i * 53) % 31);
        auto expected = values;
        std::sort(expected.begin(), expected.end());
        stlsort::sort(values.begin(), values.end(), std::less<int>{});
        check(values == expected);
        if (count)
        {
            std::reverse(values.begin(), values.end());
            stlsort::nth_element(values.begin(), values.begin() + count / 2, values.end(), std::less<int>{});
            check(values[count / 2] == expected[count / 2]);
        }
    }
}
