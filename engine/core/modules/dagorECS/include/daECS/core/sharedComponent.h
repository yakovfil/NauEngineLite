// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#pragma once

#include <daECS/core/componentTypes.h>
#include <daECS/core/componentsMap.h>

#include <cstdlib>
#include <memory>
#include <type_traits>
#include <utility>

#define ECS_DECLARE_SHARED_TYPE(klass) ECS_DECLARE_BOXED_TYPE(ecs::SharedComponent<klass>)

#define ECS_REGISTER_SHARED_TYPE(klass, io) \
  ECS_REGISTER_MANAGED_TYPE(ecs::SharedComponent<klass>, io, ecs::SharedComponentCreator<klass>)

#if _ECS_CODEGEN
#define ECS_SHARED(type) ecs::SharedComponent<type>
#else
#define ECS_SHARED(type) const type &
#endif

// usage:
//  ECS_DECLARE_SHARED_TYPE(ecs::Object);
//  ECS_REGISTER_SHARED_TYPE(ecs::Object, nullptr);
//  then in ES you can use const ecs::SharedComponent<ecs::Object> & or
//  EntityManager::get<ecs::SharedComponent<ecs::Object>>
//  this component is shared between all data of one template, and can be used as replacement to Props
//  as soon as we will support it in IO.
//  usage ECS_SHARED(ecs::Object) props (will be visible as const ecs::Object& props)
//  todo: support in IO (reading from BLK)


namespace ecs
{

template <class T>
struct SharedComponentCreator;

template <class T>
class SharedComponent : public std::shared_ptr<T>
{
public:
    operator const T&() const
    {
        auto vptr = base_type::get();
        DAECS_EXT_FAST_ASSERT(vptr != nullptr);
        return *vptr;
    }
    SharedComponent(const T& v) :
        base_type(std::make_shared<T>(v))
    {
    }
    SharedComponent(T&& v) :
        base_type(std::make_shared<T>(std::move(v)))
    {
    }
    template <typename U, typename = std::enable_if_t<std::is_rvalue_reference<U&&>::value, void>>
    SharedComponent(U&& v) :
        base_type(std::make_shared<U>(std::move(v)))
    {
    }
    SharedComponent(SharedComponent&& v) :
        base_type(std::move(v))
    {
    }
  SharedComponent &operator=(SharedComponent &&v)
  {
      (base_type&)(*this) = std::move(v);
      return *this;
  }

protected:
  friend struct SharedComponentCreator<T>;
  typedef std::shared_ptr<T> base_type;
  SharedComponent(const SharedComponent &sharedPtr) : base_type(sharedPtr) {}
  SharedComponent(T *initial) : base_type(initial) {}
  SharedComponent() {}
};

NAU_DAGORECS_EXPORT EntityManager &get_entity_mgr();

template <class T>
struct SharedComponentCreator final : public ComponentTypeManager
{
  typedef T component_type;
  typedef SharedComponent<T> shared_type;

  static shared_type* value(void* p)
  {
      return *static_cast<shared_type**>(p);
  }
  static const shared_type* value(const void* p)
  {
      return *static_cast<shared_type* const*>(p);
  }
  static shared_type* allocate(void* p)
  {
      auto* result = static_cast<shared_type*>(std::malloc(sizeof(shared_type)));
      *static_cast<shared_type**>(p) = result;
      return result;
  }

  bool copy(void *to, const void *from, ecs::component_index_t index, ecs::EntityId to_eid) const override
  {
    NAU_ASSERT_RETURN(from && to, false);
    if (*value(from) || !to_eid)
        new(allocate(to)) shared_type(*value(from));
    else // first copy
    {
        T* data = std::allocator<T>{}.allocate(1);
        const_cast<shared_type*>(value(from))->reset(data, [](T* value)
        {
            std::destroy_at(value);
            std::allocator<T>{}.deallocate(value, 1);
        });
        new(allocate(to)) shared_type(*value(from));
        // Note: Make sure that actual comp construction called after ptr creation (as ctor might want to access it)
        construct(data, get_entity_mgr(), to_eid, index);
    }
    return true;
  }
  bool is_equal(const void *to, const void *from) const override
  {
    NAU_ASSERT_RETURN(from && to, true);
    if (!value(to)->get() && !value(from)->get())  // equal
        return true;
    if (!value(to)->get() || !value(from)->get())  // not equal
        return false;
    return ConstructInplaceType<T>::is_equal(value(to)->get(), value(from)->get());
  }
  bool assign(void *to, const void *from) const override
  {
    NAU_ASSERT_RETURN(from && to, false);
    NAU_ASSERT_RETURN(value(to)->get() && value(from)->get(), false);
    return ConstructInplaceType<T>::assign(value(to)->get(), value(from)->get());
  }
  void create(void *p, EntityManager &mgr, EntityId eid, const ComponentsMap &, ecs::component_index_t index) override
  {
      new(allocate(p)) shared_type;
      if (!eid)
          return;

      T* data = std::allocator<T>{}.allocate(1);
      construct(data, mgr, eid, index);
      value(p)->reset(data, [](T* value)
      {
          std::destroy_at(value);
          std::allocator<T>{}.deallocate(value, 1);
      });
  }

  void destroy(void *p) override
  {
    NAU_ASSERT_RETURN(p, );
    value(p)->~shared_type();
    std::free(value(p));
    *static_cast<shared_type**>(p) = nullptr;
  } // memset is not needed, we keep it for debug purposes

  bool replicateCompare(void *to, const void *from) const override
  {
    NAU_ASSERT_RETURN(from && to, false);
    NAU_ASSERT_RETURN(value(to)->get() && value(from)->get(), false);
    return ConstructInplaceType<T>::replicateCompare(value(to)->get(), value(from)->get());
  }
  void clear() override { ConstructInplaceType<T>::onTypeManagerClear(); }

private:
  void construct(T *data, EntityManager &mgr, EntityId eid, ecs::component_index_t index) const
  {
    ConstructInplaceType<T>::create(data, mgr, eid, ComponentsMap(), index);
  }
};
} // namespace ecs

// Keep stable type names while moving only boxed pointers in ECS storage.
#define ECS_DECLARE_BOXED_SHARED_ALIAS(type, name) ECS_DECLARE_BASE_TYPE(type, name, true, false, false)

// Digraphs-frienly aliases
ECS_DECLARE_BOXED_SHARED_ALIAS(ecs::SharedComponent<ecs::Object>, "ecs::SharedComponent< ::ecs::Object>");
ECS_DECLARE_BOXED_SHARED_ALIAS(ecs::SharedComponent<ecs::Array>, "ecs::SharedComponent< ::ecs::Array>");
ECS_DECLARE_BOXED_SHARED_ALIAS(ecs::SharedComponent<ecs::string>, "ecs::SharedComponent< ::ecs::string>");

#define DECL_LIST_TYPE(lt, t) \
    ECS_DECLARE_BOXED_SHARED_ALIAS(ecs::SharedComponent<ecs::lt>, "ecs::SharedComponent< ::ecs::" #lt ">");
ECS_DECL_LIST_TYPES
#undef DECL_LIST_TYPE
