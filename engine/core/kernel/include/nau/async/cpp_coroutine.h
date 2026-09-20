// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once

#if !defined(__cpp_impl_coroutine) || __cpp_impl_coroutine < 201902L
    #error "Nau requires C++20 coroutine support"
#endif

#include <coroutine>
namespace CoroNs = std;
