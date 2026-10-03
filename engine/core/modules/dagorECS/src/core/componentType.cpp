// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#include <daECS/core/componentType.h>
#include <daECS/utility/ecs_asserts.h>

#include <type_traits>

namespace ecs
{
CompileComponentTypeRegister *CompileComponentTypeRegister::tail = 0;
type_index_t ComponentTypes::registerType(const char *name, component_type_t type, uint16_t data_size, ComponentSerializer *io,
  ComponentTypeFlags flags, create_ctm_t ctm, destroy_ctm_t dtm, void *user_data)
{
  const type_index_t ctypeId = findType(type);
  if (ctypeId != INVALID_COMPONENT_TYPE_INDEX)
  {
    if (strcmp(name, getTypeNameById(ctypeId)) != 0)
    {
      logerr("component type <{}> with same hash =0x{} as <{}> is already registered, hash collision.", getTypeNameById(ctypeId), type,
        name);
      return INVALID_COMPONENT_TYPE_INDEX;
    }
    // This is not severe error per se but without logger developers are not noticing it during development
    logerr("ecs type <{}>(%#x) is already registered", name, type);
    return ctypeId;
  }
  NAU_ASSERT_RETURN(getTypeCount() < INVALID_COMPONENT_TYPE_INDEX - 1, INVALID_COMPONENT_TYPE_INDEX);
  if (ctm)
    flags = ComponentTypeFlags(flags | COMPONENT_TYPE_NON_TRIVIAL_CREATE);
  if((flags & (COMPONENT_TYPE_NON_TRIVIAL_CREATE | COMPONENT_TYPE_IS_POD)) ==
     (COMPONENT_TYPE_NON_TRIVIAL_CREATE | COMPONENT_TYPE_IS_POD))
  {
      logerr("ecs type <{}> can be declared as pod and has creator in same time", name);
  }
  flags = ComponentTypeFlags(flags & ~COMPONENT_TYPE_IS_POD);
  if (flags & ecs::COMPONENT_TYPE_BOXED)
  {
    NAU_ASSERT((flags & (COMPONENT_TYPE_BOXED | COMPONENT_TYPE_NON_TRIVIAL_CREATE)) ==
                (COMPONENT_TYPE_BOXED | COMPONENT_TYPE_NON_TRIVIAL_CREATE),
      "all boxed types should be creatable, inspect <{}>", name);
    flags = ComponentTypeFlags(
        std::underlying_type_t<ComponentTypeFlags>(flags) | COMPONENT_TYPE_BOXED | COMPONENT_TYPE_NON_TRIVIAL_CREATE);
  }
  if (io)
    flags = ComponentTypeFlags(flags | COMPONENT_TYPE_HAS_IO);
  typesIndex.emplace(type, getTypeCount());
  types.push_back(io, type, ComponentType{(uint16_t)data_size, flags}, nullptr, user_data, name, ctm, dtm);
  NAU_ASSERT(!(flags & ecs::COMPONENT_TYPE_NON_TRIVIAL_MOVE),
    "currently non trivially moveable types are not supported <{}>,"
    " declare as relocatable(ECS_DECLARE_RELOCATABLE_TYPE), if you are sure that it can be moved with memcpy"
    " or boxed (ECS_DECLARE_BOXED_TYPE) otherwise",
    name);

  ECS_VERBOSE_LOG("create {} ecs {} {}{}{}{}type <{}> hash<0x{}> of size {} (flags={})", getTypeCount() - 1,
    (flags & ecs::COMPONENT_TYPE_BOXED) ? "boxed" : "data", (flags & ecs::COMPONENT_TYPE_NON_TRIVIAL_MOVE) ? "hard_moveable " : "",
    (flags & ecs::COMPONENT_TYPE_NON_TRIVIAL_CREATE) ? "createable " : "",
    (flags & ecs::COMPONENT_TYPE_NEED_RESOURCES) ? "need_resources " : "", (flags & ecs::COMPONENT_TYPE_HAS_IO) ? "io " : "", name,
    type, data_size, flags);
  return getTypeCount() - 1;
}
void ComponentTypes::clear()
{
  for (int i = 0; i < types.size(); ++i)
  {
    types.get<create_ctm_t>()[i] = nullptr; // to prevent re-creation of type
    if (!types.get<ComponentTypeManager *>()[i])
      continue;
    if (types.get<destroy_ctm_t>()[i])
    {
      ComponentTypeManager *ctm = types.get<ComponentTypeManager *>()[i];
      types.get<ComponentTypeManager *>()[i] = nullptr;
      (*(types.get<destroy_ctm_t>()[i]))(ctm);
    }
  }
  types.clear();
}
void ComponentTypes::initialize()
{
  clear();
  ECS_LOG("ecs: initialize component Types");
  // initialize eid and tag first, for debugging purposes?
  for (CompileComponentTypeRegister *start = CompileComponentTypeRegister::tail; start; start = start->next)
    registerType(start->name, start->name_hash, start->size, start->io, start->flags, start->ctm, start->dtm);
}
}; // namespace ecs
