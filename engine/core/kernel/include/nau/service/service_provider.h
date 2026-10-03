// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once

#include <array>
#include <memory>
#include <mutex>
#include <span>
#include <type_traits>
#include <vector>

#include "nau/diag/assertion.h"
#include "nau/dispatch/class_descriptor.h"
#include "nau/dispatch/class_descriptor_builder.h"
#include "nau/kernel/kernel_config.h"
#include "nau/memory/singleton_memop.h"
#include "nau/rtti/ptr.h"
#include "nau/rtti/rtti_impl.h"
#include "nau/rtti/rtti_object.h"
#include "nau/rtti/rtti_utils.h"
#include "nau/rtti/type_info.h"
#include "nau/runtime/async_disposable.h"
#include "nau/threading/lock_guard.h"
#include "nau/utils/cancellation.h"
#include "nau/utils/functor.h"
#include "nau/utils/scope_guard.h"
#include "nau/utils/type_list/append.h"
#include "nau/utils/type_utility.h"

namespace nau::core_detail
{
    template <typename T>
    struct ServiceAccessorHelper
    {
        using ProvidedApi = type_list::AppendHead<meta::ClassAllUniqueBase<T>, T>;

        static bool hasApi(const rtti::TypeInfo& t)
        {
            return hasApiHelperInternal(t, ProvidedApi{});
        }

        static void* getApi(T& instance, const rtti::TypeInfo& t)
        {
            return getApiHelperInternal(instance, t, ProvidedApi{});
        }

        template <typename U>
        static bool tryStaticCast(T& instance, const rtti::TypeInfo& targetType, void** outPtr)
        {
            if (rtti::getTypeInfo<U>() != targetType)
            {
                return false;
            }

            if constexpr (std::is_same_v<T, U> || std::is_assignable_v<U&, T&>)
            {
                *outPtr = &static_cast<U&>(instance);
                return true;
            }
            else
            {
                return false;
            }
        }

    private:
        template <typename... U>
        static bool hasApiHelperInternal(const rtti::TypeInfo& t, TypeList<U...>)
        {
            return ((rtti::getTypeInfo<U>() == t) || ...);
        }

        template <typename... U>
        static void* getApiHelperInternal(T& instance, const rtti::TypeInfo& targetType, TypeList<U...>)
        {
            void* outPtr = nullptr;
            [[maybe_unused]]
            const bool success = (tryStaticCast<U>(instance, targetType, &outPtr) || ...);

            return outPtr;
        }
    };

}  // namespace nau::core_detail

namespace nau
{

    /**
     */
    struct NAU_ABSTRACT_TYPE ServiceAccessor
    {
        enum class GetApiMode
        {
            AllowLazyCreation,
            DoNotCreate
        };

        using Ptr = std::unique_ptr<ServiceAccessor>;

        virtual ~ServiceAccessor() = default;

        /**
         */
        virtual void* getApi(const rtti::TypeInfo&, GetApiMode = GetApiMode::AllowLazyCreation) = 0;

        virtual bool hasApi(const rtti::TypeInfo&) = 0;
    };

}  // namespace nau

namespace nau::core_detail
{
    template <typename T>
    constexpr inline bool IsKnownInstanceUniquePtr = IsTemplateOf<std::unique_ptr, T> || IsTemplateOf<std::unique_ptr, T>;

    template <typename T>
    constexpr inline bool IsKnownInstancePtr = IsKnownInstanceUniquePtr<T> || IsTemplateOf<nau::Ptr, T>;

    /**
     */
    template <rtti::WithTypeInfo T, typename SmartPtr>
    class NonRttiServiceAccessor final : public ServiceAccessor
    {
    public:
        NonRttiServiceAccessor(SmartPtr instance) :
            m_instance(std::move(instance))
        {
        }

        void* getApi(const rtti::TypeInfo& type, [[maybe_unused]] GetApiMode) override
        {
            return ServiceAccessorHelper<T>::getApi(*this->m_instance, type);
        }

        bool hasApi(const rtti::TypeInfo& type) override
        {
            return ServiceAccessorHelper<T>::hasApi(type);
        }

    private:
        const SmartPtr m_instance;
    };

    /**
     */
    template <template <typename, typename...> class SmartPtrT = std::unique_ptr>
    class RttiServiceAccessor final : public ServiceAccessor
    {
    public:
        template <typename T>
        RttiServiceAccessor(SmartPtrT<T> instance) :
            m_instance(rtti::pointer_cast<IRttiObject>(std::move(instance)))
        {
        }

        void* getApi(const rtti::TypeInfo& type, [[maybe_unused]] GetApiMode) override
        {
            NAU_FATAL(this->m_instance);
            return this->m_instance->as(type);
        }

        bool hasApi(const rtti::TypeInfo& type) override
        {
            NAU_FATAL(this->m_instance);
            return this->m_instance->is(type);
        }

    private:
        const SmartPtrT<IRttiObject> m_instance;
    };

    /**
     */
    class RefCountedLazyServiceAccessor final : public ServiceAccessor
    {
    public:
        template <rtti::ClassWithTypeInfo T>
        requires(std::is_base_of_v<IRefCounted, T>)
        RefCountedLazyServiceAccessor(TypeTag<T>) :
            m_classDescriptor(nau::getClassDescriptor<T>())
        {
            NAU_ASSERT(m_classDescriptor);
            NAU_ASSERT(m_classDescriptor->getConstructor() != nullptr);
        }

        inline void* getApi(const rtti::TypeInfo& type, GetApiMode getApiMode) override
        {
            if (!hasApi(type))
            {
                return nullptr;
            }

            {
                lock_(m_mutex);

                if (!m_instance)
                {
                    if (getApiMode == GetApiMode::DoNotCreate)
                    {
                        return nullptr;
                    }

                    const IMethodInfo* const ctor = m_classDescriptor->getConstructor();
                    Result<IRttiObject*> instance = ctor->invoke(nullptr, {});
                    NAU_FATAL(instance);

                    IRefCounted* const refCounted = (*instance)->as<IRefCounted*>();
                    NAU_FATAL(refCounted, "Only refcounted objects currently are supported");

                    m_instance = rtti::TakeOwnership{refCounted};
                }
            }

            NAU_FATAL(m_instance);
            return m_instance->as(type);
        }

        inline bool hasApi(const rtti::TypeInfo& type) override
        {
            return m_classDescriptor->findInterface(type) != nullptr;
        }

        IClassDescriptor::Ptr getClassDescriptor() const
        {
            return m_classDescriptor;
        }

    private:
        const IClassDescriptor::Ptr m_classDescriptor;
        std::mutex m_mutex;
        nau::Ptr<> m_instance;
    };
    /**
     */
    template <typename F>

    class FactoryLazyServiceAccessor final : public ServiceAccessor
    {
    public:
        using ServicePtr = std::invoke_result_t<F>;
        using T = typename ServicePtr::element_type;

        FactoryLazyServiceAccessor(F&& factory) :
            m_factory(std::move(factory)),
            m_classDescriptor(nau::getClassDescriptor<T>())

        {
            static_assert(IsKnownInstancePtr<ServicePtr>);
            static_assert(rtti::ClassWithTypeInfo<T>, "Expected non abstract type with known rtti");
        }

        void* getApi(const rtti::TypeInfo& type, GetApiMode getApiMode) override
        {
            if (!hasApi(type))
            {
                return nullptr;
            }

            {
                lock_(m_mutex);

                if (!m_instance)
                {
                    if (getApiMode == GetApiMode::DoNotCreate)
                    {
                        return nullptr;
                    }

                    m_instance = m_factory();
                }
            }

            NAU_FATAL(m_instance);
            if constexpr (std::is_base_of_v<IRttiObject, T>)
            {
                return m_instance->as(type);
            }
            else
            {
                return ServiceAccessorHelper<T>::getApi(*m_instance, type);
            }
        }

        bool hasApi(const rtti::TypeInfo& type) override
        {
            return m_classDescriptor->findInterface(type) != nullptr;
        }

        IClassDescriptor::Ptr getClassDescriptor() const
        {
            return m_classDescriptor;
        }

    private:
        F m_factory;
        const IClassDescriptor::Ptr m_classDescriptor;
        std::mutex m_mutex;
        ServicePtr m_instance;
    };

}  // namespace nau::core_detail

namespace nau
{
    /**
     */
    struct NAU_ABSTRACT_TYPE ServiceProvider : virtual IRttiObject
    {
        NAU_INTERFACE(ServiceProvider, IRttiObject)

        using Ptr = std::unique_ptr<ServiceProvider>;

        virtual ~ServiceProvider() = default;

        nau::Cancellation getCancellation();

        template <rtti::WithTypeInfo T>
        bool has();

        /**
            @brief
                return service reference.
                will assert if service does not registered
        */
        template <rtti::WithTypeInfo T>
        T& get();

        template <rtti::WithTypeInfo T>
        T* find();

        template <rtti::WithTypeInfo T>
        [[nodiscard]]
        std::vector<T*> getAll();

        template <rtti::WithTypeInfo T, typename Predicate>
        T* findIf(Predicate);

        /**
            @brief register existing service instance
        */
        template <rtti::WithTypeInfo T>
        void addService(std::unique_ptr<T>&&);

        template <rtti::WithTypeInfo T>
        void addService(nau::Ptr<T>);

        template <rtti::ClassWithTypeInfo T>
        void addService();

        template <typename F>
        void addServiceLazy(F factory);

        template <rtti::ClassWithTypeInfo T>
        void addClass();

        /**

        */
        template <rtti::WithTypeInfo T, rtti::WithTypeInfo... U>
        std::vector<IClassDescriptor::Ptr> findClasses(bool anyType = true);

        virtual void addClass(IClassDescriptor::Ptr&& classDesc) = 0;

        virtual std::vector<IClassDescriptor::Ptr> findClasses(const rtti::TypeInfo& type) = 0;

        virtual std::vector<IClassDescriptor::Ptr> findClasses(std::span<const rtti::TypeInfo*> types, bool anyType = true) = 0;

    protected:
        using GenericServiceFactory = Functor<std::unique_ptr<IRttiObject>()>;

        template <typename T, typename... U>
        static std::vector<const rtti::TypeInfo*> makeTypesInfo(TypeList<T, U...>)
        {
            return {&rtti::getTypeInfo<T>(), &rtti::getTypeInfo<U>()...};
        }

        virtual void* findInternal(const rtti::TypeInfo&) = 0;

        virtual void findAllInternal(const rtti::TypeInfo&, void (*)(void* instancePtr, void*), void*, ServiceAccessor::GetApiMode = ServiceAccessor::GetApiMode::AllowLazyCreation) = 0;

        virtual void addServiceAccessorInternal(ServiceAccessor::Ptr, IClassDescriptor::Ptr = nullptr) = 0;

        virtual bool hasApiInternal(const rtti::TypeInfo&) = 0;
    };

    template <rtti::WithTypeInfo T>
    bool ServiceProvider::has()
    {
        return hasApiInternal(rtti::getTypeInfo<T>());
    }

    template <rtti::WithTypeInfo T>
    T& ServiceProvider::get()
    {
        void* const service = findInternal(rtti::getTypeInfo<T>());
        NAU_ASSERT(service, "Service ({}) does not exists", rtti::getTypeInfo<T>().getTypeName());
        return *reinterpret_cast<T*>(service);
    }

    template <rtti::WithTypeInfo T>
    T* ServiceProvider::find()
    {
        void* const service = findInternal(rtti::getTypeInfo<T>());
        return service ? reinterpret_cast<T*>(service) : nullptr;
    }

    template <rtti::WithTypeInfo T>
    std::vector<T*> ServiceProvider::getAll()
    {
        std::vector<T*> services;
        findAllInternal(rtti::getTypeInfo<T>(), [](void* ptr, void* data)
        {
            T* const instance = reinterpret_cast<T*>(ptr);
            auto& container = *reinterpret_cast<decltype(services)*>(data);

            container.push_back(instance);
        }, &services);

        return services;
    }

    template <rtti::WithTypeInfo T, typename Predicate>
    T* ServiceProvider::findIf(Predicate predicate)
    {
        static_assert(std::is_invocable_r_v<bool, Predicate, T&>, "Invalid predicate callback: expected (T&) -> bool");

        for (T* const instance : getAll<std::remove_const_t<T>>())
        {
            if (predicate(*instance))
            {
                return instance;
            }
        }

        return nullptr;
    }

    template <rtti::WithTypeInfo T>
    void ServiceProvider::addService(std::unique_ptr<T>&& instance)
    {
        NAU_ASSERT(instance);
        if (!instance)
        {
            return;
        }

        if constexpr (std::is_base_of_v<IRttiObject, T>)
        {
            using Accessor = core_detail::RttiServiceAccessor<std::unique_ptr>;
            addServiceAccessorInternal(std::make_unique<Accessor>(std::move(instance)));
        }
        else
        {
            static_assert(!std::is_abstract_v<T>, "Type can not be registered as service because runtime type info access is not avail (abstract or non IRttiObject)");

            using Accessor = core_detail::NonRttiServiceAccessor<T, std::unique_ptr<T>>;
            addServiceAccessorInternal(std::make_unique<Accessor>(std::move(instance)));
        }
    }

    template <rtti::WithTypeInfo T>
    void ServiceProvider::addService(nau::Ptr<T> instance)
    {
        NAU_ASSERT(instance);
        if (!instance)
        {
            return;
        }

        using Accessor = core_detail::RttiServiceAccessor<nau::Ptr>;
        addServiceAccessorInternal(std::make_unique<Accessor>(std::move(instance)));
    }

    template <rtti::ClassWithTypeInfo T>
    void ServiceProvider::addService()
    {
        if constexpr (std::is_base_of_v<IRefCounted, T>)
        {
            using Accessor = core_detail::RefCountedLazyServiceAccessor;
            auto accessor = std::make_unique<Accessor>(TypeTag<T>{});
            auto classDescriptor = accessor->getClassDescriptor();

            addServiceAccessorInternal(std::move(accessor), std::move(classDescriptor));
        }
        else
        {
            auto factory = []() -> std::unique_ptr<T>
            {
                return std::make_unique<T>();
            };

            using Accessor = core_detail::FactoryLazyServiceAccessor<decltype(factory)>;
            auto accessor = std::make_unique<Accessor>(std::move(factory));
            auto classDescriptor = accessor->getClassDescriptor();

            addServiceAccessorInternal(std::move(accessor), std::move(classDescriptor));
        }
    }

    template <typename F>
    void ServiceProvider::addServiceLazy(F factory)
    {
        static_assert(std::is_invocable_v<F>, "Invalid factory functor signature, or factory is not invocable object.");
        static_assert(core_detail::IsKnownInstancePtr<std::invoke_result_t<F>>, "Factory result must be known pointer: std::unique_ptr<>, std::unique_ptr<>, nau::Ptr<>");

        using Accessor = core_detail::FactoryLazyServiceAccessor<decltype(factory)>;
        auto accessor = std::make_unique<Accessor>(std::move(factory));
        IClassDescriptor::Ptr classDescriptor = accessor->getClassDescriptor();

        addServiceAccessorInternal(std::move(accessor), std::move(classDescriptor));
    }

    template <rtti::ClassWithTypeInfo T>
    void ServiceProvider::addClass()
    {
        static_assert(!std::is_abstract_v<T>, "Invalid class");
        addClass(getClassDescriptor<T>());
    }

    template <rtti::WithTypeInfo T, rtti::WithTypeInfo... U>
    std::vector<IClassDescriptor::Ptr> ServiceProvider::findClasses([[maybe_unused]] bool anyType)
    {
        using namespace nau::rtti;
        if constexpr (sizeof...(U) > 0)
        {
            std::array types = {&getTypeInfo<T>(), &getTypeInfo<U>()...};
            return findClasses(std::span<const TypeInfo*>{types.data(), types.size()}, anyType);
        }
        else
        {
            return findClasses(getTypeInfo<T>());
        }
    }

    NAU_KERNEL_EXPORT ServiceProvider::Ptr createServiceProvider();

    NAU_KERNEL_EXPORT void setDefaultServiceProvider(ServiceProvider::Ptr&&);

    NAU_KERNEL_EXPORT bool hasServiceProvider();

    NAU_KERNEL_EXPORT ServiceProvider& getServiceProvider();
}  // namespace nau