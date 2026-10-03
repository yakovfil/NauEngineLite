// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#pragma once

#include <nau/dataBlock/dag_dataBlock.h>
#include <nau/utils/dag_oaHashNameMap.h>

#include <string>
#include <vector>
// #include <memory/dag_fixedBlockAllocator.h>
#include <nau/threading/dag_atomic.h>
#include <nau/utils/dag_oaHashNameMap.h>

#include <atomic>

#include "nau/memory/mem_allocator.h"

namespace nau
{

#define DATABLOCK_USES_FIXED_BLOCK_ALLOCATOR 0
    static constexpr uint32_t IS_NAMEMAP_ID = 0x80000000;
    static inline bool is_string_id_in_namemap(uint32_t id)
    {
        return id & IS_NAMEMAP_ID;
    };
    static inline uint32_t namemap_id_from_string_id(uint32_t id)
    {
        return id & ~IS_NAMEMAP_ID;
    };

    struct DBNameMapBase : public dag::OAHashNameMap<false>
    {
    };
    struct DBNameMap : public DBNameMapBase
    {
        void addRef() const
        {
            usageRefCount.fetch_add(1);
        }
        int delRef() const
        {
            return usageRefCount.fetch_add(-1);
        }
        int getUsageRefs() const
        {
            return usageRefCount.load();
        }

    private:
        mutable std::atomic_int usageRefCount{0};
    };

    template <class To, class From>
    static __forceinline To memcpy_cast(const From& f)
    {
        To ret;
        memcpy(&ret, &f, sizeof(To));
        return ret;
    }

    template <class To>
    static __forceinline To memcpy_cast(const char* f)
    {
        To ret;
        memcpy(&ret, f, sizeof(To));
        return ret;
    }

    struct DataBlockOwned  // not thread safe
    {
        std::vector<char> data;

    public:
        uint32_t allocate(uint32_t sz)
        {
            uint32_t at = data.size();
            data.resize(at + sz);
            return at;
        }

        char* getUnsafe(uint32_t at)
        {
            return data.data() + at;
        }
        const char* getUnsafe(uint32_t at) const
        {
            return data.data() + at;
        }
        char* get(uint32_t at)
        {
            NAU_ASSERT(data.size() > at, "{} sz at = {}", data.size(), at);
            return getUnsafe(at);
        }
        const char* get(uint32_t at) const
        {
            NAU_ASSERT(data.size() > at, "{} sz at = {}", data.size(), at);
            return getUnsafe(at);
        }

        char* insertAt(uint32_t at, uint32_t n)
        {
            NAU_ASSERT(data.size() >= at, "sz {} at {} n {}", data.size(), at, n);
            data.insert(data.begin() + at, n, char());
            return data.data() + at;
        }
        char* insertAt(uint32_t at, uint32_t n, const char* v)
        {
            NAU_ASSERT(data.size() >= at, "sz {} at {} n {}", data.size(), at, n);
            data.insert(data.begin() + at, n, char());
            auto ret = data.data() + at;
            memcpy(ret, v, n);
            return ret;
        }
    };

    struct DataBlockShared
    {
        const char* getName(uint32_t id) const
        {
            int roc = !ro ? 0 : ro->nameCount();
            return id < roc
                       ? reinterpret_cast<const char*>(ro->getName(id))
                       : reinterpret_cast<const char*>(rw.getName((int)id - roc));  //-V1004
        }
        uint32_t nameCount() const
        {
            return rw.nameCount() + (!ro ? 0 : ro->nameCount());
        }
        bool nameExists(uint32_t id) const
        {
            return id < nameCount();
        }

        int getNameId(const char* name, size_t name_len) const
        {
            auto hash = DBNameMap::string_hash(reinterpret_cast<const char*>(name), name_len);
            const int roc = !ro ? 0 : ro->nameCount();
            if (rw.nameCount())
            {
                int id = rw.getNameId(reinterpret_cast<const char*>(name), name_len, hash);
                if (id >= 0)
                    return id + roc;
            }
            return roc ? ro->getNameId(reinterpret_cast<const char*>(name), name_len, hash) : -1;  //-V1004
        }
        int getNameId(const char* name) const
        {
            return getNameId(name, std::char_traits<char>::length(name));
        }

        int addNameId(const char* name, size_t name_len)
        {
            auto hash = DBNameMap::string_hash(reinterpret_cast<const char*>(name), name_len);
            const int roc = !ro ? 0 : ro->nameCount();
            int id = roc ? ro->getNameId(reinterpret_cast<const char*>(name), name_len, hash) : -1;  //-V1004
            if (id >= 0)
                return id;
            return rw.addNameId(reinterpret_cast<const char*>(name), name_len, hash) + roc;
        }
        int addNameId(const char* name)
        {
            return addNameId(name, std::char_traits<char>::length(name));
        }

        DataBlock* getBlock(uint32_t i)
        {
            NAU_ASSERT(i < roDataBlocks);
            return getROBlockUnsafe(i);
        }
        const DataBlock* getBlock(uint32_t i) const
        {
            NAU_ASSERT(i < roDataBlocks);
            return getROBlockUnsafe(i);
        }

        const DataBlock* getROBlockUnsafe(uint32_t i) const
        {
            return ((const DataBlock*)getUnsafe(blocksStartsAt)) + i;
        }
        DataBlock* getROBlockUnsafe(uint32_t i)
        {
            return ((DataBlock*)getUnsafe(blocksStartsAt)) + i;
        }

        char* getUnsafe(uint32_t at)
        {
            return getDataUnsafe() + at;
        }
        const char* getUnsafe(uint32_t at) const
        {
            return getDataUnsafe() + at;
        }
        char* get(uint32_t at)
        {
            NAU_ASSERT(at < roDataSize());
            return getData() + at;
        }
        const char* get(uint32_t at) const
        {
            NAU_ASSERT(at < roDataSize());
            return getData() + at;
        }

        bool isROBlock(const DataBlock* db) const
        {
            return getROBlockUnsafe(0) <= db && db - getROBlockUnsafe(0) < roDataBlocks;
        }
        uint32_t roDataSize() const
        {
            return blocksStartsAt + roDataBlocks * sizeof(DataBlock) + sizeof(*this);
        }

        // protected:
        DBNameMapBase rw;
        const DBNameMap* ro = nullptr;
        uint32_t roDataBlocks = 0;
        uint32_t blocksStartsAt = 0;

        nau::string srcFilename;  // src filename
        void setSrc(const char* src)
        {
            srcFilename = src;
        }
        const char8_t* getSrc() const
        {
            return !srcFilename.empty() ? srcFilename.c_str() : nullptr;
        }

        enum
        {
            F_ROBUST_LD = 1u << 0,
            F_ROBUST_OPS = 1u << 1,
            F_BINONLY_LD = 1u << 2,
            F_VALID = 1u << 3
        };
        unsigned blkFlags = F_VALID;  // BLK property flags

        unsigned blkRobustLoad() const
        {
            return blkFlags & F_ROBUST_LD;
        }
        unsigned blkRobustOps() const
        {
            return blkFlags & F_ROBUST_OPS;
        }
        unsigned blkBinOnlyLoad() const
        {
            return blkFlags & F_BINONLY_LD;
        }
        unsigned blkValid() const
        {
            return blkFlags & F_VALID;
        }
        void setBlkFlag(unsigned f, bool v)
        {
            v ? blkFlags |= f : blkFlags &= ~f;
        }
        void setBlkRobustLoad(bool v)
        {
            setBlkFlag(F_ROBUST_LD, v);
        }
        void setBlkRobustOps(bool v)
        {
            setBlkFlag(F_ROBUST_OPS, v);
        }
        void setBlkBinOnlyLoad(bool v)
        {
            setBlkFlag(F_BINONLY_LD, v);
        }
        void setBlkValid(bool v)
        {
            setBlkFlag(F_VALID, v);
        }

#if DATABLOCK_USES_FIXED_BLOCK_ALLOCATOR
        FixedBlockAllocator blocksAllocator = {sizeof(DataBlock), 1};
        FixedBlockAllocator dataAllocator = {sizeof(DataBlockOwned), 1};

        void* allocateBlock()
        {
            return blocksAllocator.allocateOneBlock();
        }
        void deallocateBlock(void* p)
        {
            blocksAllocator.freeOneBlock(p);
        }
        void* allocateData()
        {
            return dataAllocator.allocateOneBlock();
        }
        void deallocateData(void* p)
        {
            dataAllocator.freeOneBlock(p);
        }
#else
        void* allocateBlock()
        {
            return nau::getDefaultAllocator()->allocate(sizeof(DataBlock)); /* memalloc(sizeof(DataBlock), midmem);*/
        }
        void deallocateBlock(void* p)
        {
            nau::getDefaultAllocator()->deallocate(p);
        }
        void* allocateData()
        {
            return nau::getDefaultAllocator()->allocate(sizeof(DataBlockOwned)); /*memalloc(sizeof(DataBlockOwned), midmem);*/
        }
        void deallocateData(void* p)
        {
            nau::getDefaultAllocator()->deallocate(p);
        }
#endif

        void shrink_to_fit()
        {
            rw.shrink_to_fit();
        }

        // instead of having additional indirection, we just allocate the 'rest' of DataBlockShared for data (as it is of known size)
        char* getDataUnsafe()
        {
            return ((char*)this + sizeof(DataBlockShared));
        }
        const char* getDataUnsafe() const
        {
            return ((const char*)this + sizeof(DataBlockShared));
        }

        char* getData()
        {
            NAU_ASSERT(roDataBlocks);
            return getDataUnsafe();
        }
        const char* getData() const
        {
            NAU_ASSERT(roDataBlocks);
            return getDataUnsafe();
        }

        friend class DataBlock;
    };

    __forceinline void DataBlock::insertNewParamRaw(uint32_t at, uint32_t name_id, uint32_t type, size_t type_sz, const char* nd)
    {
        NAU_ASSERT(type != TYPE_STRING);
        NAU_ASSERT(isOwned());
        // const uint32_t pcnt = paramCount();
        // uint32_t paramsEnd = pcnt*sizeof(Param);
        // uint32_t paramsVal = paramsVal + pcnt*sizeof(uint32_t);
        Param p;
        p.nameId = name_id;
        p.type = type;
        if (type_sz <= INPLACE_PARAM_SIZE)
        {
            NAU_STATIC_ASSERT(INPLACE_PARAM_SIZE == sizeof(Param::v));
            p.v = 0;
            memcpy(&p.v, nd, type_sz);
            insertAt(at * sizeof(Param), sizeof(Param), reinterpret_cast<char*>(&p));
            paramsCount++;
        }
        else
        {
            p.v = complexParamsSize();
            insertAt(at * sizeof(Param), sizeof(Param), reinterpret_cast<char*>(&p));
            paramsCount++;
            insertAt(getUsedSize() + p.v, (uint32_t)type_sz, nd);
        }
    }

    template <bool rw>
    DataBlock::Param& DataBlock::getParam(uint32_t i)
    {
        NAU_ASSERT(i < paramCount());
        return getParams<rw>()[i];
    }

    template <bool rw>
    const DataBlock::Param& DataBlock::getParam(uint32_t i) const
    {
        NAU_ASSERT(i < paramCount());
        return getCParams<rw>()[i];
    }

    inline DataBlock::Param* DataBlock::getParamsImpl()
    {
        return paramCount() ? (isOwned() ? getParams<true>() : getParams<false>()) : nullptr;
    }
    inline const DataBlock::Param* DataBlock::getParamsImpl() const
    {
        return paramCount() ? (isOwned() ? getParams<true>() : getParams<false>()) : nullptr;
    }

    namespace dblk
    {
        enum FormatHeaderByte
        {
            BBF_full_binary_in_stream = '\1',     // complete BLK with private namemap in binary stream follows to the end of the file
            BBF_full_binary_in_stream_z = '\2',   // 3 bytes (compressed data size) and then complete BLK with private namemap in ZSTD compressed
                                                  // binary stream follows
            BBF_binary_with_shared_nm = '\3',     // BLK (using shared namemap) in binary stream follows to the end of the file
            BBF_binary_with_shared_nm_z = '\4',   // BLK (using shared namemap) in ZSTD compressed binary stream follows to the end of the file
            BBF_binary_with_shared_nm_zd = '\5',  // BLK (using shared namemap) in ZSTD compressed (with dict) binary stream follows to the end of
                                                  // the file
        };

        bool check_shared_name_map_valid(const VirtualRomFsData* fs, const char** out_err_desc);
        bool add_name_to_name_map(DBNameMap& nm, const char* s);

        bool read_names_base(iosys::IGenLoad& cr, DBNameMapBase& names, uint64_t* names_hash);
        bool write_names_base(iosys::IGenSave& cwr, const DBNameMapBase& names, uint64_t* names_hash);

    }  // namespace dblk
}  // namespace nau