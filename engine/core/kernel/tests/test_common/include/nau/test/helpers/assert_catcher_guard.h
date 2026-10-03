// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once

namespace nau::test
{
    struct AssertCatcherGuard
    {
        AssertCatcherGuard();
        ~AssertCatcherGuard();

        size_t assertFailureCounter = 0;
        size_t fatalFailureCounter = 0;
    };
}  // namespace nau::test
