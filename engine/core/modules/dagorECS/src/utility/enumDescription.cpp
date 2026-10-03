// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#include <daECS/core/component.h>
#include <daECS/core/entityComponent.h>
#include <daECS/utility/ecs_asserts.h>
#include <daECS/utility/enum_description.h>
#include <daECS/utility/enum_registration.h>
#include <nau/utils/span.h>

#include <string>
#include <string_view>

#include "nau/utils/sorted_vector.h"

namespace ecs
{

    using DesriptionMap = nau::SortedVectorMap<std::string, EnumLoaderDesc>;
    using string_c_str_comparator = std::less<>;

    static DesriptionMap& get_descriptions()
    {
        static DesriptionMap descriptionMap;
        return descriptionMap;
    }

void ecs_enum_registration(const char *enum_name, enum_parse_t parse, enum_names_t get_names, update_enum_t update_value,
  find_enum_idx_t find_enum, component_type_t component)
{
    auto it = get_descriptions().find(enum_name);
    if (it != get_descriptions().end())
    {
        logerr("you try to register more than one enum type {}", enum_name);
        return;
    }
  get_descriptions().emplace(enum_name, EnumLoaderDesc{parse, get_names, update_value, find_enum, component});
}

const EnumLoaderDesc* find_enum_description(std::string_view tp_name)
{
    auto it = get_descriptions().find(tp_name);
    return it == get_descriptions().end() ? nullptr : &it->second;
}

bool is_type_ecs_enum(const char *type_name) { return find_enum_description(type_name) != nullptr; }

nau::ConstSpan<const char *> get_ecs_enum_values(const char *type_name)
{
  if (auto enumDescr = find_enum_description(type_name))
  {
    return enumDescr->getNames();
  }
  return {};
}

void update_enum_value(const char *type_name, ecs::EntityComponentRef &enum_component, int enum_idx)
{
  if (auto enumDescr = find_enum_description(type_name))
  {
    enumDescr->updateValue(enum_component, enum_idx);
  }
}

int find_enum_idx(const char *type_name, const ecs::EntityComponentRef &enum_component)
{
  if (auto enumDescr = find_enum_description(type_name))
  {
    return enumDescr->findEnum(enum_component);
  }
  return -1;
}
} // namespace ecs