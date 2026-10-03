// Copyright 2026 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include <memory>

#include "nau/utils/fixed_function.h"
#include <cstring>

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
struct Target
{
    static inline int alive = 0;
    int value = 7;
    Target()
    {
        ++alive;
    }
    Target(const Target& other) :
        value(other.value)
    {
        ++alive;
    }
    Target(Target&& other) :
        value(other.value)
    {
        ++alive;
    }
    ~Target()
    {
        --alive;
    }
    int operator()()
    {
        return value++;
    }
};
struct Large
{
    char bytes[128];
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

int main(int argc, char** argv)
{
    using Function = nau::FixedFunction<16, int()>;
    if (argc == 2 && std::strcmp(argv[1], "--empty") == 0)
    {
        Function empty;
        empty();
        return 1;
    }
    static_assert(!std::is_constructible_v<nau::FixedFunction<16, void()>, Large>);
    static_assert(!std::is_constructible_v<nau::FixedFunction<64, void()>, Aligned>);
    const auto allocationsBefore = backingAllocations;
    {
        const Function first{Target{}};
        check(first() == 7);
        Function copy = first;
        check(copy() == 8 && first() == 8);
        Function moved = std::move(copy);
        check(!copy && moved() == 9);
        copy = first;
        copy = copy;
        copy = std::move(copy);
        check(copy() == 9);
        copy.swap(moved);
        check(copy() == 10);
        moved = nullptr;
        check(!moved);
    }
    check(Target::alive == 0);
    check(backingAllocations == allocationsBefore);
    nau::FixedFunction<16, int(std::unique_ptr<int>)> consume{[](auto value)
    {
        return *value;
    }};
    check(consume(std::make_unique<int>(42)) == 42);
    nau::FixedFunction<16, void()> discard{[]
    {
        return 1;
    }};
    discard();
    int (*nullFunction)() = nullptr;
    Function empty{nullFunction};
    check(!empty);
#if defined(__cpp_exceptions) || defined(_CPPUNWIND)
    bool caught = false;
    try
    {
        empty();
    }
    catch (const std::bad_function_call&)
    {
        caught = true;
    }
    check(caught);
#endif
    std::puts("fixed function: copy/move/lifetime/bounds/forwarding passed");
}
