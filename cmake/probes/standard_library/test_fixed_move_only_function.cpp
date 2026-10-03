// Copyright 2026 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include <cstring>
#include <memory>

#include "nau/generic/dag_fixedMoveOnlyFunction.h"
#if defined(_MSC_VER) && defined(_DEBUG)
    #include <crtdbg.h>
#endif

static unsigned backingAllocations = 0;
void* operator new(std::size_t bytes)
{
    ++backingAllocations;
    if (void* pointer = std::malloc(bytes ? bytes : 1))
        return pointer;
    std::abort();
}
void operator delete(void* pointer) noexcept
{
    std::free(pointer);
}
void operator delete(void* pointer, std::size_t) noexcept
{
    std::free(pointer);
}

void check(bool condition)
{
    if (!condition)
        std::abort();
}
struct Large
{
    char data[128];
    void operator()() const
    {
    }
};
struct alignas(32) Aligned
{
    void operator()() const
    {
    }
};
struct Mutable
{
    void operator()()
    {
    }
};
struct Lifetime
{
    static inline int alive = 0;
    Lifetime()
    {
        ++alive;
    }
    Lifetime(Lifetime&&)
    {
        ++alive;
    }
    Lifetime(const Lifetime&) = delete;
    ~Lifetime()
    {
        --alive;
    }
    int operator()() const
    {
        return 42;
    }
};

int main(int argc, char** argv)
{
#if defined(_MSC_VER)
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
    #if defined(_DEBUG)
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);
    #endif
#endif
    if (argc == 2 && std::strcmp(argv[1], "--empty") == 0)
    {
        nau::FixedMoveOnlyFunction<16, void()> empty;
        empty();
        return 1;
    }
    if (argc == 2 && std::strcmp(argv[1], "--empty-const") == 0)
    {
        const nau::FixedMoveOnlyFunction<16, void() const> empty;
        empty();
        return 1;
    }
    using Function = nau::FixedMoveOnlyFunction<16, void()>;
    static_assert(!std::is_constructible_v<Function, Large>);
    static_assert(!std::is_constructible_v<Function, Aligned>);
    static_assert(!std::is_copy_constructible_v<Function>);
    static_assert(!std::is_constructible_v<nau::FixedMoveOnlyFunction<16, void() const>, Mutable>);
    const auto allocationsBefore = backingAllocations;
    {
        nau::FixedMoveOnlyFunction<16, int() const> original{Lifetime{}};
        check(Lifetime::alive == 1 && original() == 42);
        auto moved = std::move(original);
        check(!original && moved() == 42 && Lifetime::alive == 1);
        original = std::move(moved);
        check(!moved && original() == 42 && Lifetime::alive == 1);
        original = nullptr;
        check(!original && Lifetime::alive == 0);
        auto empty = std::move(original);
        check(!empty);
    }
    check(backingAllocations == allocationsBefore);
    nau::FixedMoveOnlyFunction<16, int(std::unique_ptr<int>)> consume{
        [offset = std::make_unique<int>(2)](std::unique_ptr<int> value)
    {
        return *value + *offset;
    }};
    check(consume(std::make_unique<int>(40)) == 42);
    Function ignoreResult{[]
    {
        return 1;
    }};
    ignoreResult();
    std::puts("fixed move-only function: lifetime, forwarding, qualifiers and bounds passed");
}
