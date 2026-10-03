// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#pragma once

#include <util/dag_globDef.h>

#include <algorithm>
#include <type_traits>
#include <utility>
template <typename T, bool bConst>
struct NullableComponent
{
    typedef typename std::conditional<bConst, const T, T>::type T_const_type;
    typedef const T* cptr_type;
    typedef const T& cref_type;
    typedef T_const_type* ptr_type;
    typedef T_const_type& ref_type;

    NullableComponent() = default;
    explicit NullableComponent(ptr_type a) :
        attr(a)
    {
    }
    template <typename U = T, typename = std::enable_if_t<!bConst, void>>
    U& get()
    {
        NAU_ASSERT(attr);
        return *attr;
    }
    template <typename U = T, typename = std::enable_if_t<!bConst, void>>
    ref_type operator*()
    {
        return get();
    }
    template <typename U = T, typename = std::enable_if_t<!bConst, void>>
    ptr_type operator->()
    {
        NAU_ASSERT(attr);
        return attr;
    }
    template <typename U = T, typename = std::enable_if_t<!bConst, void>>
    NullableComponent& operator=(const T& a)
    {
        NAU_ASSERT_RETURN(attr, *this);
        *attr = a;
        return *this;
    }

  cref_type &get() const
  {
    NAU_ASSERT(attr);
    return *attr;
  }
  cref_type operator*() const { return get(); }
  cptr_type operator->() const
  {
    NAU_ASSERT(attr);
    return attr;
  }
  bool isValid() const { return attr != nullptr; }

  template <typename U = T, typename = std::enable_if_t<std::is_rvalue_reference<U&&>::value && !bConst, void>>
  NullableComponent& operator=(U&& a)
  {
    NAU_ASSERT_RETURN(attr, *this);
    *attr = std::move(a);
    return *this;
  }
  NullableComponent(NullableComponent &&a)
  {
    attr = a.attr;
    a.attr = nullptr;
  }

  NullableComponent(const NullableComponent &) = delete;
  NullableComponent &operator=(const NullableComponent &) = delete;
  NullableComponent &operator=(NullableComponent &&) = delete;

protected:
  ptr_type attr = nullptr;
};

template <class T>
using NullableRO = NullableComponent<T, true>;
template <class T>
using NullableRW = NullableComponent<T, false>;
