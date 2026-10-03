// Copyright 2026 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <stdexcept>
#include <string>

#include "nau/utils/tuple_vector.h"
void check(bool value)
{
    if (!value)
        std::abort();
}
struct alignas(128) Value
{
    static inline int alive = 0;
    int number;
    Value(int n = 0) :
        number(n)
    {
#if defined(__cpp_exceptions) || defined(_CPPUNWIND)
        if (n == -1)
            throw std::runtime_error("fixture");
#endif
        ++alive;
    }
    Value(const Value& other) :
        Value(other.number)
    {
    }
    Value(Value&& other) noexcept :
        number(other.number)
    {
        ++alive;
    }
    Value& operator=(const Value&) = default;
    ~Value()
    {
        --alive;
    }
};
int main()
{
#if defined(_MSC_VER)
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
#endif
    {
        nau::TupleVector<std::string, std::unique_ptr<int>, Value> rows;
        for (int i = 0; i < 60; ++i)
            rows.emplace_back(std::to_string(i), std::make_unique<int>(i), i);
        check(rows.size() == 60 && Value::alive == 60);
        for (int i = 0; i < 60; ++i)
        {
            check(rows.get<0>()[i] == std::to_string(i));
            check(*rows.get<1>()[i] == i && rows.get<Value>()[i].number == i);
            check(reinterpret_cast<uintptr_t>(&rows.get<Value>()[i]) % 128 == 0);
        }
#if defined(__cpp_exceptions) || defined(_CPPUNWIND)
        bool caught = false;
        try
        {
            rows.emplace_back("bad", std::make_unique<int>(-1), -1);
        }
        catch (const std::runtime_error&)
        {
            caught = true;
        }
        check(caught && rows.size() == 60 && Value::alive == 60);
        check(rows.get<0>()[59] == "59" && *rows.get<1>()[59] == 59);
#endif
        rows.erase(rows.begin() + 1, rows.begin() + 11);
        check(rows.size() == 50 && Value::alive == 50);
        check(rows.get<0>()[1] == "11" && *rows.get<1>()[1] == 11 && rows.get<2>()[1].number == 11);
        auto moved = std::move(rows);
        check(rows.empty() && moved.size() == 50);
        moved.resize(2);
        check(Value::alive == 2 && moved.get<2>()[1].number == 11);
    }
    check(Value::alive == 0);
    nau::TupleVector<int, std::string> values;
    values.emplace_back(1, "one");
    values.emplace_back(2, "two");
    auto copied = values;
    values.clear();
    check(copied.get<int>()[1] == 2 && copied.get<std::string>()[1] == "two");
    copied.resize(4);
    check(copied.get<int>()[2] == 0 && copied.get<std::string>()[2].empty());
    values = copied;
    check(values.size() == 4);
    values.erase(values.begin() + 1);
    check(values.size() == 3 && values.get<int>()[1] == 0);
    std::puts("tuple columns: alignment, row correspondence, moves, erase and rollback passed");
}
