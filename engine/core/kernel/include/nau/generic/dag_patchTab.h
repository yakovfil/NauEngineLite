// Copyright 2024 N-GINN LLC. All rights reserved.
// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>

/// @addtogroup containers
/// @{

/// Template array that has method for patching pointer address.
/// Explicit pointer/count layout preserves serialized dumps independently of STL layout.
template <class T>
class PatchableTab
{
public:
    using value_type = std::remove_cv_t<T>;
    using index_type = std::size_t;
    using iterator = T*;
    using const_iterator = const T*;
    using reference = T&;
    using const_reference = const T&;
    reference operator[](index_type index) const
    {
        return mpData[index];
    }
    iterator begin() const
    {
        return mpData;
    }
    iterator end() const
    {
        return mnSize ? mpData + mnSize : mpData;
    }
    const_iterator cbegin() const
    {
        return begin();
    }
    const_iterator cend() const
    {
        return end();
    }
    reference front() const
    {
        return mpData[0];
    }
    reference back() const
    {
        return mpData[mnSize - 1];
    }
    T* data() const
    {
        return mpData;
    }
    index_type size() const
    {
        return mnSize;
    }
    index_type size_bytes() const
    {
        return mnSize * sizeof(T);
    }
    bool empty() const
    {
        return mnSize == 0;
    }

    PatchableTab()
    {
    }
    ~PatchableTab()
    {
    }

    PatchableTab(PatchableTab&& ft) = default;
    PatchableTab& operator=(PatchableTab&& ft) = default;

    /// Add specified base address to data address (pointer).
    void patch(void* base)
    {
#if NAU_64BIT
    #if _TARGET_BE
        mnSize = intptr_t(mpData) & 0xFFFFFFFF;
        mpData = mnSize ? (T*)((ptrdiff_t(mpData) >> 32) + ptrdiff_t(base)) : NULL;
    #else

        mnSize = intptr_t(mpData) >> 32;
        mpData = mnSize ? (T*)((ptrdiff_t(mpData) & 0xFFFFFFFF) + ptrdiff_t(base)) : NULL;
    #endif
#else
        if (mnSize)
            mpData = (T*)(ptrdiff_t(mpData) + ptrdiff_t(base));
#endif
    }

    /// Rebases already patched data address (useful for fast cloning)
    void rebase(void* newbase, const void* oldbase)
    {
        if (mnSize)
            mpData = (T*)(ptrdiff_t(mpData) + ptrdiff_t(newbase) - ptrdiff_t(oldbase));
    }

    /// Explicitly initialize the pointer/count view (useful when when constructing PatchableTab not from load only)
    void init(void* base, intptr_t cnt)
    {
        mpData = (T*)base;
        mnSize = cnt;
    }

private:
    T* mpData = nullptr;
    index_type mnSize = 0;

    PatchableTab(const PatchableTab& ft);
    PatchableTab& operator=(const PatchableTab& ft);

#if !NAU_64BIT
    int _resv[2];
#endif
};
/// @}

#pragma pack(push, 4)  // this pointers are not aligned at least in dag::RoNameMap::map
template <class T>
class PatchablePtr
{
public:
    PatchablePtr()
    {
    }  //-V730
    ~PatchablePtr()
    {
    }

    T* get() const
    {
        return p;
    }
    operator T*() const
    {
        return p;
    }

    const T* operator->() const
    {
        return p;
    }
    T* operator->()
    {
        return p;
    }

    /// reinterprets initial value as int (must be used only before patch!)
    int toInt() const
    {
        return int(intptr_t(p));
    }

    /// Add specified base address to data address (pointer).
    void patch(void* base)
    {
        p = int(ptrdiff_t(p) & 0xFFFFFFFF) >= 0 ? (T*)((ptrdiff_t(p) & 0xFFFFFFFF) + ptrdiff_t(base)) : NULL;
    }

    /// Add specified base address to data address (pointer). When offset is 0, pointer is treated as NULL
    void patchNonNull(void* base)
    {
        p = int(ptrdiff_t(p) & 0xFFFFFFFF) > 0 ? (T*)((ptrdiff_t(p) & 0xFFFFFFFF) + ptrdiff_t(base)) : NULL;
    }

    /// Rebases already patched data address (useful for fast cloning)
    void rebase(void* newbase, const void* oldbase)
    {
        p = p ? (T*)(ptrdiff_t(p) + ptrdiff_t(newbase) - ptrdiff_t(oldbase)) : NULL;
    }

    /// Expicitly assigns new pointer
    void operator=(const T* new_p)
    {
        p = (T*)new_p;
    }
    void setPtr(const void* new_p)
    {
        p = (T*)new_p;
    }

#if NAU_64BIT
    void clearUpperBits()
    {
    }
#else
    void clearUpperBits()
    {
        _resv = 0;
    }
#endif

private:
    T* p;

#if !NAU_64BIT
    int _resv;
#endif
};
#pragma pack(pop)

#if NAU_64BIT
    #define PATCHABLE_64BIT_PAD32(NM) int NM
    #define PATCHABLE_32BIT_PAD32(NM)
    #define PATCHABLE_DATA64(T, NM) T NM
enum
{
    PATCHABLE_64BIT_PAD32_SZ = 4,
    PATCHABLE_32BIT_PAD32_SZ = 0,
};
#else
    #define PATCHABLE_64BIT_PAD32(NM)
    #define PATCHABLE_32BIT_PAD32(NM) int NM
    #define PATCHABLE_DATA64(T, NM) \
        T NM;                       \
        int _##NM##HDW;
enum
{
    PATCHABLE_64BIT_PAD32_SZ = 0,
    PATCHABLE_32BIT_PAD32_SZ = 4,
};
#endif
