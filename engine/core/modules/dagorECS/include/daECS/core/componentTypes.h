// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#pragma once

#include <daECS/core/baseComponentTypes/arrayType.h>
#include <daECS/core/baseComponentTypes/listType.h>
#include <daECS/core/baseComponentTypes/objectType.h>
#include <daECS/core/componentType.h>
#include <daECS/core/entityId.h>
#include <nau/math/dag_e3dColor.h>
#include <nau/math/math.h>
#include <nau/math/transform.h>

#include <string>       //for ecs::string type
#include <type_traits>  //for true type

// all basic types
ECS_DECLARE_TYPE(bool);
ECS_DECLARE_TYPE_ALIAS(std::true_type, bool);
ECS_DECLARE_TYPE_ALIAS(std::false_type, bool);
ECS_DECLARE_TYPE(uint8_t);
ECS_DECLARE_TYPE(uint16_t);
ECS_DECLARE_TYPE(uint32_t);
ECS_DECLARE_TYPE(uint64_t);
ECS_DECLARE_TYPE(int8_t);
ECS_DECLARE_TYPE(int16_t);
ECS_DECLARE_TYPE(int);
ECS_DECLARE_TYPE(int64_t);
ECS_DECLARE_TYPE(float);
ECS_DECLARE_TYPE(nau::math::Point2);
ECS_DECLARE_TYPE(nau::math::Point3);
ECS_DECLARE_TYPE(nau::math::Vector4d);  // Point4
ECS_DECLARE_TYPE(nau::math::IVector2);  // IPoint2
ECS_DECLARE_TYPE(nau::math::IVector3);  // IPoint3
ECS_DECLARE_TYPE(nau::math::IVector4);  // IPoint4
ECS_DECLARE_TYPE(nau::math::Vector3d);  //  DPoint3
ECS_DECLARE_TYPE(nau::math::Vector3);
ECS_DECLARE_TYPE(nau::math::Quat);
ECS_DECLARE_TYPE(nau::math::Matrix4d);  // TMatrix
ECS_DECLARE_TYPE(nau::math::Vector4);  //vec4f
ECS_DECLARE_TYPE(nau::math::Matrix4);  // mat44f
ECS_DECLARE_TYPE(nau::math::Transform);  // mat44f

//ECS_DECLARE_TYPE(Vectormath::SSE::IPoint2);  // IPoint2
//ECS_DECLARE_TYPE(Vectormath::SSE::bbox3f);   // bbox3f
ECS_DECLARE_TYPE(nau::math::E3DCOLOR);

ECS_DECLARE_BOXED_TYPE(ecs::Array);
ECS_DECLARE_BOXED_TYPE(ecs::Object);

#define ECS_DECL_LIST_TYPES                 \
    DECL_LIST_TYPE(UInt8List, uint8_t)      \
    DECL_LIST_TYPE(UInt16List, uint16_t)    \
    DECL_LIST_TYPE(UInt32List, uint32_t)    \
    DECL_LIST_TYPE(UInt64List, uint64_t)    \
    DECL_LIST_TYPE(StringList, ecs::string) \
    DECL_LIST_TYPE(EidList, ecs::EntityId)  \
    DECL_LIST_TYPE(FloatList, float)        \
    DECL_LIST_TYPE(BoolList, bool)          \
    DECL_LIST_TYPE(Int8List, int8_t)        \
    DECL_LIST_TYPE(Int16List, int16_t)      \
    DECL_LIST_TYPE(IntList, int)            \
    DECL_LIST_TYPE(Int64List, int64_t)      \
    DECL_LIST_TYPE(Point2List, nau::math::Point2)      \
    DECL_LIST_TYPE(Point3List, nau::math::Point3)      \
    DECL_LIST_TYPE(Point4List, nau::math::Vector4)     \
    DECL_LIST_TYPE(Vector3List, nau::math::Vector3)    \
    DECL_LIST_TYPE(IPoint2List, nau::math::IVector2)   \
    DECL_LIST_TYPE(IPoint3List, nau::math::IVector3)   \
    DECL_LIST_TYPE(IPoint4List, nau::math::IVector4)   \
    DECL_LIST_TYPE(TMatrixList, nau::math::Matrix4d)   \
    DECL_LIST_TYPE(TransformList, nau::math::Transform)   \
    DECL_LIST_TYPE(ColorList, nau::math::E3DCOLOR)

//DECL_LIST_TYPE(IPoint2List, Vectormath::SSE::IPoint2)

namespace ecs
{
#define DECL_LIST_TYPE(type_alias, T) using type_alias = ecs::List<T>;
    ECS_DECL_LIST_TYPES
#undef DECL_LIST_TYPE
}  // namespace ecs

#define DECL_LIST_TYPE(type_alias, T) ECS_DECLARE_BOXED_TYPE(ecs::type_alias);
ECS_DECL_LIST_TYPES
#undef DECL_LIST_TYPE
