// Copyright 2026 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include <cstring>
#include <unordered_map>

#include "nau/memory/standard_allocator.h"
#include "nau/utils/inline_string.h"

void check(bool condition)
{
    if (!condition)
        std::abort();
}
struct Resource
{
    int allocations = 0;
    std::unordered_map<void*, size_t> live;
    void* allocate(size_t bytes, size_t alignment)
    {
        void* pointer = ::operator new(bytes, std::align_val_t(alignment));
        live.emplace(pointer, alignment);
        ++allocations;
        return pointer;
    }
    void deallocate(void* pointer, size_t, size_t alignment)
    {
        check(live.at(pointer) == alignment);
        live.erase(pointer);
        ::operator delete(pointer, std::align_val_t(alignment));
    }
};

int main(int argc, char** argv)
{
#if defined(_MSC_VER)
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
#endif
    if (argc > 1 && std::strcmp(argv[1], "--overflow-disabled") == 0)
    {
        nau::InlineString<char, 4, false> bounded("abc");
        bounded += 'd';
        return 1;
    }
    Resource first, second;
    using Allocator = nau::ResourceAllocator<char, Resource>;
    using String = nau::InlineString<char, 4, true, Allocator>;
    {
        String text{Allocator(first)};
        check(text.empty() && text.c_str()[0] == 0 && first.allocations == 0);
        text = "abc";
        check(text.size() == 3 && text.c_str()[3] == 0 && first.allocations == 0);
        auto copy = text;
        check(copy == text && first.allocations == 0);
        text += 'd';
        check(first.allocations > 0 && std::string_view(text) == "abcd" && text.c_str()[4] == 0);
        text.append(text.data() + 1, 3);
        check(std::string_view(text) == "abcdbcd");
        auto moved = std::move(text);
        check(text.empty() && text.c_str()[0] == 0 && moved.size() == 7);
        String destination{Allocator(second)};
        destination = std::move(moved);
        check(&destination.get_allocator().resource() == &second);
        check(moved.empty() && moved.c_str()[0] == 0 && destination.size() == 7);
        destination.resize(2);
        check(std::string_view(destination) == "ab");
        destination.resize(5, 'x');
        check(std::string_view(destination) == "abxxx" && destination.c_str()[5] == 0);
        destination = "name_local";
        destination.replace(destination.begin() + 5, destination.end(), "remote", 6);
        check(std::string_view(destination) == "name_remote");
        destination = std::string_view("a\0b", 3);
        check(destination.size() == 3 && destination[1] == 0 && destination[2] == 'b' && destination[3] == 0);
        destination.append(destination.c_str(), destination.size() + 1);
        check(destination.size() == 7 && destination[6] == 0 && destination[7] == 0);
    }
    check(first.live.empty() && second.live.empty());
    nau::InlineString<char, 8, false> bounded("abcdefg");
    check(bounded.size() == 7 && bounded.c_str()[7] == 0);
    auto inlineMoved = std::move(bounded);
    check(bounded.empty() && bounded.c_str()[0] == 0 && inlineMoved.size() == 7);
    nau::InlineString<char, 8> concatenated = "abc";
    check(std::string_view(concatenated + "def") == "abcdef");
    std::puts("inline string: capacity, terminators, aliasing, moves and domains passed");
}
