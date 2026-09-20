// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "./service_provider_impl.h"

#include "nau/memory/eastl_aliases.h"
#include "nau/runtime/async_disposable.h"
#include "nau/runtime/disposable.h"
#include "nau/runtime/internal/runtime_object_registry.h"

namespace nau
{
    namespace
    {
        struct ServiceEntry
        {
            IServiceInitialization* service;
            bool collectingDependencies = true;
            eastl::set<rtti::TypeIndex> dependencies;

            ServiceEntry(IServiceInitialization* serviceIn) :
                service(serviceIn)
            {
            }

        private:
            void appendDependencies(const eastl::vector<const rtti::TypeInfo*>& typeInfoCollection)
            {
                for (const rtti::TypeInfo* const type : typeInfoCollection)
                {
                    dependencies.emplace(rtti::TypeIndex{*type});
                }
            }

            void appendDependencies(const eastl::set<rtti::TypeIndex> typeInfoCollection)
            {
                for (const rtti::TypeIndex& type : typeInfoCollection)
                {
                    dependencies.emplace(type);
                }
            }

            bool isDependsOn(const ServiceEntry& other) const
            {
                if (dependencies.empty() || other.service == service)
                {
                    return false;
                }

                return std::any_of(dependencies.begin(), dependencies.end(), [service = other.service](const rtti::TypeIndex& t)
                {
                    return service->is(t.getType());
                });
            }

            friend class OrderedServiceListBuilder;
        };

        class OrderedServiceListBuilder
        {
        public:
            OrderedServiceListBuilder(eastl::vector<IServiceInitialization*> allServices) :
                m_allServices(std::move(allServices))
            {
            }

            eastl::tuple<eastl::list<ServiceEntry>, eastl::list<ServiceEntry>> takeOutServiceList()
            {
                for (IServiceInitialization* const service : m_allServices)
                {
                    [[maybe_unused]] auto& entry = getServiceEntry(service);
                }

                NAU_FATAL(m_services.size() == m_allServices.size());

                m_services.sort([](const ServiceEntry& left, const ServiceEntry& right)
                {
                    return right.isDependsOn(left);
                });

                // ensure that services without dependencies have forced previous services with dependencies
                eastl::list<ServiceEntry> servicesWithNoDependencies;
                for (auto iter = m_services.begin(); iter != m_services.end();)
                {
                    if (iter->dependencies.empty())
                    {
                        auto temp = iter;
                        ++iter;
                        servicesWithNoDependencies.splice(servicesWithNoDependencies.end(), m_services, temp);
                    }
                    else
                    {
                        ++iter;
                    }
                }

                NAU_FATAL(m_services.size() + servicesWithNoDependencies.size() == m_allServices.size());

                return {std::move(servicesWithNoDependencies), std::move(m_services)};
            }

        private:
            static bool isDependencyFor(const IServiceInitialization& service, const eastl::vector<const rtti::TypeInfo*>& types)
            {
                return std::any_of(types.begin(), types.end(), [&service](const rtti::TypeInfo* t)
                {
                    return service.is(*t);
                });
            }

            ServiceEntry& getServiceEntry(IServiceInitialization* service)
            {
                auto iter = eastl::find_if(m_services.begin(), m_services.end(), [service](const ServiceEntry& entry)
                {
                    return entry.service == service;
                });

                if (iter == m_services.end())
                {
                    m_services.emplace_back(service);
                    auto& entry = m_services.back();
                    entry.collectingDependencies = true;

                    if (auto directDependencies = service->getServiceDependencies(); !directDependencies.empty())
                    {
                        entry.appendDependencies(directDependencies);

                        for (IServiceInitialization* const otherService : m_allServices)
                        {
                            if (otherService == service)
                            {
                                continue;
                            }

                            if (isDependencyFor(*otherService, directDependencies))
                            {
                                ServiceEntry& otherServiceEntry = getServiceEntry(otherService);
                                entry.appendDependencies(otherServiceEntry.dependencies);
                            }
                        }
                    }

                    entry.collectingDependencies = false;
                    return entry;
                }

                NAU_FATAL(!iter->collectingDependencies, "Service cyclic dependency");
                return *iter;
            }

            const eastl::vector<IServiceInitialization*> m_allServices;

            eastl::list<ServiceEntry> m_services;
        };

        /**
            reorder services to takes into account the dependencies between them:
            dependencies come first, followed by other services that use them.

            returns two sequences : 1) independent services (can initialize in parallel) 2) ordered dependent services (MUST initialize sequentially)
         */
        auto makeInitOrderedServiceList(eastl::vector<IServiceInitialization*> allServices)
        {
            OrderedServiceListBuilder serviceListBuilder{std::move(allServices)};
            return serviceListBuilder.takeOutServiceList();
        }

        /**
            reorder services to shutdown to takes into account the dependencies between them:
            returns two sequences : 1) independent services (MUST shutdown in parallel) 2) ordered dependent services (MUST shutdown sequentially)
         */
        std::tuple<eastl::list<IServiceShutdown*>, eastl::list<IServiceShutdown*>> makeShutdownOrderedServiceList(eastl::vector<IServiceShutdown*> unorderedShutdownSequence)
        {
            eastl::vector<IServiceInitialization*> initializationList;
            initializationList.reserve(unorderedShutdownSequence.size());

            eastl::list<IServiceShutdown*> orderedIndependentServices;
            eastl::list<IServiceShutdown*> orderedDependentServices;

            for (IServiceShutdown* const serviceShutdown : unorderedShutdownSequence)
            {
                NAU_FATAL(serviceShutdown);
                if (auto* const serviceInitialization = serviceShutdown->as<IServiceInitialization*>(); serviceInitialization)
                {
                    initializationList.push_back(serviceInitialization);
                }
                else
                {
                    // service without initialization: just adding it into sequence, but it will be executed after interdependent services.
                    orderedIndependentServices.push_back(serviceShutdown);
                }
            }

            if (!initializationList.empty())
            {
                auto [independentServices, dependentServices] = makeInitOrderedServiceList(initializationList);

                for (auto iter = independentServices.begin(); iter != independentServices.end(); ++iter)
                {
                    IServiceShutdown* const serviceShutdown = iter->service->as<IServiceShutdown*>();
                    NAU_FATAL(serviceShutdown);
                    orderedIndependentServices.push_front(serviceShutdown);
                }

                for (auto iter = dependentServices.begin(); iter != dependentServices.end(); ++iter)
                {
                    IServiceShutdown* const serviceShutdown = iter->service->as<IServiceShutdown*>();
                    NAU_FATAL(serviceShutdown);
                    // will shutdown services in reverse order of its initialization
                    orderedDependentServices.push_front(serviceShutdown);
                }
            }

            NAU_ASSERT(orderedIndependentServices.size() + orderedDependentServices.size() == unorderedShutdownSequence.size());

            return {std::move(orderedIndependentServices), std::move(orderedDependentServices)};
        }

    }  // namespace

    ServiceProviderImpl::ServiceProviderImpl()
    {
    }

    ServiceProviderImpl::~ServiceProviderImpl()
    {
        if (!m_isDisposed)
        {
        }
    }

    void* ServiceProviderImpl::findInternal(const rtti::TypeInfo& type)
    {
        ServiceAccessor* accessor = nullptr;

        {
            shared_lock_(m_mutex);

            auto iter = m_instances.find(type);
            if (iter != m_instances.end())
            {
                return iter->second;
            }

            auto accessorIter = std::find_if(m_accessors.begin(), m_accessors.end(), [&type](const ServiceAccessor::Ptr& accessor)
            {
                return accessor->hasApi(type);
            });
            accessor = accessorIter != m_accessors.end() ? accessorIter->get() : nullptr;
        }

        // getApi can also access to the service provider (through lazy service creation and service impl constructor's invocation)
        void* const api = accessor ? accessor->getApi(type) : nullptr;
        if (api)
        {
            lock_(m_mutex);
            m_instances.emplace(type, ServiceInstanceEntry{api, accessor});
        }

        return api;
    }

    void ServiceProviderImpl::findAllInternal(const rtti::TypeInfo& type, void (*callback)(void* instancePtr, void*), void* callbackData, ServiceAccessor::GetApiMode getApiMode)
    {
        NAU_ASSERT(callback);
        if (!callback)
        {
            return;
        }

        // todo: use stack allocator
        eastl::vector<ServiceAccessor*> accessors;
        {
            shared_lock_(m_mutex);
            for (const ServiceAccessor::Ptr& accessor : m_accessors)
            {
                if (accessor->hasApi(type))
                {
                    accessors.emplace_back(accessor.get());
                }
            }
        }

        for (auto& accessor : accessors)
        {
            // be aware: this is normal if even accessor::hasApi(type) return true,
            // but accessor::getApi(type) return nullptr (this can be when getApiMode == GetApiMode::DoNotCreate)
            void* const api = accessor->getApi(type, getApiMode);
            if (api)
            {
                callback(api, callbackData);
            }
        }
    }

    void ServiceProviderImpl::addServiceAccessorInternal(ServiceAccessor::Ptr accessor, IClassDescriptor::Ptr classDescriptor)
    {
        NAU_ASSERT(accessor);
        if (!accessor)
        {
            return;
        }

        lock_(m_mutex);
        NAU_ASSERT(!m_isDisposed);

        m_accessors.emplace_back(std::move(accessor));
    }

    void ServiceProviderImpl::addClass(IClassDescriptor::Ptr&& descriptor)
    {
        NAU_ASSERT(descriptor);
        NAU_ASSERT(descriptor->getInterfaceCount() > 0);

        lock_(m_mutex);
        m_classDescriptors.push_back(std::move(descriptor));
    }

    eastl::vector<IClassDescriptor::Ptr> ServiceProviderImpl::findClasses(const rtti::TypeInfo& t)
    {
        eastl::vector<IClassDescriptor::Ptr> classes;

        shared_lock_(m_mutex);

        for (const auto& classDescriptor : m_classDescriptors)
        {
            for (size_t i = 0, count = classDescriptor->getInterfaceCount(); i < count; ++i)
            {
                if (const auto* apiType = classDescriptor->getInterface(i).getTypeInfo(); apiType && *apiType == t)
                {
                    classes.push_back(classDescriptor.get());
                    break;
                }
            }
        }

        return classes;
    }

    eastl::vector<IClassDescriptor::Ptr> ServiceProviderImpl::findClasses(eastl::span<const rtti::TypeInfo*> types, bool anyType)
    {
        using MatchPredicate = bool (*)(decltype(types), const IClassDescriptor&);

        eastl::vector<IClassDescriptor::Ptr> classes;

        const MatchPredicate matchAny = [](decltype(types) types, const IClassDescriptor& classDescriptor) -> bool
        {
            return eastl::any_of(types.begin(), types.end(), [&classDescriptor](const rtti::TypeInfo* type)
            {
                NAU_FATAL(type);
                return classDescriptor.hasInterface(*type);
            });
        };

        const MatchPredicate matchAll = [](decltype(types) types, const IClassDescriptor& classDescriptor) -> bool
        {
            return eastl::all_of(types.begin(), types.end(), [&classDescriptor](const rtti::TypeInfo* type)
            {
                NAU_FATAL(type);
                return classDescriptor.hasInterface(*type);
            });
        };

        const auto predicate = anyType ? matchAny : matchAll;

        shared_lock_(m_mutex);

        for (const IClassDescriptor::Ptr& classDescriptor : m_classDescriptors)
        {
            if (predicate(types, *classDescriptor))
            {
                classes.push_back(classDescriptor.get());
            }
        }

        return classes;
    }

    bool ServiceProviderImpl::hasApiInternal(const rtti::TypeInfo& type)
    {
        shared_lock_(m_mutex);

        return eastl::any_of(m_accessors.begin(), m_accessors.end(), [&type](ServiceAccessor::Ptr& accessor)
        {
            return accessor->hasApi(type);
        });
    }

    template <typename T>
    T& ServiceProviderImpl::getInitializationInstance(T* instance)
    {
        NAU_FATAL(instance);
        shared_lock_(m_mutex);

        if constexpr (std::is_same_v<IServiceInitialization, T>)
        {
            auto proxy = m_initializationProxy.find(instance);
            return proxy == m_initializationProxy.end() ? *instance : *proxy->second;
        }
        else
        {
            const IServiceInitialization* const serviceInit = instance->template as<const IServiceInitialization*>();
            if (auto proxyIter = m_initializationProxy.find(serviceInit); proxyIter != m_initializationProxy.end())
            {
                IServiceInitialization* const proxyServiceInit = proxyIter->second;
                T* const proxyTargetApi = proxyServiceInit->as<T*>();
                return proxyTargetApi ? *proxyTargetApi : *instance;
            }

            return *instance;
        }
    }

    Error::Ptr ServiceProviderImpl::getLifecycleError() const
    {
        if (m_initializationError)
            return m_initializationError;
        if (m_shutdownError)
            return m_shutdownError;
        for (const auto* task : m_activeLifecycleTasks)
        {
            if (*task && task->isReady() && task->isRejected())
                return task->getError();
        }
        return nullptr;
    }

    uint64_t ServiceProviderImpl::getLifecycleProgress() const
    {
        uint64_t progress = m_completedServiceTasks;
        for (const auto* task : m_activeLifecycleTasks)
        {
            if (*task && task->isReady())
                ++progress;
        }
        return progress;
    }

    async::Task<> ServiceProviderImpl::initServicesInternal(async::Task<> (*getTaskCallback)(IServiceInitialization&), bool finalPhase)
    {
        using namespace nau::async;

        if (m_initializationError)
        {
            co_await Result<>{m_initializationError};
        }

        eastl::vector<IServiceInitialization*> services;
        findAllInternal(rtti::getTypeInfo<IServiceInitialization>(), [](void* servicePtr, void* serviceCollection)
        {
            reinterpret_cast<decltype(services)*>(serviceCollection)->push_back(reinterpret_cast<IServiceInitialization*>(servicePtr));
        }, &services, ServiceAccessor::GetApiMode::AllowLazyCreation);

        auto [independentServices, orderedDependentServices] = makeInitOrderedServiceList(services);

        {
            eastl::vector<eastl::pair<IServiceInitialization*, Task<>>> independentInitializationTasks;
            for (const ServiceEntry& entry : independentServices)
            {
                if (m_stopToken && m_stopToken->load())
                {
                    break;
                }
                IServiceInitialization& serviceInstance = getInitializationInstance(entry.service);
                independentInitializationTasks.emplace_back(entry.service, getTaskCallback(serviceInstance));
            }

            for (const auto& entry : independentInitializationTasks)
            {
                m_activeLifecycleTasks.push_back(&entry.second);
            }
            scope_on_leave
            {
                m_activeLifecycleTasks.clear();
            };
            eastl::string errors;
            for (auto& [service, task] : independentInitializationTasks)
            {
                const Result<> result = task ? co_await task.doTry() : Result<>{};
                ++m_completedServiceTasks;
                if (!result)
                {
                    if (!errors.empty())
                    {
                        errors += "\n";
                    }
                    errors += result.getError()->getDiagMessage();
                }
                else if (finalPhase)
                {
                    m_initializedServices.push_back(service);
                }
            }
            if (!errors.empty())
            {
                m_initializationError = NauMakeError(errors);
                co_await Result<>{m_initializationError};
            }
        }

        for (const ServiceEntry& serviceEntry : orderedDependentServices)
        {
            if (m_stopToken && m_stopToken->load())
            {
                co_return;
            }
            IServiceInitialization& serviceInstance = getInitializationInstance(serviceEntry.service);
            if (Task<> task = getTaskCallback(serviceInstance); task)
            {
                if (const Result<> result = co_await task.doTry(); !result)
                {
                    m_initializationError = result.getError();
                    co_await Result<>{m_initializationError};
                }
            }
            if (finalPhase)
            {
                m_initializedServices.push_back(serviceEntry.service);
            }
            ++m_completedServiceTasks;
        }
    }

    void ServiceProviderImpl::setInitializationProxy(const IServiceInitialization& source, IServiceInitialization* proxy)
    {
        lock_(m_mutex);

        if (proxy)
        {
            NAU_ASSERT(!m_initializationProxy.contains(&source), "Proxy for source already set");
            m_initializationProxy[&source] = proxy;
        }
        else
        {
            m_initializationProxy.erase(&source);
        }
    }

    async::Task<> ServiceProviderImpl::preInitServices()
    {
        return initServicesInternal([](IServiceInitialization& serviceInit)
        {
            return serviceInit.preInitService();
        }, false);
    }

    async::Task<> ServiceProviderImpl::initServices()
    {
        return initServicesInternal([](IServiceInitialization& serviceInit)
        {
            return serviceInit.initService();
        }, true);
    }

    async::Task<> ServiceProviderImpl::shutdownServices()
    {
        using namespace nau::async;
        eastl::string errors;
        const auto recordError = [this, &errors](const Result<>& result)
        {
            ++m_completedServiceTasks;
            if (!result)
            {
                if (!errors.empty())
                {
                    errors += "\n";
                }
                errors += result.getError()->getDiagMessage();
                m_shutdownError = NauMakeError(errors);
            }
        };

        constexpr auto NoLazyCreation = ServiceAccessor::GetApiMode::DoNotCreate;

        {
            lock_(m_mutex);
            m_isDisposed = true;
        }

        // Seems there is no need from this point to keep m_mutex locked any more:
        // m_accessors collection must not changed because shutdown

        // 1.1.creating ordered shutdown sequence: reverse order of initialization
        // 1.2 shutdown sequence consists of two sub-sequences:
        //      1) services with dependencies: must shutdown sequentially in specified order
        //      2) services without dependencies: must shutdown without blocking each other (i.e. run serviceShutdown without waiting previous)
        // 2. invoke service disposeAsync -> dispose, wait all dispose task

        {
            eastl::vector<IServiceShutdown*> unorderedShutdownSequence;

            for (const ServiceAccessor::Ptr& accessor : m_accessors)
            {
                if (void* const serviceShutdown = accessor->getApi(rtti::getTypeInfo<IServiceShutdown>(), NoLazyCreation))
                {
                    auto* service = reinterpret_cast<IServiceShutdown*>(serviceShutdown);
                    const auto* initialization = service->as<IServiceInitialization*>();
                    if (!initialization || eastl::find(m_initializedServices.begin(), m_initializedServices.end(), initialization) != m_initializedServices.end())
                    {
                        unorderedShutdownSequence.push_back(service);
                    }
                }
            }

            auto [independentServices, orderedDependentServices] = makeShutdownOrderedServiceList(unorderedShutdownSequence);

            for (IServiceShutdown* const serviceShutdown : orderedDependentServices)
            {
                if (auto task = getInitializationInstance(serviceShutdown).shutdownService(); task)
                {
                    recordError(co_await task.doTry());
                }
            }

            eastl::vector<Task<>> shutdownIndependentTasks;
            for (IServiceShutdown* const serviceShutdown : independentServices)
            {
                if (auto task = getInitializationInstance(serviceShutdown).shutdownService(); task)
                {
                    shutdownIndependentTasks.emplace_back(std::move(task));
                }
            }

            for (const auto& task : shutdownIndependentTasks)
                m_activeLifecycleTasks.push_back(&task);
            scope_on_leave
            {
                m_activeLifecycleTasks.clear();
            };
            for (auto& task : shutdownIndependentTasks)
            {
                recordError(co_await task.doTry());
            }
        }

        {
            eastl::vector<Task<>> disposeTasks;

            {
                for (const ServiceAccessor::Ptr& accessor : m_accessors)
                {
                    auto* object = reinterpret_cast<IRttiObject*>(accessor->getApi(rtti::getTypeInfo<IRttiObject>(), NoLazyCreation));
                    if (object && (object->is<IAsyncDisposable>() || object->is<IDisposable>()) && !claimRuntimeDisposal(*object))
                    {
                        continue;
                    }
                    // invoke disposeAsync first.
                    // same class can provide both IAsyncDisposable and IDisposable api,
                    // so first async version must be called and in this case dispose can do nothing (because if async variant called).
                    if (void* const asyncDisposable = accessor->getApi(rtti::getTypeInfo<IAsyncDisposable>(), NoLazyCreation))
                    {
                        Task<> task = reinterpret_cast<IAsyncDisposable*>(asyncDisposable)->disposeAsync();
                        if (task)
                        {
                            disposeTasks.push_back(std::move(task));
                        }
                    }

                    if (void* const disposable = accessor->getApi(rtti::getTypeInfo<IDisposable>(), NoLazyCreation))
                    {
                        reinterpret_cast<IDisposable*>(disposable)->dispose();
                    }
                }
            }

            for (const auto& task : disposeTasks)
                m_activeLifecycleTasks.push_back(&task);
            scope_on_leave
            {
                m_activeLifecycleTasks.clear();
            };
            for (auto& task : disposeTasks)
            {
                recordError(co_await task.doTry());
            }
        }
        if (!errors.empty())
        {
            co_await Result<>{NauMakeError(errors)};
        }
    }

    namespace
    {
        ServiceProvider::Ptr& getServiceProviderInstanceRef()
        {
            static ServiceProvider::Ptr s_serviceProvider;
            return (s_serviceProvider);
        }
    }  // namespace

    ServiceProvider::Ptr createServiceProvider()
    {
        return eastl::make_unique<ServiceProviderImpl>();
    }

    void setDefaultServiceProvider(ServiceProvider::Ptr&& provider)
    {
        NAU_FATAL(!provider || !getServiceProviderInstanceRef(), "Service provider already set");
        getServiceProviderInstanceRef() = std::move(provider);
    }

    bool hasServiceProvider()
    {
        return static_cast<bool>(getServiceProviderInstanceRef());
    }

    ServiceProvider& getServiceProvider()
    {
        auto& instance = getServiceProviderInstanceRef();
        NAU_FATAL(instance);
        return *instance;
    }

}  // namespace nau
