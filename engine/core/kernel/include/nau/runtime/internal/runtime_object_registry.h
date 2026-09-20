// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.
// nau/runtime/runtime_object_registry.h

#pragma once

#include <EASTL/shared_ptr.h>
#include <EASTL/span.h>

#include <memory>

#include "nau/kernel/kernel_config.h"
#include "nau/rtti/ptr.h"
#include "nau/rtti/rtti_object.h"
#include "nau/rtti/weak_ptr.h"

namespace nau
{
    /**
     */
    class NAU_ABSTRACT_TYPE RuntimeObjectRegistry
    {
    public:
        NAU_KERNEL_EXPORT
        static RuntimeObjectRegistry& getInstance();

        NAU_KERNEL_EXPORT
        static bool hasInstance();

        NAU_KERNEL_EXPORT static void setDefaultInstance();

        NAU_KERNEL_EXPORT static void releaseInstance();

        virtual ~RuntimeObjectRegistry() = default;

        // Claim disposal of a registered object once for its registration lifetime.
        // Unregistered objects remain the caller's responsibility.
        virtual bool claimDisposal(IRttiObject&) = 0;

        template <typename Callback>
        requires(std::is_invocable_v<Callback, eastl::span<IRttiObject*>>)
        void visitAllObjects(Callback callback)
        {
            const auto callbackHelper = [](eastl::span<IRttiObject*> objects, void* callbackData)
            {
                (*reinterpret_cast<Callback*>(callbackData))(objects);
            };

            visitObjects(callbackHelper, nullptr, &callback);
        }

        template <typename T, typename Callback>
        requires(std::is_invocable_v<Callback, eastl::span<IRttiObject*>>)
        void visitObjects(Callback callback)
        {
            const auto callbackHelper = [](eastl::span<IRttiObject*> objects, void* callbackData)
            {
                (*reinterpret_cast<Callback*>(callbackData))(objects);
            };

            visitObjects(callbackHelper, &rtti::getTypeInfo<T>(), &callback);
        }

    protected:
        using ObjectId = uint64_t;
        using VisitObjectsCallback = void (*)(eastl::span<IRttiObject*>, void*);

        virtual void visitObjects(VisitObjectsCallback callback, const rtti::TypeInfo*, void*) = 0;

        friend class RuntimeObjectRegistration;
    };

    NAU_KERNEL_EXPORT bool claimRuntimeDisposal(IRttiObject& object);

    /**
     */
    class [[nodiscard]] NAU_KERNEL_EXPORT RuntimeObjectRegistration
    {
    public:
        RuntimeObjectRegistration();
        RuntimeObjectRegistration(nau::Ptr<>);
        RuntimeObjectRegistration(IRttiObject&);
        RuntimeObjectRegistration(RuntimeObjectRegistration&&);
        RuntimeObjectRegistration(const RuntimeObjectRegistration&) = delete;
        ~RuntimeObjectRegistration();

        RuntimeObjectRegistration& operator=(RuntimeObjectRegistration&&);
        RuntimeObjectRegistration& operator=(const RuntimeObjectRegistration&) = delete;
        RuntimeObjectRegistration& operator=(std::nullptr_t);

        explicit operator bool() const;

        void setAutoRemove();

    private:
        void reset();

        RuntimeObjectRegistry::ObjectId m_objectId;

        friend class RuntimeObjectRegistryImpl;
    };

}  // namespace nau
