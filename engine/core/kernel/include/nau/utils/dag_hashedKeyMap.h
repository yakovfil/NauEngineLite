// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved

#pragma once

#include <nau/math/dag_adjpow2.h>
#include <wyhash.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <type_traits>
#include <utility>
#if !defined(NAU_ASSERT) || !defined(NAU_FAST_ASSERT)
    #include <nau/diag/assertion.h>
#endif

// this is simple Open Addressing hash table with separated (hashed, integral type) keys and values map. Keys==EmptyKey Key(), are
// special and not allowed although that results in two cache miss in a case of hit, it also allows keys to be of a different size than
// value yet keeping alignment also it is faster on miss then 'tied' Load factor is integral, not a float for performance reasons. By
// default it is 3/4 (75%). Even 9/10 should be fast enough in most cases, but 1/2 is fastest
//! erase is slow as can cause rehashing!

// Empty tables reference their own sentinel; moving/swapping must rebind that pointer.
// Raw value storage supports trivially copyable values only. Allocators move with
// storage on swap; copy assignment allocates through the destination allocator.

namespace oa_hashmap_util
{
    template <class T, class Allocator>
    T* allocate(Allocator& allocator, size_t count)
    {
        using Typed = typename std::allocator_traits<Allocator>::template rebind_alloc<T>;
        Typed typed(allocator);
        if (count > std::allocator_traits<Typed>::max_size(typed))
        {
            std::fputs("Nau hashed container allocation size overflow\n", stderr);
            std::abort();
        }
        auto* pointer = std::allocator_traits<Typed>::allocate(typed, count);
        if (!pointer && count)
        {
            std::fputs("Nau hashed container allocation failed\n", stderr);
            std::abort();
        }
        return pointer;
    }
    template <class T, class Allocator>
    void deallocate(Allocator& allocator, T* pointer, size_t count)
    {
        using Typed = typename std::allocator_traits<Allocator>::template rebind_alloc<T>;
        Typed typed(allocator);
        std::allocator_traits<Typed>::deallocate(typed, pointer, count);
    }

    template <class Key>
    struct NoHash
    {
        inline uint32_t hash(const Key& k)
        {
            return uint32_t(k);
        }
    };
    template <class Key>
    struct MumStepHash
    {
        inline uint32_t hash(const Key& k)
        {
            return wyhash64_const(uint64_t(k), _wyp[0]);
        }
    };

    inline uint32_t min(uint32_t a, uint32_t b)
    {
        return a < b ? a : b;
    }
    inline uint32_t max(uint32_t a, uint32_t b)
    {
        return a < b ? b : a;
    }

    template <class Key, Key EmptyKey, class Hasher>
    inline uint32_t emplace_new(Key key, Key* __restrict to, uint32_t mask)
    {
        const uint32_t bucketI = Hasher().hash(key) & mask;
        auto* __restrict tp = to + bucketI, * __restrict e = to + mask + 1;
        for (;;)
        {
            do
            {
                if (*tp == EmptyKey)
                {
                    *tp = key;
                    return (uint32_t)(tp - to);
                }
            } while ((++tp) != e);
            tp = to;
            e = to + bucketI;
        }
    }

    template <class Key, Key EmptyKey, class Hasher>
    inline uint32_t emplace(Key key, Key* __restrict to, uint32_t mask, bool& emplaced_new)
    {
        const uint32_t bucketI = Hasher().hash(key) & mask;
        auto* __restrict tp = to + bucketI, * __restrict e = to + mask + 1;
        for (;;)
        {
            do
            {
                if (*tp == EmptyKey)
                {
                    *tp = key;
                    emplaced_new = true;
                    return (uint32_t)(tp - to);
                }
                if (*tp == key)
                {
                    return (uint32_t)(tp - to);
                }
            } while ((++tp) != e);
            tp = to;
            e = to + bucketI;
        }
    }

    template <class Key, Key EmptyKey, class Hasher>  // return true, if was missing
    inline bool emplace_if_missing(Key key, Key* __restrict to, uint32_t mask)
    {
        const uint32_t bucketI = Hasher().hash(key) & mask;
        auto* __restrict tp = to + bucketI, * __restrict e = to + mask + 1;
        for (;;)
        {
            do
            {
                if (*tp == EmptyKey)
                {
                    *tp = key;
                    return true;
                }
                if (*tp == key)
                    return false;
            } while ((++tp) != e);
            tp = to;
            e = to + bucketI;
        }
    }

    template <class Key, Key EmptyKey, class Hasher, class KeyIndexCB>
    inline const Key* find_key(Key key, const Key* __restrict begin, uint32_t mask, KeyIndexCB cb)
    {
        const uint32_t hashedKey = Hasher().hash(key);
        uint32_t bucketI = hashedKey & mask, bucketE = mask + 1;
        for (;;)
        {
            do
            {
                Key k = begin[bucketI];
                if (k == EmptyKey)
                    return nullptr;
                if (k == key && cb(bucketI))
                    return &begin[bucketI];
            } while ((++bucketI) != bucketE);
            // since we don't allow fully loaded HT, it can fall through once, in the end of table. after that we will definetly find one 0 or
            // key itself
            bucketE = (hashedKey & mask);
            bucketI = 0;
        }
    }

    template <class Key, Key EmptyKey, class Hasher, class Val, class ValCB>
    const Val find_or(Key key, const Key* __restrict begin, const Val* __restrict vals, const Val& def, uint32_t mask, ValCB cb)
    {
        const uint32_t hashedKey = Hasher().hash(key);
        uint32_t bucketI = hashedKey & mask, bucketE = mask + 1;
        for (;;)
        {
            do
            {
                Key k = begin[bucketI];
                if (k == EmptyKey)
                    return def;
                if (k == key)
                {
                    const Val& v = vals[bucketI];
                    if (cb(v))
                        return v;
                }
            } while ((++bucketI) != bucketE);
            // since we don't allow fully loaded HT, it can fall through once, in the end of table. after that we will definetly find one 0 or
            // key itself
            bucketE = (hashedKey & mask);
            bucketI = 0;
        }
    }

    template <class Key, Key EmptyKey, class Hasher, class Val, class ValCB>
    Val* find_val(Key key, const Key* __restrict begin, Val* __restrict vals, uint32_t mask, ValCB cb)
    {
        const uint32_t hashedKey = Hasher().hash(key);
        uint32_t bucketI = hashedKey & mask, bucketE = mask + 1;
        for (;;)
        {
            do
            {
                Key k = begin[bucketI];
                if (k == EmptyKey)
                    return nullptr;
                if (k == key)
                {
                    Val& v = vals[bucketI];
                    if (cb(v))
                        return &v;
                }
            } while ((++bucketI) != bucketE);
            // since we don't allow fully loaded HT, it can fall through once, in the end of table. after that we will definetly find one 0 or
            // key itself
            bucketE = (hashedKey & mask);
            bucketI = 0;
        }
    }

    template <class Val>
    struct DefaultValCB
    {
        bool operator()(const Val& /*containerVal*/)
        {
            return true;
        }
    };

}  // namespace oa_hashmap_util

namespace dag
{

    template <class Key, class Value, Key EmptyKey = Key(), class Hasher = oa_hashmap_util::NoHash<Key>, class allocator_type = std::allocator<std::byte>, int load_factor_nom = 3, int load_factor_denom = 4>
    struct HashedKeyMap
    {
        static_assert(std::is_trivially_copyable_v<Key> && std::is_trivially_copyable_v<Value>, "Nau hashed maps require trivially copyable keys and values");
        typedef Key key_t;
        typedef Value val_t;
        typedef HashedKeyMap<Key, Value, EmptyKey, Hasher, allocator_type, load_factor_nom, load_factor_denom> this_type_t;
        void emplace(key_t key, const val_t& input)
        {
            const val_t val = input;
            NAU_FAST_ASSERT(key != EmptyKey);
            if (used >= mask * load_factor_nom / load_factor_denom) [[unlikely]]
                rehash(oa_hashmap_util::max(4, mask + mask + 2));
            vals[oa_hashmap_util::emplace_new<key_t, EmptyKey, Hasher>(key, keys.first, mask)] = val;
            used++;
        }
        bool emplace_if_missing(key_t key, const val_t& input)
        {
            const val_t val = input;
            NAU_FAST_ASSERT(key != EmptyKey);
            if (used >= mask * load_factor_nom / load_factor_denom) [[unlikely]]
                rehash(oa_hashmap_util::max(4, mask + mask + 2));
            bool emplacedNew = false;
            vals[oa_hashmap_util::emplace<key_t, EmptyKey, Hasher>(key, keys.first, mask, emplacedNew)] = val;
            used += uint32_t(emplacedNew);
            return emplacedNew;
        }
        std::pair<val_t*, bool> emplace_if_missing(key_t key)
        {
            NAU_FAST_ASSERT(key != EmptyKey);
            if (used >= mask * load_factor_nom / load_factor_denom) [[unlikely]]
                rehash(oa_hashmap_util::max(4, mask + mask + 2));
            bool emplacedNew = false;
            auto id = oa_hashmap_util::emplace<key_t, EmptyKey, Hasher>(key, keys.first, mask, emplacedNew);
            used += uint32_t(emplacedNew);
            return std::pair<val_t*, bool>{vals + id, emplacedNew};
        }
        void emplace_new(key_t key, const val_t& input)  // that's unsafe emplace, it requires that key hasn't been added before
        {
            const val_t val = input;
#if DAGOR_DBGLEVEL > 1  // ensure constraints
            NAU_ASSERT(!has_key(key));
#endif
            emplace(key, val);
        }

        template <class ValCB = oa_hashmap_util::DefaultValCB<val_t>>
        void erase(key_t key, ValCB cb = ValCB())
        {
            const key_t* k =
                oa_hashmap_util::find_key<key_t, EmptyKey, Hasher>(key, keys.first, mask, [&](uint32_t i)
            {
                return cb(vals[i]);
            });
            if (k != nullptr)
            {
                const size_t ind = k - keys.first;
                keys.first[ind] = EmptyKey;
                --used;
                if (keys.first[(ind + 1) & mask] != EmptyKey)
                    rehash(get_capacity(used));
            }
        }
        template <class ValCB = oa_hashmap_util::DefaultValCB<val_t>>
        const val_t findOr(key_t key, const val_t& def, ValCB cb = ValCB()) const
        {
            return oa_hashmap_util::find_or<key_t, EmptyKey, Hasher>(key, keys.first, vals, def, mask, [&](const auto& v)
            {
                return cb(v);
            });
        }

        template <class ValCB = oa_hashmap_util::DefaultValCB<val_t>>
        val_t* findVal(key_t key, ValCB cb = ValCB())
        {
            return oa_hashmap_util::find_val<key_t, EmptyKey, Hasher>(key, keys.first, vals, mask, [&](const auto& v)
            {
                return cb(v);
            });
        }

        template <class ValCB = oa_hashmap_util::DefaultValCB<val_t>>
        const val_t* findVal(key_t key, ValCB cb = ValCB()) const
        {
            return const_cast<this_type_t*>(this)->findVal(key, cb);
        }

        template <class ValCB = oa_hashmap_util::DefaultValCB<val_t>>
        bool has_key(key_t key, ValCB cb = ValCB()) const
        {
            return oa_hashmap_util::find_key<key_t, EmptyKey, Hasher>(key, keys.first, mask, [&](uint32_t i)
            {
                return cb(vals[i]);
            }) != nullptr;
        }

        void shrink_to_fit()
        {
            const uint32_t new_capacity = get_capacity(used);
            if (new_capacity < bucket_count())
                rehash(new_capacity);
        }
        void clear(uint32_t shrink_sz = 0)
        {
            const uint32_t new_capacity = get_capacity(shrink_sz);
            if (shrink_sz && bucket_count() != new_capacity)
            {
                used = 0;
                rehash(new_capacity);
            }
            else
            {
                if (used)
                {
                    if (EmptyKey == 0)
                        memset(keys.first, 0, bucket_count() * sizeof(key_t));
                    else
                        std::fill_n(keys.first, bucket_count(), EmptyKey);
                }
                used = 0;
            }
        }
        static inline size_t get_capacity(size_t sz)
        {
            return oa_hashmap_util::max(4, nau::math::get_bigger_pow2(sz * load_factor_denom / load_factor_nom));
        }
        uint32_t bucket_count() const
        {
            return mask + 1;
        }
        uint32_t size() const
        {
            return used;
        }
        bool empty() const
        {
            return size() == 0;
        }
        void reserve(uint32_t sz)
        {
            uint32_t newBuckets = get_capacity(sz);
            if (newBuckets > bucket_count())
                rehash(newBuckets);
        }
        struct iterator
        {
            this_type_t& map;
            key_t *current, *end;
            iterator(this_type_t& m, key_t* c, key_t* e) :
                map(m),
                current(c),
                end(e)
            {
                advance();
            }
            key_t& key()
            {
                return *current;
            }
            val_t& val()
            {
                return *(map.vals + (current - map.keys.first));
            }
            iterator& operator++()
            {
                ++current;
                advance();
                return *this;
            }
            inline void advance()
            {
                for (; current != end && *current == EmptyKey; ++current)
                    ;
            }
            bool operator!=(const iterator& a) const
            {
                return current != a.current;
            }
            bool operator==(const iterator& a) const
            {
                return current == a.current;
            }
            iterator& operator*()
            {
                return *this;
            }
        };
        struct const_iterator
        {
            const this_type_t& map;
            const key_t *current, *end;
            const_iterator(const this_type_t& m, const key_t* c, const key_t* e) :
                map(m),
                current(c),
                end(e)
            {
                advance();
            }
            const key_t& key() const
            {
                return *current;
            }
            const val_t& val() const
            {
                return *(map.vals + (current - map.keys.first));
            }
            const_iterator& operator++()
            {
                ++current;
                advance();
                return *this;
            }
            inline void advance()
            {
                for (; current != end && *current == EmptyKey; ++current)
                    ;
            }
            bool operator!=(const const_iterator& a) const
            {
                return current != a.current;
            }
            bool operator==(const const_iterator& a) const
            {
                return current == a.current;
            }
            const_iterator& operator*()
            {
                return *this;
            }
        };

        iterator begin()
        {
            return iterator{*this, keys.first, keys.first + bucket_count()};
        }
        iterator end()
        {
            return iterator{*this, keys.first + bucket_count(), keys.first + bucket_count()};
        }
        const_iterator cbegin() const
        {
            return const_iterator{*this, keys.first, keys.first + bucket_count()};
        }
        const_iterator cend() const
        {
            return const_iterator{*this, keys.first + bucket_count(), keys.first + bucket_count()};
        }
        const_iterator begin() const
        {
            return cbegin();
        }
        const_iterator end() const
        {
            return cend();
        }

        HashedKeyMap(const allocator_type& allocator = allocator_type()) :
            keys(makeNoKey(), allocator)
        {
        }
        ~HashedKeyMap()
        {
            deallocate();
        }
        HashedKeyMap(HashedKeyMap&& a) :
            keys(std::move(a.keys)),
            vals(a.vals),
            mask(a.mask),
            used(a.used)
        {
            if (a.keys.first == a.noKey())
                keys.first = makeNoKey();
            a.vals = nullptr;
            a.keys.first = a.makeNoKey();
            a.used = a.mask = 0;
        }
        HashedKeyMap& operator=(HashedKeyMap&& a)
        {
            swap(a);
            return *this;
        }
        void swap(HashedKeyMap& a)
        {
            std::swap(vals, a.vals);
            std::swap(keys, a.keys);
            if (keys.first == a.noKey())
                keys.first = makeNoKey();
            if (a.keys.first == noKey())
                a.keys.first = a.makeNoKey();
            std::swap(mask, a.mask);
            std::swap(used, a.used);
        }

        template <typename Cb>
        void iterate(Cb cb)
        {
            val_t* v = vals;
            for (const key_t *i = keys.first, *e = i + bucket_count(); i != e; ++i, ++v)
                if (*i != EmptyKey)
                    cb(*i, *v);
        }
        template <typename Cb>
        void iterate(Cb cb) const
        {
            const val_t* v = vals;
            for (const key_t *i = keys.first, *e = i + bucket_count(); i != e; ++i, ++v)
                if (*i != EmptyKey)
                    cb(*i, *v);
        }

    protected:
        inline key_t* make_zero(size_t n)
        {
            key_t* p = oa_hashmap_util::allocate<key_t>(internalAllocator(), n);
            if (EmptyKey == 0)
                memset(p, 0, sizeof(key_t) * n);
            else
                std::fill_n(p, n, EmptyKey);
            return p;
        }
        void deallocate(key_t* n = nullptr, val_t* v = nullptr)
        {
            if (keys.first != noKey())
                oa_hashmap_util::deallocate(internalAllocator(), keys.first, bucket_count());
            if (vals)
                oa_hashmap_util::deallocate(internalAllocator(), vals, bucket_count());

            keys.first = n ? n : makeNoKey();
            vals = v;
        }
        uint32_t used = 0, mask = 0;  // this has to go one-after one
        std::pair<key_t*, allocator_type> keys;
        val_t* vals = nullptr;
        allocator_type& internalAllocator()
        {
            return keys.second;
        }
        key_t emptyKey = EmptyKey;
        key_t* noKey()
        {
            return &emptyKey;
        }
        key_t* makeNoKey()
        {
            return noKey();
        }
        void allocate(uint32_t new_capacity, key_t*& k, val_t*& v)
        {
            auto releaseKeys = [this, new_capacity](key_t* pointer)
            {
                oa_hashmap_util::deallocate(internalAllocator(), pointer, new_capacity);
            };
            std::unique_ptr<key_t, decltype(releaseKeys)> pendingKeys(make_zero(new_capacity), releaseKeys);
            v = oa_hashmap_util::allocate<val_t>(internalAllocator(), new_capacity);
            k = pendingKeys.release();
        }
        void rehash(uint32_t new_capacity)
        {
            key_t* newKeys;
            val_t* newVals;
            allocate(new_capacity, newKeys, newVals);
            const uint32_t new_mask = new_capacity - 1;
            if (used)
            {
                auto* tv = vals;
                for (auto *tp = keys.first, *e = keys.first + bucket_count(); tp != e; ++tp, ++tv)
                    if (*tp != EmptyKey)
                        newVals[oa_hashmap_util::emplace_new<key_t, EmptyKey, Hasher>(*tp, newKeys, new_mask)] = *tv;
            }
            deallocate(newKeys, newVals);
            mask = new_mask;
        }

    public:
        HashedKeyMap(const HashedKeyMap& a) :
            HashedKeyMap()
        {
            *this = a;
        }
        HashedKeyMap& operator=(const HashedKeyMap& a)
        {
            if (this == &a)
                return *this;
            if (a.used == 0)  // empty
            {
                deallocate();
                used = mask = 0;
                return *this;
            }
            if (a.bucket_count() != bucket_count())  // otherwise memory is there
            {
                key_t* newKeys;
                val_t* newVals;
                allocate(a.bucket_count(), newKeys, newVals);
                deallocate(newKeys, newVals);
            }

            mask = a.mask;
            used = a.used;
            memcpy(keys.first, a.keys.first, sizeof(key_t) * bucket_count());
            memcpy(vals, a.vals, sizeof(val_t) * bucket_count());
            return *this;
        }
    };

    template <class Key, Key EmptyKey = Key(), class Hasher = oa_hashmap_util::NoHash<Key>, class allocator_type = std::allocator<std::byte>, int load_factor_nom = 3, int load_factor_denom = 4>
    struct HashedKeySet
    {
        static_assert(std::is_trivially_copyable_v<Key>, "Nau hashed sets require trivially copyable keys");
        typedef Key key_t;
        typedef HashedKeySet<Key, EmptyKey, Hasher, allocator_type, load_factor_nom, load_factor_denom> this_type_t;
        bool insert(key_t key)  // that's safe. It can be done more optimal, though, as we do same work twice
        {
            NAU_FAST_ASSERT(key != EmptyKey);
            if (used >= (mask * load_factor_nom) / load_factor_denom) [[unlikely]]
                rehash(oa_hashmap_util::max(4, mask + mask + 2));
            if (!oa_hashmap_util::emplace_if_missing<key_t, EmptyKey, Hasher>(key, keys.first, mask))
                return false;
            ++used;
            return true;
        }
        void emplace_new(key_t key)  // that's unsafe emplace, it requires that key hasn't been added before
        {
            NAU_FAST_ASSERT(key != EmptyKey);
#if DAGOR_DBGLEVEL > 1  // ensure constraints
            NAU_ASSERT(!has_key(key));
#endif
            if (used >= (mask * load_factor_nom) / load_factor_denom) [[unlikely]]
                rehash(oa_hashmap_util::max(4, mask + mask + 2));
            oa_hashmap_util::emplace_new<key_t, EmptyKey, Hasher>(key, keys.first, mask);
            used++;
        }

        void erase(key_t key)
        {
            const key_t* k = oa_hashmap_util::find_key<key_t, EmptyKey, Hasher>(key, keys.first, mask, [&](uint32_t)
            {
                return true;
            });
            if (k != nullptr)
            {
                const size_t ind = k - keys.first;
                keys.first[ind] = EmptyKey;
                --used;
                if (keys.first[(ind + 1) & mask] != EmptyKey)
                    rehash(get_capacity(used));
            }
        }
        bool has_key(key_t key) const
        {
            return oa_hashmap_util::find_key<key_t, EmptyKey, Hasher>(key, keys.first, mask, [&](uint32_t)
            {
                return true;
            }) != nullptr;
        }

        void shrink_to_fit()
        {
            const uint32_t new_capacity = get_capacity(used);
            if (new_capacity < bucket_count())
                rehash(new_capacity);
        }
        void forced_clear()  // after forced clear can't be used for find until reserve
        {
            used = 0;
            deallocate();
            mask = 0;
        }
        void clear(uint32_t shrink_sz = 0)
        {
            const uint32_t new_capacity = get_capacity(shrink_sz);
            if (shrink_sz && bucket_count() != new_capacity)
            {
                used = 0;
                rehash(new_capacity);
            }
            else
            {
                if (used)
                {
                    if (EmptyKey == 0)
                        memset(keys.first, 0, bucket_count() * sizeof(key_t));
                    else
                        std::fill_n(keys.first, bucket_count(), EmptyKey);
                }
                used = 0;
            }
        }
        static inline size_t get_capacity(size_t sz)
        {
            return oa_hashmap_util::max(4, nau::math::get_bigger_pow2(sz * load_factor_denom / load_factor_nom));
        }
        uint32_t bucket_count() const
        {
            return mask + 1;
        }
        uint32_t size() const
        {
            return used;
        }
        bool empty() const
        {
            return size() == 0;
        }
        void reserve(uint32_t sz)
        {
            uint32_t newBuckets = get_capacity(sz);
            if (newBuckets > bucket_count())
                rehash(newBuckets);
        }
        struct iterator
        {
            this_type_t& map;
            key_t *current, *end;
            iterator(this_type_t& m, key_t* c, key_t* e) :
                map(m),
                current(c),
                end(e)
            {
                advance();
            }
            key_t& key()
            {
                return *current;
            }
            const key_t& key() const
            {
                return *current;
            }
            iterator& operator++()
            {
                ++current;
                advance();
                return *this;
            }
            inline void advance()
            {
                for (; current != end && *current == EmptyKey; ++current)
                    ;
            }
            bool operator!=(const iterator& a) const
            {
                return current != a.current;
            }
            bool operator==(const iterator& a) const
            {
                return current == a.current;
            }
            key_t& operator*()
            {
                return key();
            }
            const key_t& operator*() const
            {
                return key();
            }
        };
        struct const_iterator
        {
            const this_type_t& map;
            const key_t *current, *end;
            const key_t& key() const
            {
                return *current;
            }
            const_iterator(const this_type_t& m, const key_t* c, const key_t* e) :
                map(m),
                current(c),
                end(e)
            {
                advance();
            }
            const_iterator& operator++()
            {
                ++current;
                advance();
                return *this;
            }
            inline void advance()
            {
                for (; current != end && *current == EmptyKey; ++current)
                    ;
            }
            bool operator!=(const const_iterator& a) const
            {
                return current != a.current;
            }
            bool operator==(const const_iterator& a) const
            {
                return current == a.current;
            }
            const key_t& operator*()
            {
                return key();
            }
        };

        iterator begin()
        {
            return iterator{*this, keys.first, keys.first + bucket_count()};
        }
        iterator end()
        {
            return iterator{*this, keys.first + bucket_count(), keys.first + bucket_count()};
        }
        const_iterator cbegin() const
        {
            return const_iterator{*this, keys.first, keys.first + bucket_count()};
        }
        const_iterator cend() const
        {
            return const_iterator{*this, keys.first + bucket_count(), keys.first + bucket_count()};
        }
        const_iterator begin() const
        {
            return cbegin();
        }
        const_iterator end() const
        {
            return cend();
        }

        HashedKeySet(uint32_t sz = 1, const allocator_type& allocator = allocator_type()) :
            keys(nullptr, allocator)
        {
            if (sz)
            {
                sz = get_capacity(sz);
                keys.first = make_zero(sz);
                mask = sz - 1;
            }
            else
                keys.first = makeNoKey();
        }
        ~HashedKeySet()
        {
            deallocate();
        }
        HashedKeySet(HashedKeySet&& a) :
            keys(std::move(a.keys)),
            mask(a.mask),
            used(a.used)
        {
            if (a.keys.first == a.noKey())
                keys.first = makeNoKey();
            a.keys.first = a.makeNoKey();
            a.used = a.mask = 0;
        }
        HashedKeySet& operator=(HashedKeySet&& a)
        {
            swap(a);
            return *this;
        }
        void swap(HashedKeySet& a)
        {
            std::swap(keys, a.keys);
            if (keys.first == a.noKey())
                keys.first = makeNoKey();
            if (a.keys.first == noKey())
                a.keys.first = a.makeNoKey();
            std::swap(mask, a.mask);
            std::swap(used, a.used);
        }
        template <typename Cb>
        void iterate(Cb cb) const
        {
            for (const key_t *i = keys.first, *e = i + bucket_count(); i != e; ++i)
                if (*i != EmptyKey)
                    cb(*i);
        }

    protected:
        inline key_t* make_zero(size_t n)
        {
            key_t* p = oa_hashmap_util::allocate<key_t>(internalAllocator(), n);
            if (EmptyKey == 0)
                memset(p, 0, sizeof(key_t) * n);
            else
                std::fill_n(p, n, EmptyKey);
            return p;
        }
        void deallocate(key_t* n = nullptr)
        {
            if (keys.first != noKey())
                oa_hashmap_util::deallocate(internalAllocator(), keys.first, bucket_count());

            keys.first = n ? n : makeNoKey();
        }
        uint32_t used = 0, mask = 0;
        key_t emptyKey = EmptyKey;
        key_t* noKey()
        {
            return &emptyKey;
        }
        key_t* makeNoKey()
        {
            return noKey();
        }
        std::pair<key_t*, allocator_type> keys;
        allocator_type& internalAllocator()
        {
            return keys.second;
        }
        void rehash(uint32_t new_capacity)
        {
            key_t* newKeys = make_zero(new_capacity);
            const uint32_t new_mask = new_capacity - 1;
            if (used)
            {
                for (auto *tp = keys.first, *e = keys.first + bucket_count(); tp != e; ++tp)
                    if (*tp != EmptyKey)
                        oa_hashmap_util::emplace_new<key_t, EmptyKey, Hasher>(*tp, newKeys, new_mask);
            }
            deallocate(newKeys);
            mask = new_mask;
        }
    };

    template <class Key, class Value, Key EmptyKey = Key(), class Hasher = oa_hashmap_util::NoHash<Key>>
    struct FixedCapacityHashedKeyMap
    {
        static_assert(std::is_trivially_copyable_v<Key> && std::is_trivially_copyable_v<Value>, "Nau hashed maps require trivially copyable keys and values");
        typedef Key key_t;
        typedef Value val_t;
        typedef FixedCapacityHashedKeyMap<Key, Value, EmptyKey> this_type_t;
        uint32_t mask, used;
        key_t* keys;
        val_t* vals;
        bool emplace(key_t key, const val_t& input)  // that's unsafe emplace, it requires that key hasn't been added before
        {
            const val_t val = input;
            NAU_FAST_ASSERT(key != EmptyKey);
            if (used >= mask)  // not enough space
                return false;
            vals[oa_hashmap_util::emplace_new<key_t, EmptyKey, Hasher>(key, keys, mask)] = val;
            used++;
            return true;
        }
        void emplace_new(key_t key, const val_t& input)  // that's unsafe emplace, it requires that key hasn't been added before
        {
            const val_t val = input;
#if DAGOR_DBGLEVEL > 1  // ensure constraints
            NAU_ASSERT(!has_key(key));
#endif
            emplace(key, val);
        }
        bool emplace_if_missing(key_t key, const val_t& input)
        {
            const val_t val = input;
            NAU_FAST_ASSERT(key != EmptyKey);
            if (used >= mask)  // not enough space
                return false;
            bool emplacedNew = false;
            vals[oa_hashmap_util::emplace<key_t, EmptyKey, Hasher>(key, keys, mask, emplacedNew)] = val;
            used += uint32_t(emplacedNew);
            return emplacedNew;
        }

        template <class ValCB = oa_hashmap_util::DefaultValCB<val_t>>
        const val_t findOr(key_t key, const val_t& def, ValCB cb = ValCB()) const
        {
            return oa_hashmap_util::find_or<key_t, EmptyKey, Hasher>(key, keys, vals, def, mask, [&](const auto& v)
            {
                return cb(v);
            });
        }

        template <class ValCB = oa_hashmap_util::DefaultValCB<val_t>>
        val_t* findVal(key_t key, ValCB cb = ValCB())
        {
            return oa_hashmap_util::find_val<key_t, EmptyKey, Hasher>(key, keys, vals, mask, [&](const auto& v)
            {
                return cb(v);
            });
        }
        template <class ValCB = oa_hashmap_util::DefaultValCB<val_t>>
        const val_t* find_val(key_t key, ValCB cb = ValCB()) const
        {
            return const_cast<this_type_t*>(this)->find_val(key, cb);
        }

        template <class ValCB = oa_hashmap_util::DefaultValCB<val_t>>
        bool has_key(key_t key, ValCB cb = ValCB()) const
        {
            return oa_hashmap_util::find_key<key_t, EmptyKey, Hasher>(key, keys, mask, [&](uint32_t i)
            {
                return cb(vals[i]);
            }) != nullptr;
        }

        void clear()
        {
            if (used)
            {
                if (EmptyKey == 0)
                    memset(keys, 0, bucket_count() * sizeof(key_t));
                else
                    std::fill_n(keys, bucket_count(), EmptyKey);
            }
            used = 0;
        }
        uint32_t bucket_count() const
        {
            return mask + 1;
        }
        uint32_t size() const
        {
            return used;
        }
        bool empty() const
        {
            return size() == 0;
        }
    };

}  // namespace dag
