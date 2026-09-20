// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once

#include <cstdint>
#define NAU_ENGINE

#if defined(__EMSCRIPTEN__)
#define NAU_PLATFORM_EMSCRIPTEN 1
#elif defined(__unix__)
#define NAU_PLATFORM_LINUX 1
#endif

#ifdef __APPLE__
#define NAU_PLATFORM_APPLE 1
#endif

#if defined(_WIN32) || defined (WIN32)

#define NAU_PLATFORM_WIN32 1

#if defined(_WIN64)
#define NAU_64BIT 1
#endif

#endif // _WIN32

#ifndef UNALIGNED
#if defined(__SNC__)
#define UNALIGNED __unaligned
#else
#define UNALIGNED
#endif
#endif

#if defined(__cplusplus) && !defined(__GNUC__)
template <typename T, size_t N>
char (&_countof__helper_(T (&array)[N]))[N];
#define countof(x) (sizeof(_countof__helper_(x)))
#else
#define countof(x) (sizeof(x) / sizeof((x)[0]))
#endif
#define c_countof(x) (sizeof(x) / sizeof((x)[0]))

