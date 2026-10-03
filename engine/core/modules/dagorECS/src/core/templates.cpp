// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#include <daECS/core/internal/templates.h>

#include <utility>
#include <vector>

namespace ecs
{

void InstantiatedTemplate::clear(const Archetypes &archetypes, const DataComponents &dataComponents, ComponentTypes &componentTypes)
{
  // const Archetype &cArchetype = archetypes.getArchetype(archetype);
  if (!hasData.get()) // already cleared
    return;
  NAU_ASSERT(archetypes.getArchetype(archetype).getComponentsCount() == componentsCount);
  for (int i = 0; i < componentsCount; ++i)
  {
    if (!isInited(i))
      continue;
    component_index_t componentIndex = archetypes.getComponent(archetype, i);
    type_index_t componentTypeIndex = dataComponents.getComponentById(componentIndex).componentType;
    ComponentTypeFlags typeFlags = componentTypes.getTypeInfo(componentTypeIndex).flags;
    if (typeFlags & COMPONENT_TYPE_NON_TRIVIAL_CREATE)
    {
      ComponentTypeManager *ctm = componentTypes.getTypeManager(componentTypeIndex);
      if (ctm)
        ctm->destroy(initialData.get() + archetypes.getComponentInitialOfs(archetype, i));
    }
  }
  memset(hasData.get(), 0, hasDataElems() * sizeof(size_t));
}

void Templates::clear(const Archetypes &archetypes, const DataComponents &dataComponents, ComponentTypes &componentTypes)
{
  for (auto &t : templates)
    t.clear(archetypes, dataComponents, componentTypes);
  templates.clear();
  templateDbId.clear();
  replicatingTemplates.clear();
  replicatedComponents.clear();
  templateReplData.clear();
  templateReplData.push_back(0);
}

void Templates::remap(const template_t *map, uint32_t used_count, bool defrag_names, const Archetypes &archetypes,
  const DataComponents &dataComponents, ComponentTypes &componentTypes)
{
  NAU_ASSERT(used_count < INVALID_TEMPLATE_INDEX && used_count < size());
#if 0
  uint32_t eraseCount = 0, currentRemapped = used_count-1;
  for (int i = templates.size()-1; i >= 0; --i)//not allowes shuffle!
  {
    const template_t newI = map[i];
    NAU_ASSERT(newI == INVALID_TEMPLATE_INDEX || currentRemapped == newI, "{} map={}, should be {}", i, newI, currentRemapped);
    if (newI == INVALID_TEMPLATE_INDEX)
    {
      eraseCount++;
    } else
    {
      currentRemapped--;
      if (eraseCount != 0)
      {
        const uint32_t at = i+1;
        for (uint32_t ai = at; ai < at + eraseCount; ++ai)
          templates[ai].clear(archetypes, dataComponents, componentTypes);//destructor
        templates.erase(templates.begin() + at, templates.begin() + at + eraseCount);
        templateNamesId.erase(templateNamesId.begin() + at, templateNamesId.begin() + at + eraseCount);
        eraseCount = 0;
      }
    }
  }
  NAU_UNUSED(currentRemapped);
  if (eraseCount)
  {
    templates.erase(templates.begin(), templates.begin() + eraseCount);
    templateNamesId.erase(templateNamesId.begin(), templateNamesId.begin() + eraseCount);
  }
  NAU_UNUSED(used_count);
  NAU_ASSERT(used_count == templates.size());
#else
  std::vector<InstantiatedTemplate> templatesNew(used_count);
  std::vector<uint32_t> newTemplateDbId(used_count);
  std::vector<uint32_t> newTemplateReplData;
  newTemplateReplData.reserve(used_count + 1);
  newTemplateReplData.push_back(0);
  std::vector<component_index_t> newReplicatedComponents;
  newReplicatedComponents.reserve(replicatedComponents.size());
  for (uint32_t i = 0, e = size(); i != e; ++i)
  {
    const template_t newId = map[i];
    if (newId == INVALID_TEMPLATE_INDEX)
      continue;
    templatesNew[newId] = std::move(templates[i]);
    newTemplateDbId[newId] = templateDbId[i];
    uint32_t riCnt = 0;
    const component_index_t *ri = replicatedComponentsList(i, riCnt);
    newReplicatedComponents.insert(newReplicatedComponents.end(), ri, ri + riCnt);
    newTemplateReplData.push_back((uint32_t)newReplicatedComponents.size());
  }
  newReplicatedComponents.shrink_to_fit();
  std::swap(templates, templatesNew);
  std::swap(newTemplateDbId, templateDbId);
  std::swap(templateReplData, newTemplateReplData);
  std::swap(replicatedComponents, newReplicatedComponents);
  for (auto &t : templatesNew)
    t.clear(archetypes, dataComponents, componentTypes); // destructor
#endif
  NAU_UNUSED(defrag_names);
}

InstantiatedTemplate::InstantiatedTemplate(const Archetypes &archetypes, uint32_t archetype_,
  const ChildComponent *const *initializers, const DataComponents &dataComponents, ComponentTypes &componentTypes) :
  archetype(archetype_)
{
  const Archetype &cArchetype = archetypes.getArchetype(archetype);
  componentsCount = cArchetype.getComponentsCount();
  hasData.reset(new size_t[hasDataElems()]);
  alignedEntitySize = archetypes.getComponentInitialOfs(archetype, cArchetype.getComponentsCount() - 1) +
                      archetypes.getComponentSize(archetype, cArchetype.getComponentsCount() - 1);
  alignedEntitySize = (alignedEntitySize + sizeof(size_t) - 1) & ~(sizeof(size_t) - 1); // to keep it aligned to size_t which is used
                                                                                        // in createdBits
  initialData.reset(new uint8_t[alignedEntitySize]);
  memset(initialData.get(), 0, alignedEntitySize); // just in case, set to null
  memset(hasData.get(), 0, hasDataElems() * sizeof(size_t));

  const auto hasCreatableOrTracked = archetypes.getArchetypeCombinedTypeFlags(archetype);
  for (int i = 1, e = cArchetype.getComponentsCount(); i < e; ++i)
  {
    auto initializer = initializers[i]; //
    auto componentIndex = archetypes.getComponent(archetype, i);
    auto componentTypeIndex = dataComponents.getComponentById(componentIndex).componentType;
    ComponentType compType = componentTypes.getTypeInfo(componentTypeIndex);
    ComponentTypeFlags typeFlags = compType.flags;
    ComponentTypeManager *typeManager = typeFlags ? componentTypes.createTypeManager(componentTypeIndex) : NULL;

    if ((typeFlags & COMPONENT_TYPE_CREATE_ON_TEMPL_INSTANTIATE) && initializer && initializer->isNull() && typeManager)
    {
      NAU_ASSERT((typeFlags & COMPONENT_TYPE_NON_TRIVIAL_CREATE));
      alignas(16) char tmpBuf[64];
      void* tmpData = (compType.size <= sizeof(tmpBuf)) ? tmpBuf : malloc(compType.size); /* framemem_allocator TODO Allocators.*/
      // deref 1 here for EntityManager to silence compilation checks. typemanager should not use it to have proper behaviour.
      typeManager->create(tmpData, *(EntityManager *)(uintptr_t)1, INVALID_ENTITY_ID, ComponentsMap(), componentIndex);
      // ugly const_cast..
      *(ChildComponent *)initializer =
        ChildComponent(componentTypes.getTypeById(componentTypeIndex), tmpData, ChildComponent::CopyType::Shallow);
      if (!(typeFlags & COMPONENT_TYPE_BOXED))
        typeManager->destroy(tmpData);
      if (tmpData != tmpBuf)
      {
          free(tmpData); /* framemem_allocator TODO Allocators.*/
      }
    }
    if (!initializer || initializer->isNull())
      continue;
    DAECS_EXT_ASSERTF(componentTypes.getTypeById(componentTypeIndex) == initializer->getUserType(), "{}: cidx {}: 0x{} != 0x{}", i,
      componentIndex, componentTypes.getTypeById(componentTypeIndex), initializer->getUserType());

    auto destDataPtr = initialData.get() + archetypes.getComponentInitialOfs(archetype, i);
    memcpy(destDataPtr, initializer->getRawData(), archetypes.getComponentSize(archetype, i));
    if ((typeFlags & COMPONENT_TYPE_NON_TRIVIAL_CREATE))
    {
      NAU_ASSERT(typeManager);
      if (typeManager)
      {
        if (typeManager->copy(destDataPtr, initializer->getRawData(), componentIndex))
        {
            hasDataSet(i);
        }
        else
        {
            logerr("template initializer {} of type <{}> can not be copied ", dataComponents.getComponentNameById(componentIndex),
                   componentTypes.getTypeNameById(componentTypeIndex));
        }
      }
    }
    else
      hasDataSet(i);
  }

  if (hasCreatableOrTracked & Archetypes::HAS_TRACKED_COMPONENT)
  {
    const component_index_t *__restrict podCidx = archetypes.getTrackedPodCidxUnsafe(archetype);
    // since there can't be new archetype creation within memcpy
    for (auto &trackedPod : archetypes.getTrackedPodPairs(archetype)) // tracked pod
    {
      const uint32_t dstOfs = archetypes.getComponentInitialOfs(archetype, trackedPod.shadowArchComponentId);
      const uint32_t srcOfs = archetypes.getComponentInitialOfs(archetype, archetypes.getArchetypeComponentId(archetype, *podCidx));
      memcpy(initialData.get() + dstOfs, initialData.get() + srcOfs, trackedPod.size);
      ++podCidx;
    }
  }
}

template_t Templates::createTemplate(Archetypes &archetypes, const uint32_t db_id,
  const component_index_t *__restrict componentIndices, uint32_t componentIndicesSize, const component_index_t *__restrict replIndices,
  uint32_t replIndicesSize, const ChildComponent *const *initializers, const DataComponents &dataComponents,
  ComponentTypes &componentTypes)

{
  uint32_t archetype = archetypes.addArchetype(componentIndices, componentIndicesSize, dataComponents, componentTypes);
  NAU_ASSERT_RETURN(archetype < archetypes.size(), INVALID_TEMPLATE_INDEX);

  size_t templ = templates.size();
  DAECS_EXT_ASSERT(templateReplData.size() == templ + 1);
  DAECS_EXT_ASSERTF(templ == templateDbId.size(), "{} != {}", templ, templateDbId.size());
  DAECS_EXT_ASSERT(templ < INVALID_TEMPLATE_INDEX);

  templates.emplace_back(archetypes, archetype, initializers, dataComponents, componentTypes);
  templateDbId.push_back(db_id);

  if (archetypes.getArchetypeCombinedTypeFlags(archetype) & COMPONENT_TYPE_REPLICATION)
  {
    replicatingTemplates.set(templ, true);
    replicatedComponents.insert(replicatedComponents.end(), replIndices, replIndices + replIndicesSize);
  }
  templateReplData.push_back((uint32_t)replicatedComponents.size());

  return (template_t)templ;
}

}; // namespace ecs
