// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#pragma once
#include <daECS/core/component.h>
#include <daECS/core/componentsMap.h>

#include <memory>
#include <unordered_map>
#include <vector>

#include "archetypes.h"
#include "nau/utils/dynamic_bit_vector.h"
#include "typesAndLimits.h"

namespace ecs
{

struct InstantiatedTemplate // it would be used for memcpy
{
    std::unique_ptr<uint8_t[]> initialData;
    // currently initialComponentDataOffset has to be separate array, and can't be reallocated (as creatingEntities keep this as direct
    // pointer)
    std::unique_ptr<size_t[]> hasData;  // bit array!
    uint32_t alignedEntitySize = 0;     // actually it is part of archetype (same for all templates of archetype). todo: move to archetype
    archetype_t archetype = INVALID_ARCHETYPE;
    uint16_t componentsCount = 0;
    InstantiatedTemplate()
    {
    }
  InstantiatedTemplate(const Archetypes &archetypes, uint32_t archetype, const ChildComponent *const *initial_data,
    const DataComponents &dataComponents, ComponentTypes &componentTypes);
  void clear(const Archetypes &archetypes, const DataComponents &dataComponents, ComponentTypes &componentTypes);
  InstantiatedTemplate(const InstantiatedTemplate &) = delete;
  InstantiatedTemplate(InstantiatedTemplate &&) = default;
  InstantiatedTemplate &operator=(InstantiatedTemplate &&) = default;
  static constexpr int BITS_IN_WORD = sizeof(size_t) * 8;
  static constexpr unsigned int bitsShift = (sizeof(size_t) == 4 ? 5 : 6);
  void hasDataSet(uint32_t i)
  {
    NAU_ASSERT(i < componentsCount);
    hasData.get()[size_t(i) >> bitsShift] |= (size_t(1UL) << (size_t(i) & size_t(BITS_IN_WORD - 1)));
  }
  static __forceinline bool isInited(const size_t *__restrict data, uint32_t i)
  {
    return data[size_t(i) >> bitsShift] & (size_t(1UL) << (size_t(i) & size_t(BITS_IN_WORD - 1)));
  }
  const bool isInited(uint32_t i) const
  {
    NAU_ASSERT(i < componentsCount);
    return isInited(hasData.get(), i);
  }
  size_t hasDataElems() const { return (componentsCount + sizeof(size_t) * 8 - 1) / (8 * sizeof(size_t)); }
};

struct Templates
{
public:
  uint32_t size() const { return (uint32_t)templates.size(); }
  template_t createTemplate(Archetypes &archetypes, const uint32_t db_id, const component_index_t *__restrict components,
    uint32_t components_cnt, const component_index_t *__restrict repl, uint32_t repl_cnt, const ChildComponent *const *initial_data,
    const DataComponents &dataComponents, ComponentTypes &componentTypes); // will always create template. no search
  const InstantiatedTemplate &getTemplate(template_t t) const { return templates[t]; }
  void clear(const Archetypes &archetypes, const DataComponents &dataComponents, ComponentTypes &componentTypes);
  const void *getTemplateData(template_t t, uint32_t ofs, uint32_t ci) const; // ci 0.. to components count
  // const char *getTemplateName(template_t t) const
  //   {return t < templateNamesId.size() ? templateNamesAllocator.getDataRawUnsafe(templateNamesId[t]) : nullptr;}
  bool hasReplication(template_t t) const { return replicatingTemplates.test(t, false); }
  bool isReplicatedComponent(template_t t, component_index_t cidx) const;
  void remap(const template_t *map, uint32_t size, bool defrag_names, const Archetypes &archetypes,
    const DataComponents &dataComponents, ComponentTypes &componentTypes);
  const component_index_t *replicatedComponentsList(template_t t, uint32_t &cnt) const;

protected:
  friend class EntityManager;
  InstantiatedTemplate &getTemplate(template_t t) { return templates[t]; }
  // StringTableAllocator templateNamesAllocator={10, 13};
  std::vector<InstantiatedTemplate> templates;          // index for archetypes
  std::vector<uint32_t> templateDbId;                   // SoA with templates
  std::vector<uint32_t> templateReplData = {0};         // SoA with templates, offset in replicatedComponents. 0 if nothing
  std::vector<component_index_t> replicatedComponents;  // all replicated components for all templates. todo: deduplication
  nau::DynamicBitVector<> replicatingTemplates;         // cached COMPONENT_TYPE_REPLICATION archetypes flag
};

inline const void *Templates::getTemplateData(template_t t, uint32_t ofs, uint32_t cid) const
{
  if (t >= templates.size())
    return nullptr;
  const InstantiatedTemplate &tInfo = templates[t];
  if (cid >= tInfo.componentsCount || !tInfo.isInited(cid) || ofs > tInfo.alignedEntitySize)
    return nullptr;
  return tInfo.initialData.get() + ofs;
}

__forceinline const component_index_t *Templates::replicatedComponentsList(template_t t, uint32_t &cnt) const
{
  const uint32_t at = templateReplData[t];
  cnt = templateReplData[t + 1] - at;
  return replicatedComponents.data() + at;
}

}; // namespace ecs
