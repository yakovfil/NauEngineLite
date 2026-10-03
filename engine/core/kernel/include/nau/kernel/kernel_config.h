// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.
// nau/kernel/kernel_config.h

#pragma once

#if !defined(NAU_STATIC_RUNTIME)

    #ifdef _MSC_VER
        #ifdef NAU_KERNEL_BUILD
            #define NAU_KERNEL_EXPORT __declspec(dllexport)
        #else
            #define NAU_KERNEL_EXPORT __declspec(dllimport)
        #endif

    #else
        #error Unknown Compiler/OS
    #endif

#else
    #define NAU_KERNEL_EXPORT
#endif

#if defined(__clang__)
    #if __has_feature(cxx_rtti)
        #define NAU_CXX_RTTI
    #endif
#elif defined(__GNUC__)
    #if defined(__GXX_RTTI)
        #define NAU_CXX_RTTI
    #endif
#elif defined(_MSC_VER)
    #if defined(_CPPRTTI)
        #define NAU_CXX_RTTI
    #endif
#endif

#ifndef NDEBUG
    #ifndef _DEBUG
        #define _DEBUG 1
    #endif
    #ifndef DEBUG
        #define DEBUG 1
    #endif
#endif

#if defined(_DEBUG) || !defined(NDEBUG) || DEBUG
    #define NAU_DEBUG 1
#else
    #define NAU_DEBUG 0
#endif

#if !defined(NAU_ASSERT_ENABLED)
    #ifdef _DEBUG
        #define NAU_ASSERT_ENABLED
    #endif
#endif

#ifndef NAU_NOINLINE
    #if defined(__GNUC__)
        #define NAU_NOINLINE __attribute__((noinline))
    #elif _MSC_VER >= 1300
        #define NAU_NOINLINE __declspec(noinline)
    #else
        #define NAU_NOINLINE
    #endif
#endif

#ifndef NAU_LIKELY
    #if (defined(__GNUC__) && (__GNUC__ >= 3)) || defined(__clang__)
        #if defined(__cplusplus)
            #define NAU_LIKELY(x) __builtin_expect(!!(x), true)
            #define NAU_UNLIKELY(x) __builtin_expect(!!(x), false)
        #else
            #define NAU_LIKELY(x) __builtin_expect(!!(x), 1)
            #define NAU_UNLIKELY(x) __builtin_expect(!!(x), 0)
        #endif
    #else
        #define NAU_LIKELY(x) (x)
        #define NAU_UNLIKELY(x) (x)
    #endif
#endif

#ifndef NAU_THREAD_SANITIZER
    #if defined(__has_feature)
        #if __has_feature(thread_sanitizer)
            #define NAU_THREAD_SANITIZER 1
        #endif
    #else
        #if defined(__SANITIZE_THREAD__)
            #define NAU_THREAD_SANITIZER 1
        #endif
    #endif
#endif

#ifndef NAU_ADDRESS_SANITIZER
    #if defined(__has_feature)
        #if __has_feature(address_sanitizer)
            #define NAU_ADDRESS_SANITIZER 1
        #endif
    #else
        #if defined(__SANITIZE_ADDRESS__)
            #define NAU_ADDRESS_SANITIZER 1
        #endif
    #endif
#endif

#if !defined(NAU_FORCE_INLINE)
    #if defined(_MSC_VER)
        #define NAU_FORCE_INLINE __forceinline
    #elif defined(__GNUC__) || defined(__clang__)
        #define NAU_FORCE_INLINE inline __attribute__((always_inline))
    #else
        #define NAU_FORCE_INLINE inline
    #endif
#endif

#if !defined(NAU_UNUSED)
    #define NAU_UNUSED(x) ((void)(x))
#endif

#ifdef __analysis_assume
    #define NAU_ANALYSIS_ASSUME __analysis_assume
#else
    #define NAU_ANALYSIS_ASSUME(expr)
#endif

#define NAU_THREADS_ENABLED

#if _WIN64 || defined(__LP64__)
    #define _TARGET_64_BIT 1
#endif  //  _WIN64 || defined(__LP64__)
