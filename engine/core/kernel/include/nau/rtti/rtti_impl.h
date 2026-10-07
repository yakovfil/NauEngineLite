// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once
#include <concepts>
#include <cstddef>
#include <thread>
#include <type_traits>

#include "nau/diag/assertion.h"
#include "nau/memory/mem_allocator.h"
#include "nau/rtti/ptr.h"
#include "nau/rtti/rtti_object.h"
#include "nau/rtti/rtti_utils.h"
#include "nau/utils/tuple_utility.h"

#if NAU_DEBUG && defined(NAU_ASSERT_ENABLED)
    #define NAU_RTTI_VALIDATE_SHARED_STATE
#endif

namespace nau
{
    /**
     */
    template <typename T>
    concept RefCountedConcept = requires(T rc) {
        { rc.addRef() } -> std::same_as<uint32_t>;
        { rc.removeRef() } -> std::same_as<uint32_t>;
        { rc.noRefs() } -> std::same_as<bool>;
    };

    /**
     */
    class ConcurrentRC
    {
    public:
        uint32_t addRef()
        {
            const auto prev = m_counter.fetch_add(1);
            NAU_ASSERT(prev > 0);

            return prev;
        }

        uint32_t removeRef()
        {
            const auto prev = m_counter.fetch_sub(1);
            NAU_ASSERT(prev > 0);

            return prev;
        }

        bool noRefs() const
        {
            return m_counter.load() == 0;
        }

    private:
        bool tryAddRef()
        {
            uint32_t counter = m_counter.load();

            do
            {
                if (counter == 0)
                {
                    return false;
                }
            } while (!m_counter.compare_exchange_weak(counter, counter + 1));

            return true;
        }

        std::atomic_uint32_t m_counter{1};

        friend inline uint32_t refsCount(const ConcurrentRC& rc_)
        {
            return rc_.m_counter.load();
        }

        friend bool tryAddRef(ConcurrentRC& refCounted);
    };

    /**
     */
    class StrictSingleThreadRC
    {
    public:
        uint32_t addRef()
        {
            NAU_ASSERT(m_ownerThreadId == std::this_thread::get_id());
            NAU_ASSERT(m_counter > 0);

            return m_counter++;
        }

        uint32_t removeRef()
        {
            NAU_ASSERT(m_ownerThreadId == std::this_thread::get_id());
            NAU_ASSERT(m_counter > 0);

            return m_counter--;
        }

        bool noRefs() const
        {
            NAU_ASSERT(m_ownerThreadId == std::this_thread::get_id());
            return m_counter == 0;
        }

    private:
        uint32_t m_counter = 1;
        const std::thread::id m_ownerThreadId = std::this_thread::get_id();

        friend inline uint32_t refsCount(const StrictSingleThreadRC& rc_)
        {
            NAU_ASSERT(rc_.m_ownerThreadId == std::this_thread::get_id());
            return rc_.m_counter;
        }
    };

    template <RefCountedConcept RC>
    inline bool tryAddRef(RC& refCounted)
    {
        if (refCounted.noRefs())
        {
            return false;
        }

        refCounted.addRef();
        return true;
    }

    inline bool tryAddRef(ConcurrentRC& refCounted)
    {
        return refCounted.tryAddRef();
    }

    template <typename T>
    concept RefCountedClassWithImplTag = requires {
        typename T::RcClassImplTag;
    } && std::is_same_v<typename T::RcClassImplTag::Type, T>;

}  // namespace nau

namespace nau::rtti_detail
{
    template <typename T>
    struct NauRcClassImplTag
    {
        using Type = T;
    };

#ifdef NAU_RTTI_VALIDATE_SHARED_STATE
    // "Class Marker" is used to validate memory where shared state expected to be allocated.
    // This is need to be ensure that ref counted class created by any rtti factory function
    // with is used special memory layout to place [shared state][instance].
    // Without that the default implementation (with NAU_CLASS) of the ref counted class will not be operable.
    // 6004214524017983822 == ['N', 'A', 'U', 'C', 'L', 'A', 'S', 'S']
    inline constexpr uint64_t NauClassMarkerValue = 6004214524017983822;
#endif

    template <RefCountedConcept>
    class RttiClassSharedState;

    template <RefCountedConcept RC>
    class RttiClassSharedState final : public nau::IWeakRef
    {
    public:
        using AcquireFunc = IRefCounted* (*)(void*);
        using DestructorFunc = void (*)(void*);

        RttiClassSharedState(const RttiClassSharedState&) = delete;

        RttiClassSharedState(IMemAllocator::Ptr allocator_, AcquireFunc, DestructorFunc, std::byte* ptr);

        void addInstanceRef();

        void releaseInstanceRef();

        uint32_t getInstanceRefsCount() const;

        IWeakRef* acquireWeakRef();

        IMemAllocator::Ptr getAllocator() const;

    private:
        void releaseStorageRef();

        void addWeakRef() override;

        void releaseRef() override;

        IRefCounted* acquire() override;

        bool isDead() const override;

#ifdef NAU_RTTI_VALIDATE_SHARED_STATE
    public:
        const uint64_t m_classMarker = NauClassMarkerValue;

    private:
#endif
        IMemAllocator::Ptr m_allocator;
        AcquireFunc m_acquireFunc;
        DestructorFunc m_destructorFunc;
        std::byte* const m_allocatedPtr;
        RC m_stateCounter;
        RC m_instanceCounter;
    };

    /**

    */
    template <RefCountedConcept RC>
    struct RttiClassStorage
    {
        using SharedState = RttiClassSharedState<RC>;

        static constexpr size_t BlockAlignment = alignof(std::max_align_t);
        static constexpr size_t SharedStateSize = sizeof(AlignedStorage<sizeof(SharedState), BlockAlignment>);

        template <typename T>
        using ValueStorage = AlignedStorage<sizeof(T), std::max(BlockAlignment, alignof(T))>;

        // additional alignof(T) = extra space for cases when alignment fixed by offset
        template <typename T>
        static constexpr size_t InstanceStorageSize = SharedStateSize + sizeof(ValueStorage<T>) + alignof(T);

        template <typename T>
        using InstanceInplaceStorage = AlignedStorage<InstanceStorageSize<T>, std::max(alignof(T), alignof(SharedState))>;

        template <typename T>
        static SharedState& getSharedState(const T& instance)
        {
            const std::byte* const instancePtr = reinterpret_cast<const std::byte*>(&instance);
            std::byte* const statePtr = const_cast<std::byte*>(instancePtr - SharedStateSize);

            auto& sharedState = *reinterpret_cast<SharedState*>(statePtr);

#ifdef NAU_RTTI_VALIDATE_SHARED_STATE
            // check that class instance has valid shared state with expected layout.
            NAU_FATAL(sharedState.m_classMarker == NauClassMarkerValue, "Invalid SharedState. RefCounted class must be created only with rtti instance factory functions");
#endif
            return sharedState;
        }

        static void* getInstancePtr(SharedState& state)
        {
            std::byte* const statePtr = reinterpret_cast<std::byte*>(&state);
            return statePtr + SharedStateSize;
        }

        template <typename T>
        static T& getInstance(SharedState& state)
        {
            std::byte* const statePtr = const_cast<std::byte*>(&state);
            std::byte* const instancePtr = statePtr + SharedStateSize;

            return *reinterpret_cast<T*>(instancePtr);
        }

        template <typename T, typename... Args>
        static T* instanceFactory(void* inplaceMemBlock, IMemAllocator::Ptr allocator, Args&&... args)
        {
            static_assert(RefCountedClassWithImplTag<T>, "Class expected to be implemented with NAU_CLASS/NAU_CLASS_/NAU_IMPLEMENT_REFCOUNTED. Please, check Class declaration");
            static_assert((alignof(T) <= alignof(SharedState)) || (alignof(T) % alignof(SharedState) == 0), "Unsupported type alignment.");
            NAU_ASSERT(static_cast<bool>(inplaceMemBlock) != static_cast<bool>(allocator));

            const auto acquire = [](void* instancePtr) -> IRefCounted*
            {
                T* const instance = reinterpret_cast<T*>(instancePtr);
                auto const refCounted = rtti::staticCast<IRefCounted*>(instance);
                NAU_ASSERT(refCounted, "Runtime cast ({}) -> IRefCounted failed", rtti::getTypeInfo<T>().getTypeName());

                return refCounted;
            };

            const auto destructor = [](void* instancePtr)
            {
                T* instance = reinterpret_cast<T*>(instancePtr);
                std::destroy_at(instance);
            };

            constexpr size_t StorageSize = InstanceStorageSize<T>;

            // Allocator or preallocated memory
            std::byte* const storage = reinterpret_cast<std::byte*>(allocator ? allocator->allocate(StorageSize) : inplaceMemBlock);
            NAU_FATAL(storage);
            NAU_FATAL(reinterpret_cast<uintptr_t>(storage) % alignof(SharedState) == 0);

            std::byte* statePtr = storage;
            std::byte* instancePtr = statePtr + SharedStateSize;

            // need to respect type's alignment:
            // if storage is not properly aligned there is need to offset state and instance pointers:
            if (const uintptr_t alignmentOffset = reinterpret_cast<uintptr_t>(instancePtr) % alignof(T); alignmentOffset > 0)
            {
                const size_t offsetGap = alignof(T) - alignmentOffset;
                statePtr = statePtr + offsetGap;
                instancePtr = instancePtr + offsetGap;

                NAU_FATAL(reinterpret_cast<uintptr_t>(statePtr) % alignof(SharedState) == 0);
                NAU_FATAL(reinterpret_cast<uintptr_t>(instancePtr) % alignof(T) == 0);
                NAU_FATAL(StorageSize >= SharedStateSize + sizeof(T) + offsetGap);
            }

            NAU_FATAL(reinterpret_cast<uintptr_t>(instancePtr) % alignof(T) == 0, "Invalid address, expected alignment ({})", alignof(T));
#if NAU_DEBUG
            memset(storage, 0, StorageSize);
#endif
            [[maybe_unused]]
            auto sharedState = new(statePtr) SharedState(std::move(allocator), acquire, destructor, storage);  // -V799

            return new(instancePtr) T(std::forward<Args>(args)...);
        }

        template <typename T, typename MemBlock, typename... Args>
        static T* createInstanceInplace(MemBlock& memBlock, Args&&... args)
        {
            static_assert(sizeof(MemBlock) >= InstanceStorageSize<T>);

            void* const ptr = reinterpret_cast<void*>(&memBlock);
            return instanceFactory<T>(ptr, IMemAllocator::Ptr{}, std::forward<Args>(args)...);
        }

        template <typename T, typename... Args>
        static T* createInstanceWithAllocator(IMemAllocator::Ptr allocator, Args&&... args)
        {
            return instanceFactory<T>(nullptr, allocator ? std::move(allocator) : nau::getDefaultAllocator(), std::forward<Args>(args)...);
        }

        template <typename T, typename... Args>
        static T* createInstance(Args&&... args)
        {
            return instanceFactory<T>(nullptr, nau::getDefaultAllocator(), std::forward<Args>(args)...);
        }
    };

    template <RefCountedConcept RC>
    RttiClassSharedState<RC>::RttiClassSharedState(IMemAllocator::Ptr allocator, AcquireFunc acquire, DestructorFunc destructor, std::byte* allocatedPtr) :
        m_allocator(std::move(allocator)),
        m_acquireFunc(acquire),
        m_destructorFunc(destructor),
        m_allocatedPtr(allocatedPtr)
    {
        NAU_ASSERT(m_acquireFunc);
        NAU_ASSERT(m_destructorFunc);
        NAU_ASSERT(refsCount(m_stateCounter) == 1);
        NAU_ASSERT(refsCount(m_instanceCounter) == 1);
    }

    template <RefCountedConcept RC>
    IMemAllocator::Ptr RttiClassSharedState<RC>::getAllocator() const
    {
        return m_allocator;
    }

    template <RefCountedConcept RC>
    void RttiClassSharedState<RC>::addInstanceRef()
    {
        m_instanceCounter.addRef();
        m_stateCounter.addRef();
    }

    template <RefCountedConcept RC>
    void RttiClassSharedState<RC>::releaseInstanceRef()
    {
        if (m_instanceCounter.removeRef() == 1)
        {
            void* const instancePtr = RttiClassStorage<RC>::getInstancePtr(*this);
            m_destructorFunc(instancePtr);
        }

        releaseStorageRef();
    }

    template <RefCountedConcept RC>
    uint32_t RttiClassSharedState<RC>::getInstanceRefsCount() const
    {
        return refsCount(m_instanceCounter);
    }

    template <RefCountedConcept RC>
    IWeakRef* RttiClassSharedState<RC>::acquireWeakRef()
    {
        m_stateCounter.addRef();
        return this;
    }

    template <RefCountedConcept RC>
    void RttiClassSharedState<RC>::releaseStorageRef()
    {
        if (m_stateCounter.removeRef() == 1)
        {
            NAU_ASSERT(m_stateCounter.noRefs());
            NAU_ASSERT(m_instanceCounter.noRefs());

            auto allocator = std::move(m_allocator);

            std::destroy_at(this);
            if (allocator)
            {
                allocator->deallocate(m_allocatedPtr);
            }
        }
    }

    template <RefCountedConcept RC>
    void RttiClassSharedState<RC>::addWeakRef()
    {
        m_stateCounter.addRef();
    }

    template <RefCountedConcept RC>
    void RttiClassSharedState<RC>::releaseRef()
    {
        releaseStorageRef();
    }

    template <RefCountedConcept RC>
    IRefCounted* RttiClassSharedState<RC>::acquire()
    {
        if (tryAddRef(m_instanceCounter))
        {
            m_stateCounter.addRef();
            void* const instancePtr = RttiClassStorage<RC>::getInstancePtr(*this);
            IRefCounted* const instance = m_acquireFunc(instancePtr);
            return instance;
        }

        return nullptr;
    }

    template <RefCountedConcept RC>
    bool RttiClassSharedState<RC>::isDead() const
    {
        NAU_ASSERT(!m_stateCounter.noRefs());

        return m_instanceCounter.noRefs();
    }

}  // namespace nau::rtti_detail

namespace nau::rtti
{
    template <RefCountedConcept RCPolicy>
    struct RttiClassPolicy
    {
        using RC = RCPolicy;
    };

    struct RCPolicy
    {
        using Concurrent = ::nau::ConcurrentRC;
        using StrictSingleThread = ::nau::StrictSingleThreadRC;
    };

    template <typename T>
    inline constexpr size_t InstanceStorageSize = rtti_detail::template RttiClassStorage<typename T::ClassPolicy::RC>::template InstanceStorageSize<T>;

    template <typename T>
    using InstanceInplaceStorage = typename rtti_detail::RttiClassStorage<typename T::ClassPolicy::RC>::template InstanceInplaceStorage<T>;

    // template< typename T>
    // Allocator::Ptr GetClassInstanceAllocator(const T& instance) {
    //	static_assert(!std::is_abstract_v<T>, "GetInstanceAllocator must be used only with RttiClass Implementations");
    //
    //	auto& state = ::nau::rtti_detail::RttiClassStorage<typename T::ClassPolicy::RC>::template GetSharedState<T>(instance);
    //	return state.GetAllocator();
    // }

    template <typename ClassImpl, typename Itf = ClassImpl, typename MemBlock, typename... Args>
    Ptr<Itf> createInstanceInplace(MemBlock& memBlock, Args&&... args)
    {
        using namespace ::nau::rtti_detail;

        ClassImpl* const instance = RttiClassStorage<typename ClassImpl::ClassPolicy::RC>::template createInstanceInplace<ClassImpl>(memBlock, std::forward<Args>(args)...);
        auto const itf = rtti::staticCast<Itf*>(instance);
        NAU_ASSERT(itf);
        return rtti::TakeOwnership(itf);
    }

    template <typename ClassImpl, typename Itf = ClassImpl, typename... Args>
    Ptr<Itf> createInstanceSingleton(Args&&... args)
    {
        static rtti::InstanceInplaceStorage<ClassImpl> storage;
        return rtti::createInstanceInplace<ClassImpl, Itf>(storage, std::forward<Args>(args)...);
    }

    template <typename ClassImpl, typename Interface_ = ClassImpl, typename... Args>
    // requires ComInterface<Interface_>
    Ptr<Interface_> createInstanceWithAllocator(IMemAllocator::Ptr allocator, Args&&... args)
    {
        using namespace ::nau::rtti_detail;

        ClassImpl* const instance = RttiClassStorage<typename ClassImpl::ClassPolicy::RC>::template createInstanceWithAllocator<ClassImpl>(std::move(allocator), std::forward<Args>(args)...);
        auto const itf = rtti::staticCast<Interface_*>(instance);
        NAU_ASSERT(itf);
        return rtti::TakeOwnership(itf);
    }

    template <typename ClassImpl, typename Interface_ = ClassImpl, typename... Args>
    // requires ComInterface<Interface_>
    Ptr<Interface_> createInstance(Args&&... args)
    {
        using namespace ::nau::rtti_detail;

        ClassImpl* const instance = RttiClassStorage<typename ClassImpl::ClassPolicy::RC>::template createInstance<ClassImpl>(std::forward<Args>(args)...);
        Interface_* const itf = rtti::staticCast<Interface_*>(instance);
        NAU_ASSERT(itf);
        return rtti::TakeOwnership(itf);
    }

}  // namespace nau::rtti

#define NAU_IMPLEMENT_RTTI_OBJECT                                             \
                                                                              \
public:                                                                       \
    using ::nau::IRttiObject::is;                                             \
    using ::nau::IRttiObject::as;                                             \
                                                                              \
    bool is(const ::nau::rtti::TypeInfo& type) const noexcept override        \
    {                                                                         \
        return ::nau::rtti::runtimeIs<decltype(*this)>(type);                 \
    }                                                                         \
                                                                              \
    void* as(const ::nau::rtti::TypeInfo& type) noexcept override             \
    {                                                                         \
        return ::nau::rtti::runtimeCast(*this, type);                         \
    }                                                                         \
                                                                              \
    const void* as(const ::nau::rtti::TypeInfo& type) const noexcept override \
    {                                                                         \
        return ::nau::rtti::runtimeCast(*this, type);                         \
    }

#define NAU_IMPLEMENT_REFCOUNTED(ClassImpl, RcPolicy)                               \
public:                                                                             \
    using ClassPolicy = ::nau::rtti::RttiClassPolicy<RcPolicy>;                     \
    using RttiClassStorage = ::nau::rtti_detail::RttiClassStorage<ClassPolicy::RC>; \
    using RcClassImplTag = ::nau::rtti_detail::NauRcClassImplTag<ClassImpl>;        \
                                                                                    \
    void addRef() override                                                          \
    {                                                                               \
        RttiClassStorage::getSharedState(*this).addInstanceRef();                   \
    }                                                                               \
                                                                                    \
    void releaseRef() override                                                      \
    {                                                                               \
        RttiClassStorage::getSharedState(*this).releaseInstanceRef();               \
    }                                                                               \
                                                                                    \
    ::nau::IWeakRef* getWeakRef() override                                          \
    {                                                                               \
        return RttiClassStorage::getSharedState(*this).acquireWeakRef();            \
    }                                                                               \
                                                                                    \
    uint32_t getRefsCount() const override                                          \
    {                                                                               \
        return RttiClassStorage::getSharedState(*this).getInstanceRefsCount();      \
    }                                                                               \
                                                                                    \
    const ::nau::IMemAllocator::Ptr getRttiClassInstanceAllocator() const           \
    {                                                                               \
        return RttiClassStorage::getSharedState(*this).getAllocator();              \
    }

#define NAU_RTTI_CLASS(ClassImpl, ...) \
    NAU_TYPEID(ClassImpl)              \
    NAU_CLASS_BASE(__VA_ARGS__)        \
                                       \
    NAU_IMPLEMENT_RTTI_OBJECT

#define NAU_CLASS(ClassImpl, RcPolicy, ...) \
    NAU_RTTI_CLASS(ClassImpl, __VA_ARGS__)  \
    NAU_IMPLEMENT_REFCOUNTED(ClassImpl, RcPolicy)

#define NAU_CLASS_(ClassImpl, ...) NAU_CLASS(ClassImpl, ::nau::rtti::RCPolicy::Concurrent, __VA_ARGS__)

#define NAU_CLASS_ALLOW_PRIVATE_CONSTRUCTOR \
    template <nau::RefCountedConcept>       \
    friend struct nau::rtti_detail::RttiClassStorage;

#ifdef NAU_RTTI_VALIDATE_SHARED_STATE
    #undef NAU_RTTI_VALIDATE_SHARED_STATE
#endif
