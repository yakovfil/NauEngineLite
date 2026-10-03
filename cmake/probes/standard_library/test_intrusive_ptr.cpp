// Copyright 2026 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include <cstdio>
#include <cstdlib>

#include "nau/memory/intrusive_ptr.h"

void check(bool condition)
{
    if (!condition)
        std::abort();
}

struct Lease
{
    mutable unsigned references = 0;
    mutable unsigned returns = 0;

private:
    template <typename T>
    friend void nau::intrusive_ptr_add_ref(T*);
    template <typename T>
    friend void nau::intrusive_ptr_release(T*);
    void AddRef() const
    {
        ++references;
    }
    void Release() const
    {
        check(references != 0);
        if (--references == 0)
            ++returns;
    }
};

namespace custom
{
    struct Lease
    {
        int references = 0;
    };
    void intrusive_ptr_add_ref(Lease* value)
    {
        ++value->references;
    }
    void intrusive_ptr_release(Lease* value)
    {
        --value->references;
    }
}  // namespace custom

int main()
{
    using Pointer = nau::IntrusivePtr<Lease>;
    static_assert(sizeof(Pointer) == sizeof(Lease*));
    Lease lease;
    {
        Pointer first = &lease;
        auto copy = first;
        check(lease.references == 2);
        first = first;
        first = std::move(first);
        first.reset(first.get());
        check(lease.references == 2 && lease.returns == 0);
        nau::IntrusivePtr<const Lease> constant = copy;
        nau::IntrusivePtr<const Lease> moved = std::move(copy);
        check(!copy && lease.references == 3);
        check(moved == first && constant == first);
        constant = nullptr;
        check(lease.references == 2);
        Lease* adopted = first.detach();
        check(!first && lease.references == 2);
        Pointer owner(adopted, false);
        Pointer empty;
        swap(empty, owner);
        check(!owner && empty.get() == &lease);
        moved.reset();
        check(lease.references == 1 && lease.returns == 0);
    }
    check(lease.references == 0 && lease.returns == 1);
    custom::Lease customLease;
    {
        nau::IntrusivePtr<custom::Lease> value = &customLease;
        check(customLease.references == 1);
    }
    check(customLease.references == 0);
    std::puts("intrusive pointer: copy/move/adopt/hooks/final return passed");
}
