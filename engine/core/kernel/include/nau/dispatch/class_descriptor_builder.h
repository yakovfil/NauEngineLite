// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once

#include <memory>
#include <string>

#include "nau/dispatch/class_descriptor.h"
#include "nau/dispatch/dispatch.h"
#include "nau/meta/class_info.h"
#include "nau/meta/common_attributes.h"
#include "nau/meta/function_info.h"
#include "nau/rtti/rtti_impl.h"
#include "nau/rtti/weak_ptr.h"
#include "nau/serialization/runtime_value_builder.h"
#include "nau/threading/spin_lock.h"
#include "nau/utils/functor.h"
#include "nau/utils/tuple_utility.h"
#include "nau/utils/type_list/append.h"

namespace nau::nau_detail
{

    template <typename T>
    Result<> assignNativeValue(const nau::Ptr<>& inArg, T& value);

    template <typename... T>
    Result<> assignArgumentValues(const DispatchArguments& inArgs, T&... outValues);

    template <typename T>
    requires(HasRuntimeValueRepresentation<T> || meta::IsCallable<T>)
    nau::Ptr<> makeRuntimeValue(T&& value);

    // SFINAE helper to detect that type/class has custom instance constructor method: Type::classCreateInstance<T>(A...);
    template <typename T, typename... A>
    decltype(T::template classCreateInstance<T>(constLValueRef<A>...), std::true_type{}) hasClassCreateInstanceHelper(int);

    template <typename T, typename... A>
    std::false_type hasClassCreateInstanceHelper(...);

    template <typename T, typename... A>
    constexpr bool HasClassCreateInstance = decltype(hasClassCreateInstanceHelper<T, A...>(int{}))::value;

    template <typename T>
    static inline constexpr bool ClassHasConstructorMethod =
        std::is_base_of_v<IRefCounted, T> || HasClassCreateInstance<T> || std::is_constructible_v<T>;

    template <typename T>
    std::string getTypeName()
    {
        std::string name;

        if constexpr (meta::ClassHasName<T>)
        {
            name = meta::getClassName<T>();
        }
        else
        {
            if (std::string_view typeName = rtti::getTypeInfo<T>().getTypeName(); !typeName.empty())
            {
                name.assign(typeName.data(), typeName.size());
            }
            else
            {
                name = "Unnamed_Class";
            }
        }
        // TODO: modify class name if required (for example replace "::" with "_")
        return name;
    }

    /**
     */
    template <typename F, bool Const, bool NoExcept, typename Class, typename Res, typename... P>
    auto makeFunctionalDispatchWrapper(nau::Ptr<> dispatchPtr, meta::CallableTypeInfo<Const, NoExcept, Class, Res, P...>)
    {
        NAU_ASSERT(dispatchPtr);
        NAU_ASSERT(dispatchPtr->is<IDispatch>());

        return [dispatchPtr = std::move(dispatchPtr)](P... p) -> Res
        {
            constexpr size_t ArgsCount = sizeof...(P);

            DispatchArguments dispatchArgs;
            if constexpr (ArgsCount > 0)
            {
                dispatchArgs.reserve(ArgsCount);
                (dispatchArgs.emplace_back(makeRuntimeValue(std::move(p))), ...);
            }

            auto& dispatch = dispatchPtr->as<IDispatch&>();
            Result<nau::Ptr<>> invokeResult = dispatch.invoke({}, {}, std::move(dispatchArgs));

            return {};
        };
    }

    template <typename F, typename = meta::GetCallableTypeInfo<F>>
    class FunctionDispatchImpl;

    template <typename F, bool Const, bool NoExcept, typename Class, typename Res, typename... P>
    class FunctionDispatchImpl<F, meta::CallableTypeInfo<Const, NoExcept, Class, Res, P...>> final : public IDispatch,
                                                                                                     public virtual IRefCounted
    {
        NAU_CLASS_(FunctionDispatchImpl, IDispatch);

    public:
        FunctionDispatchImpl(F callable) :
            m_callable(std::move(callable))
        {
        }

        Result<nau::Ptr<>> invoke([[maybe_unused]] std::string_view contract, [[maybe_unused]] std::string_view method, DispatchArguments args) override
        {
            NAU_ASSERT(contract.empty());
            NAU_ASSERT(method.empty());
            NAU_ASSERT("Not implemented");
            return nullptr;
        }

        IClassDescriptor::Ptr getClassDescriptor() const override
        {
            return nullptr;
        }

    private:
        F m_callable;
    };

    template <typename T>
    Result<> assignNativeValue(const nau::Ptr<>& inArg, T& outValue)
    {
        if constexpr (HasRuntimeValueRepresentation<T>)
        {
            if (auto* const rtArgValue = inArg->as<RuntimeValue*>())
            {
                return RuntimeValue::assign(makeValueRef(outValue), nau::Ptr{rtArgValue});
            }
            else
            {
                return NauMakeError("Dont known how to assign value");
            }
        }
        else if constexpr (meta::IsCallable<T>)
        {
            auto* dispatch = inArg->as<IDispatch*>();
            NAU_ASSERT(dispatch, "Expected IDispatch api");

            outValue = makeFunctionalDispatchWrapper<T>(inArg, meta::GetCallableTypeInfo<T>{});
            return {};
        }

        return NauMakeError("Dont known how to assign arg value");
    }

    template <typename... A>
    Result<> assignArgumentValues(const DispatchArguments& inArgs, A&... outValues)
    {
        static_assert(sizeof...(A) == 0 || !(std::is_reference_v<A> || ...));

        Error::Ptr error;
        size_t argIndex = 0;

        if (!(assignNativeValue(inArgs[argIndex++], outValues).isSuccess(&error) && ...))
        {
            return error;
        }

        return {};
    }

    template <typename T>
    requires(HasRuntimeValueRepresentation<T> || meta::IsCallable<T>)
    nau::Ptr<> makeRuntimeValue(T&& value)
    {
        if constexpr (HasRuntimeValueRepresentation<T>)
        {
            return makeValueCopy(std::forward<T>(value));
        }
        else
        {
            static_assert(meta::IsCallable<T>);
            using FunctionDispatch = FunctionDispatchImpl<std::decay_t<T>>;

            return rtti::createInstance<FunctionDispatch, IRefCounted>(std::move(value));
        }
    }

    template <typename T>
    class MethodInfoImpl;

    template <auto F, typename... A>
    class MethodInfoImpl<meta::MethodInfo<F, A...>> : public IMethodInfo
    {
        using MethodInfo = meta::MethodInfo<F, A...>;

    public:
        MethodInfoImpl(MethodInfo methodInfo) :
            m_methodInfo(methodInfo)
        {
        }

        std::string getName() const override
        {
            return std::string{m_methodInfo.getName()};
        }

        MethodCategory getCategory() const override
        {
            if constexpr (MethodInfo::IsMemberFunction)
            {
                return MethodCategory::Instance;
            }
            else
            {
                return MethodCategory::Class;
            }
        }

        std::optional<unsigned> getParametersCount() const override
        {
            using Params = typename MethodInfo::FunctionTypeInfo::ParametersList;
            return Params::Size;
        }

        Result<IRttiObject*> invoke(IRttiObject* instance, DispatchArguments args) const override
        {
            return invokeImpl(instance, typename MethodInfo::FunctionTypeInfo{}, args);
        }

    private:
        template <bool Const, bool NoExcept, typename C, typename R, typename... P>
        static Result<IRttiObject*> invokeImpl(IRttiObject* instance, meta::CallableTypeInfo<Const, NoExcept, C, R, P...> callableInfo, DispatchArguments& inArgs)
        {
            if constexpr (MethodInfo::IsMemberFunction)
            {
                return invokeInstanceImpl<C, R>(instance, inArgs, std::remove_const_t<std::remove_reference_t<P>>{}...);
            }
            else
            {  // INVOKE STATIC
                return nullptr;
            }
        }

        template <typename Class, typename R, typename... P>
        static Result<IRttiObject*> invokeInstanceImpl(IRttiObject* instance, DispatchArguments& inArgs, P... arguments)
        {
            NAU_ASSERT(instance);
            NauCheckResult(assignArgumentValues(inArgs, arguments...));

            Class* const api = instance->as<Class*>();
            NAU_ASSERT(api);
            if (!api)
            {
                return NauMakeError("Api not supported");
            }

            if constexpr (std::is_same_v<R, void>)
            {
                (api->*F)(std::move(arguments)...);
                return nullptr;
            }
            else
            {
                auto result = (api->*F)(std::move(arguments)...);
                if constexpr (HasRuntimeValueRepresentation<decltype(result)>)
                {
                    RuntimeValue::Ptr resultAsRuntimeValue;

                    if constexpr (std::is_lvalue_reference_v<decltype(result)>)
                    {
                        resultAsRuntimeValue = makeValueRef(result);
                    }
                    else
                    {
                        resultAsRuntimeValue = makeValueCopy(result);
                    }

                    NAU_FATAL(resultAsRuntimeValue);

                    RuntimeValue* const runtimeResult = resultAsRuntimeValue.giveUp();
                    return rtti::staticCast<IRttiObject*>(runtimeResult);
                }
                else
                {
                    // TODO supports for:
                    // - invocable objects (functor/lambda) as IDispatch interface
                    // - any IRttiObject base objects

                    NAU_FATAL("Returning non runtime values is not implemented");
                }
            }

            return nullptr;
        }

        const MethodInfo m_methodInfo;
    };

    template <typename>
    struct GetMethodInfoImplTuple;

    template <typename... T>
    struct GetMethodInfoImplTuple<std::tuple<T...>>
    {
        using type = std::tuple<MethodInfoImpl<T>...>;
    };

    template <typename T>
    class InterfaceInfoImpl final : public IInterfaceInfo
    {
        using MethodInfoTuple = std::remove_const_t<std::remove_reference_t<decltype(meta::getClassDirectMethods<T>())>>;
        using MethodInfoImplTuple = typename GetMethodInfoImplTuple<MethodInfoTuple>::type;

        template <typename... M>
        inline static MethodInfoImplTuple makeMethodInfos(const std::tuple<M...>& methodInfos)
        {
            return [&]<size_t... I>(std::index_sequence<I...>)
            {
                return std::tuple{std::get<I>(methodInfos)...};
            }(std::make_index_sequence<sizeof...(M)>{});
        }

    public:
        using InterfaceType = T;

        InterfaceInfoImpl() :
            m_methods(makeMethodInfos(meta::getClassDirectMethods<T>()))
        {
        }

        std::string getName() const override
        {
            std::string name = nau_detail::getTypeName<InterfaceType>();
            return std::string{name.data(), name.size()};
        }

        const rtti::TypeInfo* getTypeInfo() const override
        {
            if constexpr (rtti::HasTypeInfo<T>)
            {
                return &rtti::getTypeInfo<T>();
            }
            else
            {
                return nullptr;
            }
        }

        constexpr size_t getMethodsCount() const override
        {
            return std::tuple_size_v<MethodInfoImplTuple>;
        }

        const IMethodInfo& getMethod(size_t i) const override
        {
            NAU_ASSERT(getMethodsCount() > 0 && i < getMethodsCount());

            const IMethodInfo* methodInfoPtr = nullptr;

            if constexpr (std::tuple_size_v<MethodInfoImplTuple> > 0)
            {
                TupleUtils::invokeAt(m_methods, i, [&methodInfoPtr](const IMethodInfo& methodInfo)
                {
                    methodInfoPtr = &methodInfo;
                });
            }

            NAU_ASSERT(methodInfoPtr);

            return *methodInfoPtr;
        }

    private:
        const MethodInfoImplTuple m_methods;
    };

    /**
        @brief Instance constructor implementation
     */
    template <typename T>
    class ClassConstructorImpl final : public IMethodInfo
    {
    public:
        std::string getName() const override
        {
            return ".ctor";
        }

        MethodCategory getCategory() const override
        {
            return MethodCategory::Class;
        }

        std::optional<unsigned> getParametersCount() const override
        {
            return 0;
        }

        Result<IRttiObject*> invoke(IRttiObject*, [[maybe_unused]] DispatchArguments args) const override
        {
            // Prefer custom constructor over default implementation
            // even if instance can be constructed with rtti::createInstanceXXX.
            if constexpr (HasClassCreateInstance<T>)
            {
                return invokeClassCreateInstance(args);
            }
            else if constexpr (std::is_base_of_v<IRefCounted, T>)
            {
                // TODO: need to check first argument is IMemAllocator
                // and using rtti::createInstanceWithAllocator in that case.
                static_assert(std::is_constructible_v<T>, "Type is not default constructible (or not constructible at all)");
                nau::Ptr<T> instance = rtti::createInstance<T, IRefCounted>();

                return instance.giveUp()->template as<IRttiObject*>();
            }
            else if constexpr (std::is_constructible_v<T>)
            {
                T* const instance = new T();
                return rtti::staticCast<IRttiObject*>(instance);
            }
            else
            {
                NAU_FAILURE("Dynamic Construction currently for this Type is not supported/implemented");
                return NauMakeError("Dynamic Construction is not implemented");
            }
        }

    private:
        static IRttiObject* invokeClassCreateInstance(const DispatchArguments& args)
        {
            // TODO: classCreateInstance can support any arguments,
            // but currently checking only for allocator
            if (!args.empty())
            {
                NAU_ASSERT(!args.front() || args.front()->is<IMemAllocator>(), "Instance creation method supports only IMemAllocator as argument");
            }

            IRttiObject* resultInstance = nullptr;
            // if constexpr (std::is_invocable_v<typename T::template classCreateInstance<T>, IMemAllocator*>)
            {
                IMemAllocator* const allocator = nullptr;

                resultInstance = T::template classCreateInstance<T>(allocator);
            }
            // else
            // {
            //     resultInstance = typename T::template classCreateInstance<T>();
            // }

            return resultInstance;
        }
    };

    class ClassConstructorStubImpl final : public IMethodInfo
    {
    public:
        std::string getName() const override
        {
            return ".ctor";
        }

        MethodCategory getCategory() const override
        {
            return MethodCategory::Class;
        }

        std::optional<unsigned> getParametersCount() const override
        {
            return 0;
        }

        Result<IRttiObject*> invoke([[maybe_unused]] IRttiObject*, [[maybe_unused]] DispatchArguments args) const override
        {
            NAU_FAILURE("This method must never be called");
            return NauMakeError("Construction for this type is not supported");
        }
    };

    template <typename T>
    class ClassDescriptorImpl final : public IClassDescriptor
    {
        NAU_CLASS_(ClassDescriptorImpl<T>, IClassDescriptor)

        using ApiCollection = type_list::Append<meta::ClassAllUniqueBase<T>, T>;
        using InterfaceInfoTuple = nau::TupleUtils::TupleFrom<type_list::Transform<ApiCollection, InterfaceInfoImpl>>;

    public:
        ClassDescriptorImpl()
        {
            using namespace nau::meta;
            m_attributes = std::make_unique<RuntimeAttributeContainer>(makeRuntimeAttributeContainer<T>());
        }

        const rtti::TypeInfo& getClassTypeInfo() const override
        {
            return rtti::getTypeInfo<T>();
        }

        std::string getClassName() const override
        {
            std::string name = nau_detail::getTypeName<T>();
            return std::string{name.data(), name.size()};
        }

        const meta::IRuntimeAttributeContainer* getClassAttributes() const override
        {
            return m_attributes.get();
        }

        size_t getInterfaceCount() const override
        {
            return ApiCollection::Size;
        }

        const IInterfaceInfo& getInterface(size_t i) const override
        {
            NAU_ASSERT(getInterfaceCount() > 0 && i < getInterfaceCount());

            const IInterfaceInfo* interfaceInfoPtr = nullptr;

            TupleUtils::invokeAt(m_interfaces, i, [&interfaceInfoPtr](const IInterfaceInfo& interfaceInfo)
            {
                interfaceInfoPtr = &interfaceInfo;
            });

            NAU_ASSERT(interfaceInfoPtr);

            return *interfaceInfoPtr;
        }

        const IMethodInfo* getConstructor() const override
        {
            if constexpr (ClassHasConstructorMethod<T>)
            {
                return &m_ctorMethod;
            }
            else
            {
                return nullptr;
            }
        }

    private:
        using ClassConstructorMethod = std::conditional_t<ClassHasConstructorMethod<T>, ClassConstructorImpl<T>, ClassConstructorStubImpl>;

        const InterfaceInfoTuple m_interfaces;
        const ClassConstructorMethod m_ctorMethod;
        std::unique_ptr<meta::RuntimeAttributeContainer> m_attributes;
    };

}  // namespace nau::nau_detail

namespace nau
{
    template <typename T>
    IClassDescriptor::Ptr getClassDescriptor()
    {
        static WeakPtr<IClassDescriptor> descriptorWeakRef;
        static threading::SpinLock mutex;

        const std::lock_guard lock{mutex};
        Ptr<IClassDescriptor> classDescriptor = descriptorWeakRef.lock();
        if (!classDescriptor)
        {
            classDescriptor = rtti::createInstance<nau_detail::ClassDescriptorImpl<T>>();
            descriptorWeakRef = classDescriptor;
        }

        return classDescriptor;
    }

}  // namespace nau
