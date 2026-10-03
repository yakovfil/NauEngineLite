// Copyright 2026 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include <cstring>
#include <memory>
#include <mutex>
#include <span>
#include <vector>

#include "nau/utils/lru_cache.h"
#if defined(_MSC_VER) && defined(_DEBUG)
    #include <crtdbg.h>
#endif

void check(bool condition)
{
    if (!condition)
    {
        std::fputs("LRU assertion failed\n", stderr);
        std::abort();
    }
}

int main(int argc, char** argv)
{
#if defined(_MSC_VER)
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
    #if defined(_DEBUG)
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);
    #endif
#endif
    std::vector<int> released;
    int created = 0;
    nau::LruCache<int, int> cache(2, [&](const int& key)
    {
        ++created;
        return key * 10;
    }, [&](const int& value)
    {
        released.push_back(value);
    });
    if (argc == 2)
    {
        cache.resize(0);
        if (std::strcmp(argv[1], "--zero-insert") == 0)
        {
            cache.insert(1, 10);
        }
        else
        {
            nau::LruCache<int, int> zero(0, [](const int&)
            {
                std::fputs("CREATOR_CALLED", stderr);
                return 0;
            });
            zero.get(1);
        }
        return 1;
    }
    check(cache.get(1) == 10 && cache.get(2) == 20 && created == 2);
    int* stable = &cache.get(1);
    cache.get(3);
    check(released == std::vector<int>{20});
    check(stable == &cache.get(1) && *stable == 10);
    check(!cache.insert(3, 300));
    cache.get(4);  // Duplicate insertion must not promote 3.
    check(released == std::vector<int>({20, 30}));
    check(cache.at(1) == 10 && !cache.at(99));
    check(cache.touch(1) && !cache.touch(99));
    cache.insert_or_assign(1, 11);
    check(released.back() == 10 && cache.get(1) == 11);
    cache.resize(1);
    check(released.back() == 40 && cache.size() == 1);
    cache.resize(0);
    check(released.back() == 11 && cache.empty() && cache.capacity() == 0);
    check(!cache.contains(1) && !cache.touch(1) && !cache.erase(1));
    cache.clear();
    cache.resize(2);
    cache.insert(7, 70);
    check(cache.erase(7) && !cache.erase(7) && released.back() == 70);

    int returned = 0;
    {
        nau::LruCache<int, std::unique_ptr<int>> resources(1, {}, [&](const auto& value)
        {
            check(bool(value));
            ++returned;
        });
        resources.insert(1, std::make_unique<int>(1));
        resources.insert(2, std::make_unique<int>(2));
        check(returned == 1);
    }
    check(returned == 2);
    {
        // Shader decompression consumers borrow bytes under an external lock.
        std::mutex lock;
        nau::LruCache<int, std::vector<unsigned char>> groups(2);
        std::lock_guard guard(lock);
        groups.insert(1, std::vector<unsigned char>{1, 2, 3});
        const std::span<const unsigned char> bytes(groups.get(1));
        groups.insert(2, std::vector<unsigned char>{4, 5});
        groups.touch(1);
        check(bytes.data() == groups.get(1).data() && bytes[2] == 3);
        groups.insert(3, std::vector<unsigned char>{6});
        check(!groups.contains(2) && bytes[0] == 1);
        // Do not use the borrowed span after erasing its owning entry.
        groups.erase(1);
    }
    std::puts("LRU recency, duplicates, callback lifecycle, resize and reference tests passed");
}
