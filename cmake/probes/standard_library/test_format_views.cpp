// Copyright 2026 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include <cstdio>
#include <cstdlib>

#include "nau/string/format.h"

void check(const std::string& actual, std::string_view expected)
{
    static unsigned index = 0;
    ++index;
    if (std::string_view(actual.data(), actual.size()) != expected)
    {
        std::fprintf(stderr, "format case %u: actual size %zu, expected %zu\n", index, actual.size(), expected.size());
        std::exit(1);
    }
}

int main()
{
    const char exact[] = {'a', 'b'};
    check(nau::utils::format("{}", std::string_view(exact, 2)), "ab");
    check(nau::utils::format("{}", std::string_view(exact, 2)), "ab");
    const char longer[] = "abx";
    check(nau::utils::format("{}", std::string_view(longer, 2)), "ab");
    const char embedded[] = {'a', 0, 'b'};
    check(nau::utils::format("{}", std::string_view(embedded, 3)), {embedded, 3});
    check(nau::utils::format("{}", std::string(embedded, 3)), {embedded, 3});
    check(nau::utils::format(std::string_view(embedded, 3)), {embedded, 3});
    const char8_t utf8[] = {u8'a', 0, u8'b'};
    check(nau::utils::format(u8"{}", std::u8string_view(utf8, 3)), {embedded, 3});
    check(nau::utils::format(u8"{}", std::u8string_view(utf8, 3)), {embedded, 3});
    const char pattern[] = {'{', '}', 0, '{', '}'};
    check(nau::utils::format(std::string_view(pattern, 5), 'a', 'b'), {embedded, 3});
    check(nau::utils::format("[{}]", std::string_view{}), "[]");
    check(nau::utils::format("[{}]", std::u8string_view{}), "[]");
    check(nau::utils::format("{:>4}", std::string_view(longer, 2)), "  ab");
    check(nau::utils::format("{:04d}/{}", 12, u8'x'), "0012/x");
    check(nau::utils::format(u8"{}", std::u8string(u8"\u0416\U0001f680")), "\xd0\x96\xf0\x9f\x9a\x80");
}
