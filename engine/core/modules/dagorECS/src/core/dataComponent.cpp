// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#include <daECS/core/dataComponent.h>

#include <string>
#include <vector>

namespace ecs
{
CompileComponentRegister *CompileComponentRegister::tail = 0;
LTComponentList *LTComponentList::tail = 0;
void DataComponents::updateComponentToLT()
{
  if (lastSeenComponentLT == LTComponentList::tail)
  {
    return;
  }
  lastSeenComponentLT = LTComponentList::tail;
  for (LTComponentList *lt = LTComponentList::tail, *next = NULL; lt; lt = next)
  {
      next = lt->next;
      auto ltMap = componentToLT.find(lt->name);

    if(ltMap == componentToLT.end())
    {
        componentToLT.emplace(lt->name, lt);
    }
    //{
    //  auto oldNext = ltMap->second->next;
    //    ltMap->second->next = lt;
    //  lt->next = oldNext;
    //}
    //else
  }
  //LTComponentList::tail = NULL;
}

component_index_t DataComponents::createComponent(const HashedConstString name_, type_index_t component_type,
  nau::Span<component_t> deps, ComponentSerializer *io, component_flags_t flags, const ComponentTypes &types)
{
  if (isFilteredOutComponent(name_.hash))
  {
    ECS_VERBOSE_LOG("data component <{}|0x{}> is filtered out", name_.str, name_.hash);
    return INVALID_COMPONENT_INDEX;
  }
  const bool isCopy = (flags & DataComponent::IS_COPY) ? true : false;
  component_t nameHash = name_.hash;
  const char *nameStr = name_.str;
  if (isCopy)
    nameHash = ecs_mem_hash<sizeof(ecs::component_t) * 8>("$", 1, nameHash); // same as string+$
  component_index_t existingId = findComponentId(nameHash);

  type_index_t existingType = INVALID_COMPONENT_TYPE_INDEX;

  if (NAU_LIKELY(existingId != INVALID_COMPONENT_INDEX))
  {
    existingType = getComponentById(existingId).componentType;
    const uint32_t oldNameAddr = components.get<NAME>()[existingId];
    if (nameStr != nullptr && oldNameAddr != 0 && strcmp(nameStr, names.getDataRawUnsafe(oldNameAddr)) != 0) //-V522
    {
      logerr("component <{}> with same hash =0x{} as <{}> is already registered, hash collision.", getComponentNameById(existingId),
        nameHash, nameStr);
      return INVALID_COMPONENT_INDEX;
    }
    if (component_type != existingType)
    {
      if (component_type != INVALID_COMPONENT_TYPE_INDEX)
      {
        logerr("component <{}>(0x{}) with type <{}>({}) is already registered with different type <{}>({})!", nameStr, nameHash,
          types.getTypeNameById(component_type), component_type, types.getTypeNameById(getComponentById(existingId).componentType),
          getComponentById(existingId).componentType);
        return INVALID_COMPONENT_INDEX;
      }
      else
        component_type = existingType;
    }
    if (oldNameAddr == 0 && nameStr != nullptr)
      components.get<NAME>()[existingId] = names.addDataRaw(nameStr, strlen(nameStr) + 1); // replace nullptr name with
    return existingId;
  }

  updateComponentToLT();

  const char *usedName = nameStr;

  if (isCopy)
  {
    flags |= DataComponent::DONT_REPLICATE; // we don't replicate copies
    const component_t baseNameHash = name_.hash;
    const component_index_t base = findComponentId(baseNameHash);
    if (base == INVALID_COMPONENT_INDEX)
    {
      logerr("can't create copy for unknown component <{}|0x{}>", usedName, baseNameHash);
      return INVALID_COMPONENT_INDEX;
    }
    const type_index_t baseComponentType = getComponentById(base).componentType;
    if (baseComponentType != component_type && component_type != INVALID_COMPONENT_TYPE_INDEX)
    {
      logerr("can't create copy for component <{}|0x{}> because of type base={} != copy={}", usedName, baseNameHash,
        types.getTypeNameById(baseComponentType), types.getTypeNameById(component_type));
      return INVALID_COMPONENT_INDEX;
    }
    component_type = baseComponentType;

    NAU_ASSERT(components.get<component_index_t>()[base] == INVALID_COMPONENT_INDEX);
    components.get<component_index_t>()[base] = components.size();
    usedName = getComponentNameById(base);
    ECS_VERBOSE_LOG(" {} ecs component <{}> hash<0x{}> of component_type {}<{}> is a copy of {} ({}|0x{})", components.size(),
      usedName, nameHash, component_type, types.getTypeNameById(component_type), base, usedName, baseNameHash);

    auto itLt = componentToLT.find(baseNameHash);
    for(LTComponentList* lt = itLt != componentToLT.end() ? itLt->second : nullptr; lt; lt = lt->next)
    {
        lt->info.canTrack = true;
    }

#if NAU_DEBUG
    if(types.getTypeInfo(component_type).size == 0)
    {
        logerr(" {} ecs component <{}> hash<0x{}> of component_type {}<{}> is a copy of 0 size", components.size(), nameStr, nameHash,
               component_type, types.getTypeNameById(component_type));
    }
#endif
  }
  if (component_type == INVALID_COMPONENT_TYPE_INDEX)
  {
#if NAU_DEBUG
    logerr("can't create component {}|0x{} with undefined type", nameStr, name_.hash);
#endif
    return INVALID_COMPONENT_INDEX;
  }
  if (types.getTypeInfo(component_type).flags & COMPONENT_TYPE_NON_TRIVIAL_CREATE)
    flags |= DataComponent::TYPE_HAS_CONSTRUCTOR;
  component_type_t component_type_name = types.getTypeById(component_type);
  if (component_type_name == ComponentTypeInfo<Tag>::type)
    flags |= DataComponent::DONT_REPLICATE;

  if (!isCopy)
  {
      auto itLt = componentToLT.find(nameHash);
      for(LTComponentList* lt = itLt != componentToLT.end() ? itLt->second : nullptr; lt; lt = lt->next)
    {
      if (lt->name != nameHash){
          continue;
      }
#if NAU_DEBUG
      if (lt->nameStr && nameStr && strcmp(lt->nameStr, nameStr) != 0)
      {
        logerr("hash collision for component <{}> nameStr = {} defined at file <{}>, line {}", lt->nameStr, nameStr, lt->fileStr,
          lt->line);
        lt->info.valid = false;
        continue;
      }
      if(lt->type != component_type_name)
      {
          logerr("expected type of component <{}> in get at file <{}>, line {} is <{}|0x{}> registered is <{}|0x{}>", nameStr,
                 lt->fileStr, lt->line, types.getTypeNameById(lt->type), lt->type, types.getTypeNameById(component_type),
                 component_type_name);
      }
#endif
      if (lt->type != component_type_name)
      {
        lt->info.valid = false; // incorrect type, it can never become correct!
        continue;
      }
      lt->info.cidx = components.size();
      lt->info.canTrack = false; // copies are only created after base components, so currently unknown
    }
  }

  if (io)
  {
    const ComponentTypeFlags typeFlags = types.getTypeInfo(component_type).flags;
    if (!is_pod(typeFlags) && !has_io(typeFlags))
    {
        logerr(
            "component <{}> hash<0x{}> of component_type {}<{}|0x{}> is registered with io, while it's type is not serializable."
            "\nThis can be easily fixed, as it is issue only with recreation, (de)serialize_comp_nameless.",
            nameStr, nameHash, component_type, types.getTypeNameById(component_type), component_type_name);
    }
    else
    {
        flags |= DataComponent::HAS_SERIALIZER;
    }
  }
  NAU_ASSERT_RETURN(components.size() < INVALID_COMPONENT_INDEX - 1, INVALID_COMPONENT_INDEX);
  componentIndex[nameHash] = components.size();
  const uint32_t nameAddr = usedName ? names.addDataRaw(usedName, strlen(usedName) + 1) : 0;
  components.push_back(DataComponent{component_type, flags, component_type_name}, io, nameHash, INVALID_COMPONENT_INDEX,
                       DependencyRange{(uint32_t)dependencies.size()}, nameAddr);

  for (const auto dependency : deps)
      dependencies.push_back(dependency);

  ECS_VERBOSE_LOG("create {} ecs component <{}> hash<0x{}> of component_type {}<{}|0x{}> flags={}", components.size() - 1, usedName,
    nameHash, component_type, types.getTypeNameById(component_type), component_type_name, flags);
  NAU_UNUSED(types);
  return components.size() - 1;
}

void DataComponents::clear()
{
  componentIndex.clear();
  components.clear();
  dependencies.clear();
  names.clear();
  NAU_ASSERT(names.addDataRaw("", 1) == 0);
}
const char *DataComponents::getHashName(component_index_t id) const
{
  static char buf[16];
  snprintf(buf, sizeof(buf), "#%X", components.get<COMPONENT>()[id]);
  return buf;
}

void DataComponents::initialize(const ComponentTypes &types)
{
  clear();
  // componentIndex.set_empty_key(0);
  // it is essential to have eid as a first component index
  createComponent(ECS_HASH("eid"), types.findType(ECS_HASH("ecs::EntityId").hash), nau::Span<component_t>(), nullptr, 0, types);
  // roots
  std::vector<CompileComponentRegister* /*, framemem_allocator TODO Allocators.*/> childs;
  uint32_t max_size = 0;
  for (CompileComponentRegister *start = CompileComponentRegister::tail; start; start = start->next)
  {
    NAU_ASSERT(ECS_HASH_SLOW(start->type_name).hash == start->type,
      "data component <{}> has type of <{}> but it's typeid is different <0x{} != 0x{}>", start->name.str, start->type_name,
      ECS_HASH_SLOW(start->type_name).hash, start->type);
    const char *typeName = types.findTypeName(start->type);
    if (!typeName)
    {
      logerr("data component <{}> is registered with type of <{}>, which can not be found in registered types."
             " Potential reasons: you forgot to REGISTER the type (type is only declared),"
             " or the module with registration was excluded by linker, due to know cohesion."
             " In that case try to pull some variable from module",
        start->name.str, start->type_name);
      continue;
    }
    else
    {
      if (strcmp(start->type_name, typeName) != 0)
      {
        logerr("data component <{}> has type of <{}> but it's registered typeName <{}> is different. Hash collision?", start->name.str,
          start->type_name, typeName);
        continue;
      }
    }
    if (filterTags.find(start->name.hash) != filterTags.end())
    {
      ECS_VERBOSE_LOG("data component <{}> is filtered with tag", start->name.str);
      continue;
    }
    if (!start->deps.size())
      createComponent(start->name, types.findType(start->type), nau::Span<component_t>(), start->io, start->flags, types);
    else
    {
      childs.push_back(start);
      max_size = std::max(max_size, (uint32_t)start->deps.size());
    }
  }

  std::vector<component_t /*, framemem_allocator  TODO Allocators.*/> deps_components;
  deps_components.reserve(max_size);
  auto tryAddComponent = [&deps_components, this, &types](auto &childs, bool can_skip_optional) -> bool {
    bool somethingAdded = false;
    for (size_t i = 0; i != childs.size();)
    {
        CompileComponentRegister* start = childs[i];
        int dep;
        deps_components.resize(0);
        bool allDependendsExist = true;
        for (dep = 0; dep < start->deps.size(); ++dep)
        {
            const char* depName = start->deps[dep];
            bool optional = false;
            if (*depName == '?')
            {
                optional = true;
                depName++;
            }
            HashedConstString depConstString = ECS_HASH_SLOW(depName);
            if (filterTags.find(depConstString.hash) == filterTags.end())
            {
                if (!this->hasComponent(depConstString.hash) && (!optional || (optional && !can_skip_optional)))
                {
                    allDependendsExist = false;
                    break;
                }
                if (!optional)
                    deps_components.push_back(depConstString.hash);
            }
      }
      if (allDependendsExist)
      {
        this->createComponent(start->name, types.findType(start->type), deps_components, start->io, start->flags, types);
        childs[i] = childs.back();
        childs.pop_back();
        somethingAdded = true;
      }
      else
          ++i;
    }
    return somethingAdded;
  };

  for (; childs.size() && tryAddComponent(childs, false);) // iteratively add dependent components
    ;
  for (; childs.size() && tryAddComponent(childs, true);) // iteratively add dependent components, but skip non existent optional
                                                          // components
    ;
  if (childs.size())
  {
    logerr("there are {} components which depends on undefined components", childs.size());
    for (auto start : childs)
    {
        std::string err;
        deps_components.resize(0);
        for (auto dep : start->deps)
        {
            HashedConstString depConstString = ECS_HASH_SLOW(dep);
            if (*dep != '?')
                deps_components.push_back(depConstString.hash);
            if (!hasComponent(depConstString.hash))
                err += nau::utils::format("{}{}", err.length() ? ", " : "", dep);
        }
      logerr(" <{}> depends on undefined components: <{}>", start->name.str, err.c_str());
      createComponent(start->name, types.findType(start->type), deps_components, start->io, start->flags, types);
    }
  }
}
}; // namespace ecs
