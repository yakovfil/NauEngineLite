// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once

#include <concepts>
#include <functional>
#include <optional>
#include <tuple>
#include <type_traits>

#include "nau/kernel/kernel_config.h"
#include "nau/memory/mem_allocator.h"
#include "nau/memory/standard_aliases.h"
#include "nau/rtti/rtti_impl.h"
#include "nau/rtti/rtti_object.h"
#include "nau/runtime/disposable.h"
#include "nau/scene/scene_query.h"
#include "nau/serialization/native_runtime_value/native_value_base.h"
#include "nau/utils/intrusive_list.h"
#include "nau/utils/uid.h"

namespace nau::scene
{
    class NauObject;

    template <typename = NauObject>
    class ObjectUniquePtr;

    template <typename = NauObject>
    class ObjectWeakRef;

}  // namespace nau::scene

namespace nau::scene_internal
{
    /**
     * Base class for a weak reference to a NauObject instance. 
     * 
     * An ObjectWeakRefBase object maintains reference count on binding/unbinding the referenced object.
     * It is not responsible for memory deallocation on destruction of the referenced object.
     * See also: ObjectUniquePtr.
     */
    class NAU_CORESCENE_EXPORT ObjectWeakRefBase : public nau::IntrusiveListNode
    {
    protected:

        /**
         * @brief Destructor.
         */
        ~ObjectWeakRefBase();

        /**
         * @brief Default constructor. 
         * 
         * Leaves the reference set to NULL.
         */
        ObjectWeakRefBase();

        /**
         * @brief Initialization constructor.
         * 
         * Binds the reference to **object**.
         */
        ObjectWeakRefBase(scene::NauObject* object);

        /**
         * @brief Copy constructor.
         * 
         * @param [in] other Another reference to the object.
         * 
         * Binds the refence to the object referenced by **other**.
         */
        ObjectWeakRefBase(const ObjectWeakRefBase&);

        /**
         * @brief Assignment operator.
         * 
         * @param [in] right    Assigned object.
         * @return              A reference to the resulted object.
         * 
         * Unbinds the reference from the currently referenced object and binds it to the object referenced by **right**.
         */
        ObjectWeakRefBase& operator=(const ObjectWeakRefBase&);

        /**
         * @brief Checks whether two ObjectWeakRefBase instances reference the same object.
         * 
         * @param [in] other    Instance to compare this reference with.
         * @return              `true` if both references are bound to the same object.
         */
        bool equals(const ObjectWeakRefBase& other) const;

        /**
         * @brief Resets the reference and possibly binds it to a different object.
         * 
         * @param [in] newObject    A pointer to the object to bind the reference to. `NULL` can be passed to render the reference unbound.
         */
        void reset(scene::NauObject* object = nullptr);

        /**
         * @brief Retrieves a non-const pointer to the referenced object.
         * 
         * @tparam T Type of the output. It has to be either statically convertible to NauObject or RTTI-castable to NauObject.
         * 
         * @return  Retrieved pointer to the referenced object.
         */
        template <typename T = scene::NauObject>
        T* getMutableTypedPtr() const;

        /**
         * @brief Retrieves a non-const pointer to the referenced object.
         * 
         * @return  Retrieved pointer to the referenced object.
         */
        scene::NauObject* getMutableNauObjectPtr() const
        {
            return m_object;
        }

        /**
         * @brief Checks whether the reference is valid.
         * 
         * @return `true` if the refence is valied (i.e. the bound object can be safely obtained from it), `false` otherwise.
         */
        bool refIsValid() const;


    private:

        /**
         * @brief Called upon the referenced object destruction.
         */
        void notifyReferencedObjectDestroyed()
        {
            m_object = nullptr;
        }

        scene::NauObject* m_object = nullptr;
        std::optional<scene::SceneQuery> m_objectQuery;

        friend class scene::NauObject;

        template <typename T>
        friend class scene::ObjectWeakRef;

        friend class RuntimeObjectWeakRefValueImpl;
    };

}  // namespace nau::scene_internal

namespace nau::scene
{
    /**
     */
    enum class ActivationState
    {
        Inactive,
        Activating,
        Active,
        Deactivating
    };

    /**
     * @brief Casts an object to a NauObject instance.
     * 
     * @tparam U Type of the casted object. It has to be a subclass of `IRttiObject`.
     * 
     * @param [in] object   A pointer to the object to cast.
     * @return              A pointer to the NauObject instance.
     * 
     * @note Type **U** of the casted object has to be derived from `IRttiObject`.
     */
    template <typename U>
    NauObject* castToNauObject(U* object);


    /**
     * @brief Casts an object to a constant NauObject instance.
     * 
     * @tparam U Type of the casted object. It has to support Nau Engine RTTI.
     * 
     * @param [in] object   A pointer to the constant object to cast.
     * @return              A pointer to the constant NauObject instance.
     * 
     * @note Type **U** of the casted object has to be derived from `IRttiObject`.
     */
    template <typename U>
    const NauObject* castToNauObject(const U* object);

    /**
     * @brief Base class for any scene-related object.
     * 
     * This class supports reference counting mechanism when the object is addressed using ObjectWeakRefю
     */
    class NAU_CORESCENE_EXPORT NAU_ABSTRACT_TYPE NauObject : public virtual IRttiObject
    {
        NAU_INTERFACE(nau::scene::NauObject, IRttiObject)

        NAU_CLASS_FIELDS(
            CLASS_NAMED_FIELD(m_uid, "uid"))

    public:

        /**
         * @brief Constructs an object.
         * 
         * @tparam T    Type of the object to construct. It has to be derived from NauObject.
         * @tparam A    Parameter pack for the object constructor.
         * 
         * @param [in] allocator    Allocator to use for costructing the object. If `NULL` is passed, the default allocator is going to be used.
         * @param [in] a            Object constructor arguments.
         * @return                  A pointer to the constructed object.
         */
        template <typename T, typename... A>
        static T* classCreateInstance(IMemAllocator* allocator = nullptr, A&&... a)
        {
            static_assert(!std::is_abstract_v<T>, "classCreateInstance<ABSTRACT_TYPE>");
            static_assert(std::is_constructible_v<T, A...>, "Invalid construction arguments");
            constexpr size_t Alignment = alignof(T);

            IMemAllocator* const actualAllocator = allocator ? allocator : getDefaultAllocator().get();
            NAU_FATAL(actualAllocator);

            void* mem = actualAllocator->allocateAligned(sizeof(T), Alignment);
            NAU_FATAL(mem);
            NAU_FATAL(reinterpret_cast<uintptr_t>(mem) % Alignment == 0);

            T* const instance = new(mem) T(std::forward<A>(a)...);
            NauObject* const nauObject = rtti::staticCast<NauObject*>(instance);

            nauObject->m_allocator = actualAllocator;
            nauObject->m_allocationAddress = mem;
            nauObject->m_destructorFunc = [](NauObject* selfAsNauObject) noexcept -> void*
            {
                NAU_FATAL(selfAsNauObject);
                T* const self = static_cast<T*>(selfAsNauObject);
                if (IDisposable* const disposable = self->template as<IDisposable*>())
                {
                    disposable->dispose();
                }

                selfAsNauObject->onBeforeDeleteObject();
                void* const address = selfAsNauObject->m_allocationAddress;

                std::destroy_at(self);
                void* const ptr = reinterpret_cast<void*>(address);
#if NAU_DEBUG
                memset(ptr, 0, sizeof(T));
#endif
                return ptr;
            };

            return instance;
        }

#ifdef NAU_ASSERT_ENABLED
        static void operator delete(void*, size_t);
#endif

        /**
         * @brief Destructor.
         */
        virtual ~NauObject();


        /**
         * @brief Retrieves the identifier of the object.
         * 
         * @return UID of the object.
         */
        Uid getUid() const;

        /**
         * @brief Assigns an identifier to the object.
         * 
         * @param [in] uid Identifier to assign.
         */
        void setUid(Uid);

        /**
         * @brief Immediately unbinds all references from the object and destroys it, effectively deallocating memory.
         */
        virtual void destroy();

    protected:

        /**
         * @brief Immediately unbinds all references from the object and destroys it, effectively deallocating memory.
         */
        void deleteObjectNow();

        /**
         * @brief Unbinds all weak references from the object.
         */
        void clearAllWeakReferences();

        virtual void onBeforeDeleteObject();

    private:
        using DestructorFunc = void* (*)(NauObject* self) noexcept;

        /**
         * @brief Adds the reference from the list.
         *
         * @param [in] reference Reference to add.
         */
        void addReference(scene_internal::ObjectWeakRefBase& reference);

        /**
         * @brief Removes the reference from the list.
         * 
         * @param [in] reference Reference to remove.
         */
        void removeReference(scene_internal::ObjectWeakRefBase& reference);

        void setHasPtrOwner(bool hasPtrOwner)
        {
            NAU_FATAL(hasPtrOwner || m_hasPtrOwner, "Resetting ownership, but object is not owned");
            NAU_FATAL(!hasPtrOwner || !m_hasPtrOwner, "Object is already owned by ObjectUniquePtr");

            m_hasPtrOwner = hasPtrOwner;
        }

        bool m_hasPtrOwner = false;

        /**
         * @brief List of references bound to the object.
         */
        nau::IntrusiveList<scene_internal::ObjectWeakRefBase> m_references;

        IMemAllocator* m_allocator = nullptr;
        void* m_allocationAddress = nullptr;

        DestructorFunc m_destructorFunc; /** < Called upon the actual object destruction. */
        Uid m_uid;

        friend class SceneManagerImpl;

        template <typename>
        friend class ObjectUniquePtr;

        template <typename>
        friend class ObjectWeakRef;

        friend class scene_internal::ObjectWeakRefBase;
    };

    /**
     * @brief Incapsulates unique pointer to a NauObject instance. 
     * 
     * Apart from standard `unique_ptr` functionality (RAII support, unique ownership) 
     * it can provide non-owning access to the managed object to ObjectWeakRef instances.
     */
    template <typename T>
    class ObjectUniquePtr
    {
    public:

        /**
         * @brief Destructor.
         * 
         * @note Calling this does not destroy the object. Instead it decrements its reference count.
         */
        ~ObjectUniquePtr()
        {
            reset();
        }

        /**
         * @brief Default construtor.
         */
        ObjectUniquePtr() = default;
        
        /**
         * @brief Copy constructor (deleted).
         */
        ObjectUniquePtr(const ObjectUniquePtr&) = delete;

        /**
         * @brief Initialization constructor.
         * 
         * @param [in] object Object to be managed by the smart pointer.
         */
        ObjectUniquePtr(T* object) :
            m_object(object)
        {
            NAU_ASSERT(m_object, "object can not be null for this constructor");
            if (m_object)
            {
                castToNauObject(m_object)->setHasPtrOwner(true);
            }
        }

        ObjectUniquePtr(std::nullptr_t) :
            ObjectUniquePtr{}
        {
        }

        /**
         * @brief Move constructor.
         * 
         * @param [in] other Smart pointer to move from.
         */
        ObjectUniquePtr(ObjectUniquePtr&& other) :
            m_object(std::exchange(other.m_object, nullptr))
        {
        }

        /**
         * @brief Move constructor.
         * 
         * @tparam U Type of the object managed by another smart pointer.
         * 
         * @param [in] other Smart pointer to move the ownership from.
         */
        template <typename U>
        requires(!std::is_same_v<U, T>)
        ObjectUniquePtr(ObjectUniquePtr<U>&& other)
        {
            U* const otherObj = std::exchange(other.m_object, nullptr);

            if constexpr (std::is_assignable_v<T&, U&>)
            {
                // In case when objects are compatible (i.e. direct static_cast are available),
                // we can guarantee that there is no logical error in different types assignment and can accept nullptr.
                m_object = static_cast<T*>(otherObj);
            }
            else
            {
                NAU_FATAL(otherObj, "nullptr value is not acceptable with this constructor. Please check that 'other' is nullptr, before constructing ObjectUniquePtr<>");
                NAU_FATAL(otherObj->template is<T>(), "Object type is not compatible, required interface is not supported");

                m_object = otherObj->template as<T*>();
                NAU_FATAL(m_object);
            }
        }

        /**
         * @brief Copy-assignment operator (deleted).
         */
        ObjectUniquePtr& operator=(const ObjectUniquePtr&) = delete;

        ObjectUniquePtr& operator=(std::nullptr_t)
        {
            reset();
            return *this;
        }

        /**
         * @brief Move-assignment operator.
         * 
         * @param [in] other Smart pointer to move the ownership from.
         * 
         * @return              Resulted smart pointer.
         */
        ObjectUniquePtr& operator=(ObjectUniquePtr&& other)
        {
            T* const newObject = std::exchange(other.m_object, nullptr);
            reset(newObject);

            return *this;
        }

        /**
         * @brief Move-assignment operator.
         * 
         * @tparam U Type of the object managed by abother smart pointer.
         * 
         * @param [in] other    Smart pointer to move the ownership from.
         * 
         * @return              Resulted smart pointer.
         */
        template <typename U>
        requires(!std::is_same_v<U, T>)
        ObjectUniquePtr& operator=(ObjectUniquePtr<U>&& other)
        {
            NAU_FATAL(other.m_object);
            NAU_FATAL(other.m_object->template is<T>(), "Object type is not compatible, required interface not supported");
            U* const otherObj = std::exchange(other.m_object, nullptr);
            reset(otherObj->template as<T*>());

            return *this;
        }

        /**
         * @brief Checks whether the smart pointer manages an object.
         * 
         * @return `true` if the smart pointer manages an object (i.e. it can be safely derefenced), `false` otherwise.
         */
        explicit operator bool() const
        {
            return m_object != nullptr;
        }

        /**
         * @brief Provides access to the managed object.
         * 
         * @return A pointer to the managed object.
         */
        T* operator->() const
        {
            NAU_FATAL(m_object, "Dereferencing non existent object");
            return m_object;
        }

        /**
         * @brief Provides access to the managed object.
         * 
         * @return A reference to the managed object.
         */
        T& operator*() const
        {
            NAU_FATAL(m_object, "Dereferencing non existent object");
            return *m_object;
        }

        /**
         * @brief Provides access to the managed object.
         * 
         * @return A pointer to the managed object.
         */
        T* get() const
        {
            return m_object;
        }

        /**
         * @brief Constructs an ObjectWeakRef instance from the bound object.
         * 
         * @return Consturcted reference object.
         */
        ObjectWeakRef<T> getRef() const;

        /**
         * @brief Resets the pointer possibly binding it to a different object.
         *
         * @param [in] newObject    A pointer to the object to bind the smart pointer to. `NULL` can be passed to render the reference 'dangling'.
         */
        void reset(T* newObject = nullptr)
        {
            if (T* const oldObject = std::exchange(m_object, newObject); oldObject)
            {
                NauObject* const oldNauObject = castToNauObject(oldObject);
                NAU_FATAL(oldNauObject->m_hasPtrOwner);

                // NauObject::deleteObjectNow() will check that it has no owners.
                oldNauObject->setHasPtrOwner(false);
                oldNauObject->deleteObjectNow();
            }
        }

        /**
         * @brief Orders the pointer to give up exclusive ownership of the object.
         * 
         * @return A pointer to the managed (prior to the call) object.
         * 
         * @note Calling this renders the pointer 'dangling'. That means the smart pointer is no more responsible for the object destruction.
         */
        T* giveUp()
        {
            T* const object = std::exchange(m_object, nullptr);
            if (object)
            {
                NAU_FATAL(castToNauObject(object)->m_hasPtrOwner);
                castToNauObject(object)->setHasPtrOwner(false);
            }

            return object;
        }

    private:
        T* m_object = nullptr;

        template <typename>
        friend class ObjectUniquePtr;
    };

    template <typename T>
    ObjectUniquePtr(T*) -> ObjectUniquePtr<T>;

    /**
     * Encapsulates a weak reference (typed) to a NauObject instance.
     */
    template <typename T>
    class ObjectWeakRef : public scene_internal::ObjectWeakRefBase
    {
        using Base = scene_internal::ObjectWeakRefBase;

    public:

        /**
         * @brief Destructor.
         */
        ~ObjectWeakRef() = default;

        /**
         * @brief Default constructor.
         */
        ObjectWeakRef() = default;

        /**
         * @brief Initialization constructor.
         * 
         * @param [in] object Object to bind the reference to.
         * 
         * Binds the reference to **object**.
         */
        ObjectWeakRef(T& object)
        {
            reset(castToNauObject(&object));
        }

        /**
         * @brief Initialization constructor.
         * 
         * @param [in] object A pointer to the object to bind the reference to.
         * 
         * Binds the reference to **object**.
         */
        ObjectWeakRef(T* object)
        {
            NAU_ASSERT(object != nullptr, "Initializing a ObjectWeakRef with nullptr is considered an error");
            reset(castToNauObject(object));
        }

        /**
         * @brief Initialization constructor.
         * 
         * @tparam U Type of the object to bind the reference to.
         * 
         * @param [in] object Object to bind the reference to.
         * 
         * Binds the reference to **object**.
         */
        template <typename U>
        requires(!std::is_same_v<T, U> && std::is_base_of_v<IRttiObject, U>)
        ObjectWeakRef(U& object)
        {
            NAU_FATAL(object.template is<T>(), "Object ({}) has no required API ({})", rtti::getTypeInfo<U>().getTypeName(), rtti::getTypeInfo<T>().getTypeName());
            reset(castToNauObject(&object));
        }

        ObjectWeakRef(std::nullptr_t) :
            ObjectWeakRef{}
        {
        }

        /**
         * @brief Copy constructor.
         * 
         * @param [in] other Another reference.
         * 
         * Binds the reference to the object bound to **other**.
         */
        ObjectWeakRef(const ObjectWeakRef<T>& other) :
            Base(static_cast<const Base&>(other))
        {
        }

        /**
         * @brief Copy constructor.
         * 
         * @tparam U Type of the object bound to another reference.
         * 
         * @param [in] other Another reference.
         * 
         * Binds the reference to the object bound to **other**.
         */
        template <typename U>
        requires(!std::is_same_v<T, U>)
        ObjectWeakRef(const ObjectWeakRef<U>& other) :
            Base(static_cast<const Base&>(other))
        {
        }

        /**
         * @brief Assignment operator.
         * 
         * @param [in] object   Object to bind the reference to.
         * @return              Result reference object.
         * 
         * Binds the reference to **object**.
         */
        ObjectWeakRef& operator=(T& object)
        {
            reset(castToNauObject(&object));
            return *this;
        }

        /**
         * @brief Assignment operator.
         * 
         * @tparam U Type of the object to bind the reference to.
         * 
         * @param [in] object   Object to bind the reference to.
         * @return              Resulted reference object.
         * 
         * Binds the reference to **object**.
         */
        template <typename U>
        requires(!std::is_same_v<T, U> && std::is_base_of_v<IRttiObject, U>)
        ObjectWeakRef& operator=(U& object)
        {
            NAU_FATAL(object.template is<T>(), "Object ({}) has no required API ({})", rtti::getTypeInfo<U>().getTypeName(), rtti::getTypeInfo<T>().getTypeName());
            reset(castToNauObject(&object));
            return *this;
        }

        ObjectWeakRef& operator=(std::nullptr_t)
        {
            reset();
            return *this;
        }

        /**
         * @brief Assignment operator.
         * 
         * @param [in] other    Another reference.
         * @return              Resulted reference object.
         * 
         * Binds the reference to the object referenced by **other**.
         */
        ObjectWeakRef& operator=(const ObjectWeakRef<T>& other)
        {
            reset(other.getMutableNauObjectPtr());
            return *this;
        }

        /**
         * @brief Assignment operator.
         * 
         * @tparam U Type of the object to bind the reference to.
         * 
         * @param [in] other    Another reference.
         * @return              Resulted reference object.
         * 
         * Binds the reference to the object referenced by **other**.
         */
        template <typename U>
        requires(!std::is_same_v<T, U>)
        ObjectWeakRef& operator=(const ObjectWeakRef<U>& other)
        {
            if (!other)
            {
                reset(nullptr);
            }
            else
            {
                U* const otherObjectPtr = other.template getMutableTypedPtr<U>();
                NAU_FATAL(otherObjectPtr);

                T* const target = otherObjectPtr->template as<T*>();
                NAU_FATAL(target, "Assign incompatible objects");
                reset(castToNauObject(target));
            }

            return *this;
        }

        /**
         * @brief Checks if the refence is valid.
         * 
         * @return `true` if the reference is valid (i.e. an object can be obtained from it), `false` otherwise.
         */
        explicit operator bool() const
        {
            return this->refIsValid();
        }

        /**
         * @brief Provides non-const access to the referenced object.
         * 
         * @return A pointer to the referenced object.
         */
        T* operator->()
        {
            NAU_FATAL(this->refIsValid(), "Object ({}) reference is not valid", rtti::getTypeInfo<T>().getTypeName());
            return getMutableTypedPtr<T>();
        }

        /**
         * @brief Provides const access to the referenced object.
         * 
         * @return A pointer to the referenced object.
         */
        const T* operator->() const
        {
            NAU_FATAL(this->refIsValid(), "Object ({}) reference is not valid", rtti::getTypeInfo<T>().getTypeName());
            return getMutableTypedPtr<T>();
        }

        /**
         * @brief Provides non-const access to the referenced object.
         * 
         * @return A reference to the bound object.
         */
        T& operator*()
        {
            NAU_FATAL(this->refIsValid(), "Object ({}) reference is not valid", rtti::getTypeInfo<T>().getTypeName());
            return *getMutableTypedPtr<T>();
        }

        /**
         * @brief Provides const access to the referenced object.
         * 
         * @return A reference to the bound object.
         */
        const T& operator*() const
        {
            NAU_FATAL(this->refIsValid(), "Object ({}) reference is not valid", rtti::getTypeInfo<T>().getTypeName());
            return *getMutableTypedPtr<T>();
        }

        /**
         * @brief Provides non-const access to the referenced object.
         * 
         * @return A pointer to the referenced object.
         */
        T* get()
        {
            return getMutableTypedPtr<T>();
        }

        /**
         * @brief Provides const access to the referenced object.
         * 
         * @return A pointer to the referenced object.
         */
        const T* get() const
        {
            return getMutableTypedPtr<T>();
        }

        /**
         * @brief Checks whether two ObjectWeakRef instances reference the same object.
         * 
         * @param [in] other    Instance to compare this reference with.
         * @return              `true` if both references are bound to the same object.
         */
        template <typename U>
        bool operator==(const ObjectWeakRef<U>& other) const
        {
            return this->equals(other);
        }

    private:
    };

    template <typename T>
    ObjectWeakRef(T&) -> ObjectWeakRef<T>;

    template <typename T>
    ObjectWeakRef(const ObjectUniquePtr<T>&) -> ObjectWeakRef<T>;

    template <typename T>
    ObjectWeakRef(ObjectUniquePtr<T>&) -> ObjectWeakRef<T>;

    template <typename T>
    ObjectWeakRef(const ObjectWeakRef<T>&) -> ObjectWeakRef<T>;

    template <typename T>
    ObjectWeakRef(ObjectWeakRef<T>&) -> ObjectWeakRef<T>;

    template <typename T>
    ObjectWeakRef(ObjectWeakRef<T>&&) -> ObjectWeakRef<T>;

    template <typename T>
    ObjectWeakRef<T> ObjectUniquePtr<T>::getRef() const
    {
        NAU_ASSERT(m_object, "Attempting to reference to non existent object");
        return ObjectWeakRef<T>{*m_object};
    }

    /**
     */
    struct NAU_ABSTRACT_TYPE RuntimeObjectWeakRefValue : virtual RuntimeValue
    {
        NAU_INTERFACE(nau::scene::RuntimeObjectWeakRefValue, RuntimeValue)

        template <typename T = NauObject>
        ObjectWeakRef<T> getObjectWeakRef();

        template <typename T>
        void setObjectWeakRef(ObjectWeakRef<T> weakRef);

        virtual SceneQuery getObjectQuery() const = 0;

        virtual bool isAssignable(const NauObject& object) const = 0;

    protected:
        virtual ObjectWeakRef<> getObjectWeakRefInternal() = 0;

        virtual void setObjectWeakRefInternal(ObjectWeakRef<> weakRef) = 0;
    };

    template <typename T>
    ObjectWeakRef<T> RuntimeObjectWeakRefValue::getObjectWeakRef()
    {
        auto weakRef = getObjectWeakRefInternal();
        if (weakRef)
        {
            return ObjectWeakRef<T>{weakRef};
        }

        return nullptr;
    }

    template <typename T>
    void RuntimeObjectWeakRefValue::setObjectWeakRef(ObjectWeakRef<T> weakRef)
    {
        setObjectWeakRefInternal(weakRef);
    }

    template <typename U>
    inline NauObject* castToNauObject(U* object)
    {
        if constexpr (std::is_convertible_v<U*, NauObject*>)
        {
            return object;
        }
        else
        {
            static_assert(std::is_base_of_v<IRttiObject, U>);
            NauObject* const nauObj = object->template as<NauObject*>();
            NAU_FATAL(nauObj, "Type () MUST BE a NauObject", rtti::getTypeInfo<U>().getTypeName());
            return nauObj;
        }
    }

    template <typename U>
    inline const NauObject* castToNauObject(const U* object)
    {
        if constexpr (std::is_convertible_v<U*, NauObject*>)
        {
            return object;
        }
        else
        {
            static_assert(std::is_base_of_v<IRttiObject, U>);
            const NauObject* const nauObj = object->template as<const NauObject*>();
            NAU_FATAL(nauObj, "Type () MUST BE a NauObject", rtti::getTypeInfo<U>().getTypeName());
            return nauObj;
        }
    }

}  // namespace nau::scene

namespace nau::scene_internal
{
    template <typename T>
    inline T* ObjectWeakRefBase::getMutableTypedPtr() const
    {
        if constexpr (std::is_convertible_v<T*, scene::NauObject*>)
        {
            return static_cast<T*>(m_object);
        }
        else
        {
            if (!m_object)
            {
                return nullptr;
            }

            T* const target = m_object->template as<T*>();
            NAU_FATAL(target, "Object has no requested API ({})", rtti::getTypeInfo<T>().getTypeName());

            return target;
        }
    }

    /**
     */
    class NAU_CORESCENE_EXPORT RuntimeObjectWeakRefValueImpl final : public ser_detail::NativePrimitiveRuntimeValueBase<scene::RuntimeObjectWeakRefValue>,
                                                                     public RuntimeStringValue
    {
        using Base = ser_detail::NativePrimitiveRuntimeValueBase<scene::RuntimeObjectWeakRefValue>;

        NAU_CLASS_(nau::scene_internal::RuntimeObjectWeakRefValueImpl, Base, RuntimeStringValue)

    public:
        using CopyCtorTag = TypeTag<int>;

        ~RuntimeObjectWeakRefValueImpl();
        RuntimeObjectWeakRefValueImpl();
        explicit RuntimeObjectWeakRefValueImpl(const scene_internal::ObjectWeakRefBase&, CopyCtorTag);
        explicit RuntimeObjectWeakRefValueImpl(const scene_internal::ObjectWeakRefBase&);
        explicit RuntimeObjectWeakRefValueImpl(scene_internal::ObjectWeakRefBase&);
        RuntimeObjectWeakRefValueImpl(const RuntimeObjectWeakRefValue&) = delete;

        RuntimeObjectWeakRefValueImpl& operator=(const RuntimeObjectWeakRefValueImpl&) = delete;

        bool isMutable() const override;

        Result<> setString(std::string_view str) override;

        std::string getString() const override;

    private:
        using WeakRefStorage = std::aligned_storage_t<sizeof(scene_internal::ObjectWeakRefBase), alignof(scene_internal::ObjectWeakRefBase)>;

        static scene_internal::ObjectWeakRefBase* allocateInternalStorage(WeakRefStorage&, const scene_internal::ObjectWeakRefBase&);

        scene::SceneQuery getObjectQuery() const override;

        bool isAssignable(const scene::NauObject& object) const override;

        scene::ObjectWeakRef<> getObjectWeakRefInternal() override;

        void setObjectWeakRefInternal(scene::ObjectWeakRef<> weakRef) override;

        const bool m_isMutable;
        WeakRefStorage m_weakRefStorage;
        scene_internal::ObjectWeakRefBase* const m_weakRef;
    };

}  // namespace nau::scene_internal

namespace nau::scene
{
    template <typename T>
    RuntimeValue::Ptr makeValueRef(ObjectWeakRef<T>& objectRef, IMemAllocator::Ptr allocator = nullptr)
    {
        using Type = scene_internal::RuntimeObjectWeakRefValueImpl;
        return rtti::createInstanceWithAllocator<Type>(std::move(allocator), objectRef);
    }

    template <typename T>
    RuntimeValue::Ptr makeValueRef(const ObjectWeakRef<T>& objectRef, IMemAllocator::Ptr allocator = nullptr)
    {
        using Type = scene_internal::RuntimeObjectWeakRefValueImpl;
        return rtti::createInstanceWithAllocator<Type>(std::move(allocator), objectRef);
    }

    template <typename T>
    RuntimeValue::Ptr makeValueCopy(const ObjectWeakRef<T>& objectRef, IMemAllocator::Ptr allocator = nullptr)
    {
        using Type = scene_internal::RuntimeObjectWeakRefValueImpl;
        using CopyCtorTag = Type::CopyCtorTag;

        return rtti::createInstanceWithAllocator<Type>(std::move(allocator), objectRef, CopyCtorTag{});
    }

}  // namespace nau::scene

#define NAU_OBJECT(TypeName, ...) \
    NAU_TYPEID(TypeName)          \
    NAU_CLASS_BASE(__VA_ARGS__)   \
    NAU_IMPLEMENT_RTTI_OBJECT\
