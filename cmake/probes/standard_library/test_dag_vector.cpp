// Copyright 2026 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include "daECS/utility/dag_vector.h"
#undef NAU_ASSERT
#define NAU_ASSERT(condition, ...) \
    do                             \
    {                              \
        if (!(condition))          \
            std::abort();          \
    } while (false)
#undef NAU_ASSERT_RETURN
#define NAU_ASSERT_RETURN(condition, result, ...) \
    do                                            \
    {                                             \
        if (!(condition))                         \
        {                                         \
            std::abort();                         \
            return result;                        \
        }                                         \
    } while (false)
#include "nau/utils/dag_relocatableFixedVector.h"

// This standalone probe exercises Dagor storage without linking the engine's
// diagnostic runtime. Any production assertion is a test failure.
namespace nau::debug
{
    bool isRunningUnderDebugger()
    {
        return false;
    }
}

namespace nau::diag_detail
{
    diag::FailureActionFlag raiseFailure(uint32_t, diag::AssertionKind, diag::SourceInfo, std::string_view, std::string_view)
    {
        std::abort();
    }
}

struct Heap
{
    static inline std::unordered_map<void*, size_t> live;
    Heap(const char* = nullptr)
    {
    }
    void* allocate(size_t bytes, size_t alignment, size_t offset, int)
    {
        NAU_ASSERT(offset == 0);
        auto* p = ::operator new(bytes, std::align_val_t(alignment));
        live.emplace(p, alignment);
        return p;
    }
    void deallocate(void* p, size_t)
    {
        auto alignment = live.at(p);
        live.erase(p);
        ::operator delete(p, std::align_val_t(alignment));
    }
    bool operator==(const Heap&) const
    {
        return true;
    }
};
struct alignas(128) Owner
{
    static inline int alive = 0;
    std::string text;
    std::vector<int> numbers;
    std::unique_ptr<int> owned;
    explicit Owner(int n = 0) :
        text(n % 2 ? "short" : std::string(100, 'x')),
        numbers(3, n),
        owned(std::make_unique<int>(n))
    {
        ++alive;
    }
    Owner(Owner&& other) noexcept :
        text(std::move(other.text)),
        numbers(std::move(other.numbers)),
        owned(std::move(other.owned))
    {
        ++alive;
    }
    Owner& operator=(Owner&&) = default;
    ~Owner()
    {
        --alive;
    }
};
static_assert(!dag::is_type_relocatable<std::string>::value);
static_assert(!dag::is_type_relocatable<std::vector<int>>::value);
static_assert(!dag::is_type_relocatable<Owner>::value);
static_assert(dag::is_type_relocatable<int>::value);
int main()
{
#if defined(_MSC_VER)
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
#endif
    {
        dag::Vector<Owner, Heap> values;
        for (int i = 0; i < 80; ++i)
            values.emplace_back(i);
        NAU_ASSERT(Owner::alive == 80);
        for (int i = 0; i < 80; ++i)
        {
            NAU_ASSERT(*values[i].owned == i && values[i].numbers[2] == i);
            NAU_ASSERT(values[i].text == (i % 2 ? "short" : std::string(100, 'x')));
            NAU_ASSERT(reinterpret_cast<uintptr_t>(&values[i]) % 128 == 0);
        }
        values.erase(values.begin() + 2, values.begin() + 12);
        NAU_ASSERT(values.size() == 70 && Owner::alive == 70 && *values[2].owned == 12);
        values.emplace(values.begin() + 1, 123);
        NAU_ASSERT(Owner::alive == 71 && *values[1].owned == 123);
        auto moved = std::move(values);
        NAU_ASSERT(values.empty() && moved.size() == 71);
        moved.resize(2);
        NAU_ASSERT(Owner::alive == 2 && *moved[1].owned == 123);
        moved.shrink_to_fit();
        NAU_ASSERT(*moved[0].owned == 0);
    }
    NAU_ASSERT(Owner::alive == 0 && Heap::live.empty());
    {
        dag::Vector<std::string, Heap> strings;
        for (int i = 0; i < 40; ++i)
            strings.emplace_back(i % 2 ? "abc" : std::string(100, 'q'));
        auto copied = strings;
        strings.clear();
        for (int i = 0; i < 40; ++i)
            NAU_ASSERT(copied[i] == (i % 2 ? "abc" : std::string(100, 'q')));
        copied.insert(copied.begin() + 1, std::string("inserted"));
        NAU_ASSERT(copied[1] == "inserted");
        dag::Vector<std::string, Heap> assigned;
        assigned = copied;
        NAU_ASSERT(assigned == copied);
    }
    NAU_ASSERT(Heap::live.empty());
#if defined(NAU_PROBE_UNSAFE_RELOCATION)
    dag::RelocatableFixedVector<std::string, 2> invalid;
    invalid.emplace_back("owning value");
#endif
    {
        dag::RelocatableFixedVector<int, 3> indexes;
        indexes.push_back(1);
        indexes.push_back(2);
        const auto snapshot = indexes.getShallowCopy();
        indexes[0] = 9;
        NAU_ASSERT(snapshot.data()[0] == 1);
        for (int i = 3; i < 20; ++i)
            indexes.push_back(i);
        const auto weak = indexes.getShallowCopy();
        NAU_ASSERT(weak.data() == indexes.data() && weak.size() == indexes.size());
        auto moved = std::move(indexes);
        NAU_ASSERT(indexes.empty() && moved[0] == 9);
        moved.erase(moved.begin() + 1, moved.end());
        NAU_ASSERT(moved.size() == 1 && moved[0] == 9 && moved.capacity() == 3);
    }
    std::puts("Dagor vector: standard ownership, growth, erase, move, copy and alignment passed");
}
