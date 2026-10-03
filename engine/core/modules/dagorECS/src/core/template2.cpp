// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#include <ctype.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/template.h>
#include <nau/utils/mum_hash.h>
#include <string.h>

#include <algorithm>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "nau/utils/dynamic_bit_vector.h"
#include "nau/utils/inline_string.h"
#include "nau/utils/sorted_vector.h"
#include "singletonName.h"
#include "tokenize_const_string.h"

namespace ecs
{

template <class To, class From>
inline void combine_hash(To &to, From b)
{
  to ^= To(To(b) + To(0x9e3779b9) + To(to << 6) + To(to >> 2));
}; // this is boost implementation, supposed to be good

#define TAG_SEP              ", "
#define TEMPL_CONCAT_SEP_STR "+"

bool filter_needed(const char* filter_tags, const ska::flat_hash_set<std::string>& valid_tags)
{
  if (!filter_tags || !*filter_tags)
    return true;
  return tokenize_const_string(filter_tags, TAG_SEP, [&](std::string_view tag)
  {
      static constexpr int MAX_TAG = 128;
      nau::InlineString<char, MAX_TAG> tagName(tag.data(), tag.length());
      NAU_ASSERT(tag.length() < MAX_TAG);
      if (valid_tags.find_as(tagName.c_str()) == valid_tags.end())
          return false;
      return true;
  });
};

bool filter_needed(const char *filter_tags, const TagsSet &valid_tags)
{
  if (!filter_tags || !*filter_tags)
    return true;
  return tokenize_const_string(filter_tags, TAG_SEP,
                               [&](std::string_view tag)
  {
      return valid_tags.find(ecs_hash(tag)) != valid_tags.end();
  });
};


bool Template::needCopy(component_t comp, const TemplatesData &db) const
{
  if (!totalComponentFlags())
    return false;
  if (templComponentFlags() && (trackedSet().count(comp) || replicatedSet().count(comp)))
    return true;
  for (const int parentId : parents)
    if (db.getTemplateRefById(parentId).needCopy(comp, db))
      return true;
  return false;
}

bool Template::isReplicated(component_t comp, const TemplatesData &db) const
{
  if (!(totalComponentFlags() & FLAG_REPLICATED))
    return false;
  if ((templComponentFlags() & FLAG_REPLICATED) && replicatedSet().count(comp))
    return true;
  for (const int parentId : parents)
    if (db.getTemplateRefById(parentId).isReplicated(comp, db))
      return true;
  return false;
}

uint32_t Template::getRegExpInheritedFlags(component_t aname, const TemplatesData &db) const
{
  if (!totalComponentFlags())
    return 0;
  uint32_t flags = 0;
  flags |= trackedSet().count(aname) ? FLAG_CHANGE_EVENT : 0;
  flags |= replicatedSet().count(aname) ? FLAG_REPLICATED : 0;
  if (flags == FLAG_REPLICATED_OR_TRACKED) // there is no sense in iterating over parents
    return flags;
  for (const int parentId : parents)
  {
    const Template &parent = db.getTemplateRefById(parentId);
    flags |= parent.getRegExpInheritedFlags(aname, db);
    if (flags == FLAG_REPLICATED_OR_TRACKED) // there is no sense in iterating over parents
      return flags;
  }
  return flags;
}

inline bool fast_isalnum_or_(char c_)
{
  unsigned char c = (unsigned char)c_;
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' || (c >= '0' && c <= '9');
}
static inline bool validate_template_name(const std::string_view& tname)
{
  if (tname.empty())
    return false;
  for (auto c : tname)
    if (!fast_isalnum_or_(c))
    {
      logerr("Template names should contain only alpha characters, numbers or '_'.\nInstead we got '%.*s'", tname.length(),
        tname.data());
      return false;
    }
  return true;
}
bool TemplateDB::is_valid_template_name(const char *n, size_t nlen)
{
    return n && validate_template_name(std::string_view(n, nlen));
}

Template::template_hash_t Template::buildHash() const
{
  template_hash_t hash = templAllFlags();
  combine_hash(hash, ecs_mem_hash((const char *)sets.get(), (ignoredOfs() + ignoredCount) * sizeof(component_t)));
  for (auto &c : components)
  {
    combine_hash(hash, c.first);
    combine_hash(hash, c.second.getUserType());
    // add values here. there is no 'getHash()' in components managers, so we can only hash pods now. But that is totally doable for
    // PODs and others as well.
  }
  return hash;
}

void Template::buildSets(const component_set &tracked_set, const component_set &replicated_set,
  const component_set &ignored_initial_repl)
{
    sets = std::make_unique<component_t[]>(tracked_set.size() + replicated_set.size() + ignored_initial_repl.size());
    trackedCount = tracked_set.size();
    replicatedCount = replicated_set.size();
    ignoredCount = ignored_initial_repl.size();
    memcpy(sets.get() + trackedOfs(), tracked_set.data(), trackedCount * sizeof(component_t));
    memcpy(sets.get() + replicatedOfs(), replicated_set.data(), replicatedCount * sizeof(component_t));
    memcpy(sets.get() + ignoredOfs(), ignored_initial_repl.data(), ignoredCount * sizeof(component_t));
    localFlags &= ~FLAG_REPLICATED_OR_TRACKED;
    localFlags |= (trackedSet().empty() ? 0 : FLAG_CHANGE_EVENT) | (replicatedSet().empty() ? 0 : FLAG_REPLICATED);
}

Template::Template(const char* tname, ComponentsMap&& amap, component_set&& tracked_set, component_set&& replicated_set, component_set&& ignored_initial_repl, bool singleton_, const char* path_ /*= ""*/) :
    name(tname),
    components(std::move(amap))
#if NAU_DEBUG
    ,
    path(path_)
#endif
{
#if NAU_DEBUG
  auto checkSet = [&](const char *n, const component_set &s) {
    for (auto t : s)
    {
        if (components.end() == std::find_if(components.begin(), components.end(), [&](auto& c)
        {
            return c.first == t;
        }))
        {
            logerr("{} component (0x{}) in template <{}> is not in it's component's list", n, t, tname);
        }
    }
  };

  checkSet("tracked", tracked_set);
  checkSet("replicated", replicated_set);
// checkSet("ignoredInInitialReplication", ignored_initial_repl);//this has to checked including parents!
#endif

  buildSets(tracked_set, replicated_set, ignored_initial_repl);
  NAU_UNUSED(path_);
  inheritedFlags = localFlags;
  if (singleton_)
    components[ECS_HASH(SINGLETON_NAME)] = ecs::Tag();
  components.shrink_to_fit();
}

Template::Template() {} // needed only for dummy template in 'getEntityTemplate'
Template::~Template() = default;

Template::Template(const Template &t)
{
  const int setsNum = t.trackedCount + t.replicatedCount + t.ignoredCount;
  sets = std::make_unique<component_t[]>(setsNum);
  std::copy(t.sets.get(), t.sets.get() + setsNum, sets.get());
  parents = t.parents;
  name = t.name;
#if NAU_DEBUG
  path = t.path;
#endif
  trackedCount = t.trackedCount;
  replicatedCount = t.replicatedCount;
  ignoredCount = t.ignoredCount;
  localFlags = t.localFlags;
  inheritedFlags = t.inheritedFlags;
  tag = t.tag;
  components = t.components;
}

bool Template::hasComponent(const HashedConstString &hashed_name, const TemplatesData &db) const
{
  auto it = components.find(hashed_name);
  // CHECK_IT_HASH(it, components, hashed_name.str, hashed_name.hash);
  if (it)
    return true;
  for (const int pid : parents)
    if (db.getTemplateRefById(pid).hasComponent(hashed_name, db))
      return true;
  return false;
}

const ChildComponent &Template::getComponent(const HashedConstString &hashed_name, const TemplatesData &db) const
{
  auto it = components.find(hashed_name);
  // CHECK_IT_HASH(it, components, hashed_name.str, hashed_name.hash);
  if (it)
    return *it;
  for (const int pid : parents)
  {
    const ChildComponent &comp = db.getTemplateRefById(pid).getComponent(hashed_name, db);
    if (!comp.isNull())
      return comp;
  }
  return ChildComponent::get_null();
}

void Template::setParentsInternal(const uint32_t *pb, const uint32_t *pe)
{
  parents.assign(pb, pe); // last parent is overwriting previous one
}

void TemplateDB::push_back(Template &&templ)
{
    templates.emplace_back(std::move(templ)).resolveFlags(*this);
    instantiatedTemplates.emplace_back();
}

template_t TemplateDB::getInstantiatedTemplateByName(const char *name) const
{
  const int id = getTemplateIdByName(name);
  if (NAU_UNLIKELY(id == -1))
      return ecs::INVALID_TEMPLATE_INDEX;
  return instantiatedTemplates[id].t;
}

int TemplateDB::buildTemplateIdByName(const char *name_)
{
  HashedLenConstString name = ECS_HASHLEN_SLOW(name_);
  int id = getTemplateIdByName(name);
  if (NAU_LIKELY(id != -1))  // likely
      return id;
  // Template not exist -> try build new one (if name is compound)
  std::vector<char, std::allocator<char>> copyName;
  copyName.reserve(name.len + 1);
  std::vector<uint32_t, std::allocator<uint32_t>> parents;
  // Template not exist -> try build new one (if name is compound)
  if (!tokenize_const_string(name_, TEMPL_CONCAT_SEP_STR, [&](std::string_view str1)
  {
      const int it = templatesIds.getNameId(str1.data(), str1.size());
      if (it == -1)
      {
          logerr("Can't find template '%.*s' while building compound template '{}'", str1.length(), str1.data(), name_);
          return false;
      }
      if (str1.data() == name.str && str1.size() == name.len)  // not a compound
          return false;
      for (const uint32_t parentId : parents)
      {
          if (parentId == it)
          {
              logerr("Ignore duplicate parent <%.*s> while building compound template <{}>", str1.data(), str1.length(), name_);
              return true;
          }
      }
      if (!copyName.empty())
          copyName.push_back(TEMPL_CONCAT_SEP_STR[0]);
      copyName.insert(copyName.end(), str1.data(), str1.data() + str1.size());
      parents.insert(parents.begin(), (uint32_t)it);  // 'parents' are reversed so first one in array is last one in data decl
      return true;
  }))
      return -1;

  if (copyName.size() != name.len) // we better have compounds->actual templates resolution (where leaves are 1->1), rather than use
                                   // naming. still works
  {
    copyName.push_back(0);
    name = ECS_HASHLEN_SLOW(copyName.data());
    int id = getTemplateIdByName(name);
    if (id != -1)
      return id;
  }
  Template newTemplate;
  newTemplate.parents.assign(parents.begin(), parents.end());
  id = templatesIds.addNameId(name.str, name.len, name.hash);
  newTemplate.name = templatesIds.getName(id);
  DAECS_EXT_ASSERT(id == templates.size());
  push_back(std::move(newTemplate));
  return id;
}

template <class Container>
static bool get_parents(const TemplatesData &db, Container &to, Template *templ, nau::ConstSpan<const char *> *pnames)
{
  if (pnames)
  {
    for (const char *parentName : *pnames)
    {
      const int parent = db.getTemplateIdByName(parentName);
      if (parent < 0)
      {
        logerr("Can't resolve parent '{}' for template '{}'", parentName, templ->getName());
        NAU_UNUSED(templ);
        return false;
      }
      to.push_back((uint32_t)parent);
    }
    std::reverse(to.begin(), to.end());  // last parent is overwriting previous one
  }
  return true;
}

uint32_t Template::resolveComponentsFlagsRecursive(const TemplatesData &db) const
{
  uint32_t flags = templComponentFlags();
  if (flags == FLAG_REPLICATED_OR_TRACKED)
    return flags;
  for (auto p : parents)
  {
    flags |= db.getTemplateRefById(p).totalAllFlags();
    if (flags == FLAG_REPLICATED_OR_TRACKED)
      return flags;
  }
  return flags;
}

bool Template::calcValidDependencies(const TemplatesData &db) const
{
  auto checkCachedDependencies = [](const DataComponents &dc, const TemplatesData &db, const Template &fin, const Template &t) {
    for (const component_index_t dcidx : t.dependencies)
      if (!fin.hasComponent(ecs::HashedConstString{nullptr, dc.getComponentTpById(dcidx)}, db))
        return false;
    return true;
  };

  auto fillCache = [&](const DataComponents &dc, const TemplatesData &db, const Template &fin, const Template &t) {
    bool resolved = true, hasAll = true;
    t.dependencies.clear();
    for (auto &c : t.getComponentsMap())
    {
      const component_index_t cidx = dc.findComponentId(c.first);
      if (cidx == INVALID_COMPONENT_INDEX)
      {
        resolved = false;
        continue;
      }
      uint32_t depBegin, depEnd;
      dc.getDependeciesById(cidx, depBegin, depEnd);
      for (uint32_t dep = depBegin; dep < depEnd; dep++)
      {
        const component_index_t dependenceCidx = dc.findComponentId(dc.getDependence(dep));
        if (dependenceCidx == INVALID_COMPONENT_INDEX)
          hasAll = resolved = false;
        else
          t.dependencies.push_back(dependenceCidx);
      }
    }
    if (resolved)
      t.setDependenciesResolved();
    return hasAll && checkCachedDependencies(dc, db, fin, t);
  };

  auto checkTemplate = [&](const DataComponents &dc, const TemplatesData &db, bool &ret, const Template &fin, const Template &t) {
    bool hasAll = checkCachedDependencies(dc, db, fin, t);
    if (hasAll && !(t.templAllFlags() & FLAG_DEPENDENCIES_RESOLVED))
      hasAll = fillCache(dc, db, fin, t);
    ret &= hasAll;
  };

  size_t id = this - std::to_address(db.begin());
  NAU_ASSERT_RETURN(id < db.size() || this->getParents().empty(), false);

  bool ret = true;
  db.iterate_parents(
    id, [&](uint32_t t) { return ret && !db.getTemplateRefById(t).canInstantiate(); }, // no sense to iterate over templates that can
                                                                                       // instantatiate, or if we already know that we
                                                                                       // can't
    [&](uint32_t t) { checkTemplate(g_entity_mgr->getDataComponents(), db, ret, *this, db.getTemplateRefById(t)); });

  return ret;
}

bool Template::reportInvalidDependencies(const TemplatesData &db) const
{
  bool ret = true;
  auto logMissing = [&](const DataComponents &dc, const TemplatesData &db, const Template &t) {
    for (auto &c : t.getComponentsMap())
    {
      const component_index_t cidx = dc.findComponentId(c.first);
      if (cidx == INVALID_COMPONENT_INDEX)
        continue;
      uint32_t depBegin, depEnd;
      dc.getDependeciesById(cidx, depBegin, depEnd);
      for (uint32_t dep = depBegin; dep < depEnd; dep++)
      {
        const component_t dependence = dc.getDependence(dep);
        const component_index_t dependenceCidx = dc.findComponentId(dependence);
        if (dependenceCidx == INVALID_COMPONENT_INDEX ||
            !this->hasComponent(ecs::HashedConstString{dc.getComponentNameById(dependenceCidx), dependence}, db))
        {
          ret = false;
          logerr("template <{}>can't be created"
                 ", because <{}> template component depends on <{}|0x{}> component that is missing in template",
            this->getName(), dc.getComponentNameById(cidx), dc.getComponentNameById(dependenceCidx),
            dc.getComponentTpById(dependenceCidx));
        }
      }
    }
  };

  size_t id = this - std::to_address(db.begin());
  NAU_ASSERT_RETURN(id < db.size() || this->getParents().empty(), false);
  db.iterate_parents(
    id, [&](uint32_t t) { return ret && !db.getTemplateRefById(t).canInstantiate(); }, // no sense to iterate over templates that can
                                                                                       // instantatiate, or if we already know that we
                                                                                       // can't
    [&](uint32_t t) { logMissing(g_entity_mgr->getDataComponents(), db, db.getTemplateRefById(t)); });
  return ret;
}

TemplateDB::AddResult TemplateDB::addTemplateWithParents(Template &&templ, uint32_t tag)
{
  const char *templName = templ.getName();
  if (!validate_template_name(templName)) // move to template read
    return AR_BAD_NAME;

#if NAU_DEBUG
  for (auto &p : templ.parents)
    DAECS_EXT_ASSERTF(p < templates.size(), "{} {}", p, templates.size());
#endif
  // we validate dependencies after sort, because components are sorted by indices, and indices are sorted by dependence.
  // and we need components to be initialized by the time dependent components is being initialized

  const Template *existingTempl = getTemplateByName(templName);
  if (existingTempl)
  {
    if (&templ == existingTempl)
      return AR_OK;
    if (templ.parents.size() != existingTempl->parents.size())
      return AR_DUP;
    if (!std::equal(templ.parents.begin(), templ.parents.end(), existingTempl->parents.begin()))
        return AR_DUP;
    if (!existingTempl->isEqual(templ))
      return AR_DUP;
    return AR_OK;
  }

  const int id = templatesIds.addNameId(templName);
  templ.name = templatesIds.getName(id);
  templ.tag = tag;
#if NAU_DEBUG
  templ.path = pathNames.getName(pathNames.addNameId(templ.path));
#endif
  DAECS_EXT_ASSERT(id == templates.size());
  push_back(std::move(templ));
  templates[id].resolveFlags(*this);
  return AR_OK;
}

TemplateDB::AddResult TemplateDB::addTemplate(Template &&templ, nau::ConstSpan<const char *> *pnames, uint32_t tag)
{
  if (pnames)
  {
      std::vector<uint32_t> pars; /* framemem_allocator TODO Allocators.*/
      if (!get_parents(*this, pars, &templ, pnames))
          return AR_UNKNOWN_PARENT;
      templ.setParentsInternal(pars.data(), pars.data() + pars.size());
  }
  else
    templ.parents.clear();
  return addTemplateWithParents(std::move(templ), tag);
}

TemplateDB::AddResult TemplateDB::addTemplate(Template &&templ, nau::ConstSpan<template_t> parentIds, uint32_t tag)
{
  templ.parents.clear();
  if (!parentIds.empty())
  {
      for(auto& parent : parentIds)
      {
          templ.parents.push_back(parent);
      }
  }
  return addTemplateWithParents(std::move(templ), tag);
}

void TemplateRefs::emplace(Template &&t, Template::ParentsList &&parents)
{
    t.parents = std::move(parents);
    int nid = templatesIds.getNameId(t.getName());
    if (nid < 0)
    {
        nid = templatesIds.addNameId(t.getName());
        NAU_ASSERT(nid == templates.size());
        templates.emplace_back(std::move(t));
    }
  else
  {
    NAU_ASSERT(emptyCount > 0);
    emptyCount--;
    templates[nid] = std::move(t);
  }
#if NAU_DEBUG
  for (size_t i = 0, e = templates.size(); i != e; ++i)
    for (auto p : templates[nid].parents)
      DAECS_EXT_ASSERTF(p < templates.size(), "{} <= {} {}", templates.size(), p, t.getName());
#endif
  templates[nid].name = templatesIds.getName(nid);
#if NAU_DEBUG
  templates[nid].path = pathNames.getName(pathNames.addNameId(templates[nid].path));
#endif
}

void TemplateRefs::amend(Template &&templ, Template::ParentsList &&parentNameIds)
{
  auto eit = find(templ.getName());
  if (eit == end() || !eit->isValid())
  {
      emplace(std::move(templ), std::move(parentNameIds));
  }
  else
  {
    eit->parents.insert(eit->parents.cend(), parentNameIds.begin(), parentNameIds.end());
    Template::component_set tracked(eit->trackedSet().begin(), eit->trackedSet().end()),
      replicated(eit->replicatedSet().begin(), eit->replicatedSet().end()),
      ignored(eit->ignoredSet().begin(), eit->ignoredSet().end());
    tracked.insert(templ.trackedSet().begin(), templ.trackedSet().end());
    replicated.insert(templ.replicatedSet().begin(), templ.replicatedSet().end());
    ignored.insert(templ.ignoredSet().begin(), templ.ignoredSet().end());
    eit->buildSets(tracked, replicated, ignored);
    eit->localFlags |= templ.localFlags;
    for (auto &&c : templ.components)
        eit->components[c.first] = std::move(c.second);
    eit->setInstantiatable(eit->calcValidDependencies(*this));
  }
}

void TemplateRefs::amend(TemplateRefs &&overrides)
{
  if (overrides.empty())
    return;
  for (auto &&templ : overrides)
  {
    if (!templ.isValid())
      continue;
    for (auto &p : templ.parents)
      p = ensureTemplate(overrides.templatesIds.getName(p));
    amend(std::move(templ), std::move(templ.parents));
  }
}


uint32_t TemplateRefs::pushEmpty(const char *n)
{
  Template t;
  t.name = nullptr; // invalid
  NAU_ASSERT_RETURN(templatesIds.getNameId(n) == -1, templatesIds.getNameId(n));
  emptyCount++;

  uint32_t nid = templatesIds.addNameId(n);
  NAU_ASSERT(nid == templates.size());
  templates.emplace_back(std::move(t));
  return nid;
}

bool TemplateRefs::isTopoSorted() const
{
  for (size_t i = 0, e = templates.size(); i != e; ++i)
    for (auto p : templates[i].parents)
      if (i <= p)
        return false;
  return true;
}

void TemplateRefs::topoSort()
{
  FRAMEMEM_REGION;
  nau::DynamicBitVector<> temp(templates.size()), perm(templates.size());
  std::vector<uint32_t, std::allocator<uint32_t>> order;
  order.reserve(templates.size());
  std::vector<uint32_t, std::allocator<uint32_t>> remap;
  remap.resize(templates.size(), ~0u);
  for (uint32_t i = 0, sz = templates.size(); i < sz; ++i)
    if (!templates[i].isValid())
      perm.set(i, true);

  for (uint32_t i = 0, sz = templates.size(); i < sz; ++i)
    iterate_parents(
      i,
      [&](uint32_t p) {
        if (perm.test(p, true))
          return false;
        if (!temp.test(p, false)) // loop not detected
        {
          temp.set(p, true);
          return true;
        }
        logerr("template <{}> has loop", templatesIds.getName(i));
        return false;
      },
      [&](uint32_t p) {
        temp.set(p, false);
        perm.set(p, true);
        remap[p] = order.size();
        order.push_back(p);
      });
  TemplateList newTemplates;
  newTemplates.reserve(order.size());
  NameMap newTemplatesIds; // todo: add better reserve
  // now process and reinsert with topo order
  for (uint32_t i : order)
  {
    auto &t = templates[i];
    NAU_VERIFY(newTemplatesIds.addNameId(t.getName()) == newTemplates.size());
    for (auto pi = t.parents.begin(), pe = t.parents.end(); pi != pe;)
    {
      template_t &p = *pi;
      if (remap[p] >= newTemplates.size()) // invalid/missing
      {
        logerr("template {} {} parent {}", remap[p] == ~0u ? "has broken" : "looped parent", t.getName(), templatesIds.getName(p));
        pi = templates[i].parents.erase(pi);
        pe = templates[i].parents.end();
      }
      else
      {
        NAU_VERIFY((p = remap[p]) < newTemplates.size()); // verify toposorting
        ++pi;
      }
    }
    t.name = newTemplatesIds.getName(newTemplates.size());
    newTemplates.emplace_back(std::move(templates[i]));
  }
  std::swap(templates, newTemplates);
  std::swap(templatesIds, newTemplatesIds);
  emptyCount = 0; // we cleaned empty as well
}

void TemplateRefs::resolveBroken()
{
  if (getEmptyCount() == 0)
    return;
  // we have to fix all of them
  nau::SortedVectorSet<uint32_t, std::less<uint32_t>, std::allocator<uint32_t>> broken;
  for (uint32_t i = 0, e = templates.size(); i < e; ++i)
    if (!templates[i].isValid())
      broken.emplace(i);
  for (auto &t : templates)
  {
    for (auto pi = t.parents.begin(), pe = t.parents.end(); pi != pe;)
    {
      if (broken.find(*pi) != broken.end())
      {
        pi = t.parents.erase(pi);
        pe = t.parents.end();
        // alternatively, call t.invalidate() (t.name = NULL;) and increase brokenCount;
      }
      else
        ++pi;
    }
  }
}

void TemplateRefs::finalize(uint32_t tag)
{
  if (finalized)
    return;
  if (!templates.size())
  {
    finalized = true;
    return;
  }
  addComponent(ECS_HASH(SINGLETON_NAME).hash, 0, SINGLETON_NAME);
  for (auto &t : templates)
  {
    t.tag = tag;
    if (t.isValid() && !TemplateDB::is_valid_template_name(t.getName()))
    {
      logerr("invalid template name <{}>", t.getName());
      t.name = nullptr; // invalidate
      emptyCount++;
    }
  }
  if (getEmptyCount() != 0)
    reportBrokenParents([&](const char *t, const char *p) {
      NAU_UNUSED(p);
      NAU_UNUSED(t);
      logerr("template <{}> has invalid parent <{}>", t, p);
    });
  if (!isTopoSorted())
    topoSort();
  resolveBroken();
  for (auto &t : templates)
    t.resolveFlags(*this);
  templates.shrink_to_fit();
  templatesIds.shrink_to_fit();
#if NAU_DEBUG
  for (auto &t : templates)
    for (auto &c : t.components)
    {
// we keep this check until we migrate from _dot_/. to _
#if 0
        if (getComponentName(c.first) && strstr(getComponentName(c.first), "."))
        {
          std::string str = getComponentName(c.first);
          for (size_t index = 0;;index++)
          {
             /* Locate the substring to replace. */
             index = str.find(".", index);
             if (index == std::string::npos)
               break;
             str.replace(index, 1, "_");
          }
          auto u = ECS_HASH_SLOW(str.c_str()).hash;
          if (g_entity_mgr->getDataComponents().hasComponent(u) || getComponentName(u))
          {
            logerr("component {} intersects with {}", getComponentName(c.first), str.c_str());
          }
        }
#endif
      if (c.second.isNull() && g_entity_mgr && !g_entity_mgr->getDataComponents().hasComponent(c.first) && !getComponentType(c.first))
        {
            logerr("template {} at {} has component {} that is not defined in code", t.getName(), t.getPath(), getComponentName(c.first));
        }
    }
#endif
#if NAU_DEBUG
  {
    size_t hash = 1;
    for (auto &t : templates)
      combine_hash(hash, t.buildHash());
    uint32_t cmpnts = 0, tracked = 0, repl = 0;
    for (auto &t : templates)
    {
      cmpnts += t.components.size();
      tracked += t.trackedSet().size();
      repl += t.replicatedSet().size();
    }
    ECS_LOG("tag={}: templates {} with total components = {}({} unique) tracked = {} repl = {}, hash 0x{}", tag, templates.size(),
      cmpnts, componentToName.size(), tracked, repl, hash);
  }
#endif
  finalized = true;
}

void TemplateDB::addTemplates(TemplateRefs &trefs, uint32_t tag)
{
  trefs.finalize(tag);
  if (trefs.getEmptyCount() == 0 && templates.empty())
  {
    ECS_LOG("fast path of templates move");
    ((TemplatesData&)*this) = std::move((TemplatesData&&)trefs);
    instantiatedTemplates.resize(templates.size());
  }
  else
  {
    addComponentNames(trefs);
    const uint32_t start = templates.size();
    templates.reserve(trefs.size() + start);
    instantiatedTemplates.reserve(templates.size());
    std::vector<uint32_t, std::allocator<uint32_t>> remap;
    if (trefs.getEmptyCount() != 0)
      remap.resize(trefs.size());

    size_t at = start;
    for (size_t i = 0, e = trefs.size(); i != e; ++i)
    {
      auto &tref = trefs.templates[i];
      if (!tref.isValid())
      {
        NAU_ASSERT(trefs.getEmptyCount() != 0);
        continue;
      }
      for (auto &p : tref.parents)
      {
        DAECS_EXT_ASSERT(p < i); // ensure toposorting
        p = remap.empty() ? p + start : remap[p];
        DAECS_EXT_ASSERT(p < i + start); // ensure toposorting
      }

      AddResult ar = addTemplateWithParents(std::move(tref), tag);
      if (ar != AR_OK && ar != AR_DUP)
      {
          logerr("Failed to add template '{}' due to {}", tref.getName(), (int)ar);
      }
      if (!remap.empty())
      {
          remap[i] = (ar == AR_OK) ? at++ : ~0u;
      }
    }
  }
  templates.shrink_to_fit();
  templatesIds.shrink_to_fit();
}

void TemplateDB::clear()
{
  templates.clear();
  templates.shrink_to_fit();
  instantiatedTemplates.clear();
  templatesIds.reset();
  templatesIds.shrink_to_fit();
  db.filterTags.clear();
#if NAU_DEBUG
  db.componentTemplate.clear();
  db.componentTemplate.shrink_to_fit();
  db.componentTags.clear();
  db.componentTags.shrink_to_fit();
#endif
  db.metaInfoData.reset();
}

void TemplateDB::setFilterTags(nau::ConstSpan<const char *> tags)
{
  db.filterTags.clear();
  for (const char *tag : tags)
    db.filterTags.insert(ECS_HASH_SLOW(tag).hash);
  db.hasServerTag = db.filterTags.find(ECS_HASH("server").hash) != db.filterTags.end();
}

void TemplateDBInfo::updateComponentTags(HashedConstString comp_name, const char *filter_tags, const char *templ_name,
  const char *tpath)
{
  NAU_UNUSED(comp_name);
  NAU_UNUSED(filter_tags);
  NAU_UNUSED(templ_name);
  NAU_UNUSED(tpath);
  if (hasServerTag && filter_tags && strstr(filter_tags, "server") != nullptr)
  {
    const hash_str_t compHash = comp_name.hash;
#if NAU_DEBUG
    auto fi = componentFlags.find(compHash);
    if (fi != componentFlags.end())
      if ((fi->second & DataComponent::DONT_REPLICATE) == 0)
      {
        logerr(
          "component named <{}> was tagged as server (don't replicate) in template <{}>, but was already registered as not-server",
          comp_name.str, templ_name);
      }
#endif
    componentFlags[compHash] |= DataComponent::DONT_REPLICATE;
  }
#if NAU_DEBUG
  hash_str_t compHash = comp_name.hash;
  const char *tagsNotNull = filter_tags ? filter_tags : "";
  auto it = componentTags.find(compHash);
  if (it == componentTags.end())
    componentTags.emplace(compHash, tagsNotNull);
  else
  {
    if (strcmp(it->second.c_str(), tagsNotNull) != 0)
    {
      const char *otherTemplName = componentTemplate.find(compHash)->second.c_str();
      logerr("component named <{}> has tag of <{}> in template <{}:{}>, while it is tagged as <{}> in other template <{}>",
        comp_name.str, tagsNotNull, tpath, templ_name, it->second.c_str(), otherTemplName);
    }
  }
  if (componentTemplate.find(compHash) == componentTemplate.end())
    componentTemplate.emplace(compHash, templ_name);
#endif
}


struct TemplateDBInfo::MetaInfoData
{
  DataBlock metaDataBlock;
  typedef ska::flat_hash_map<std::string, int> TLinks;
  ska::flat_hash_set<uint64_t, ska::power_of_two_std_hash<uint64_t>> templatesComps;
  TLinks commonComponentsLinks;
  TLinks commonTemplatesLinks;
  TLinks componentsLinks;
  TLinks templatesLinks;
};

void TemplateDBInfo::MetaInfoDataDeleter::operator()(MetaInfoData *data)
{
  if (data)
    delete data;
}

bool TemplateDBInfo::addMetaData(MetaInfoType type, const char *name, const DataBlock &info)
{
  NAU_ASSERT_RETURN(name && *name, false);
  if (!metaInfoData)
    metaInfoData.reset(new MetaInfoData());
  MetaInfoData::TLinks *links = nullptr;
  const char *id = "";
  switch (type)
  {
    case MetaInfoType::COMMON_COMPONENT:
      links = &metaInfoData->commonComponentsLinks;
      id = "common_component";
      break;
    case MetaInfoType::COMMON_TEMPLATE:
      links = &metaInfoData->commonTemplatesLinks;
      id = "common_template";
      break;
    case MetaInfoType::COMPONENT:
      links = &metaInfoData->componentsLinks;
      id = "component";
      break;
    case MetaInfoType::TEMPLATE:
      links = &metaInfoData->templatesLinks;
      id = "template";
      break;
  }
  if (!links)
    return false;
  if (links->find(name) != links->end())
    return false;
  (*links)[name] = metaInfoData->metaDataBlock.blockCount();
  metaInfoData->metaDataBlock.addNewBlock(&info, id);
  return true;
}

bool TemplateDBInfo::addMetaLink(MetaInfoType type, const char *name, const char *link)
{
  NAU_ASSERT_RETURN(name && *name, false);
  NAU_ASSERT_RETURN(link && *link, false);
  if (!metaInfoData) // no commons defined, links not possible
    return false;
  switch (type)
  {
    case MetaInfoType::COMMON_COMPONENT:
    case MetaInfoType::COMMON_TEMPLATE: return false; // common to common links not supported

    case MetaInfoType::COMPONENT:
    {
      auto itc = metaInfoData->commonComponentsLinks.find(link);
      if (itc == metaInfoData->commonComponentsLinks.end()) // invalid link
        return false;
      auto it = metaInfoData->componentsLinks.find(name);
      if (it != metaInfoData->componentsLinks.end()) // already defined, same?
        return metaInfoData->componentsLinks[name] == itc->second;
      metaInfoData->componentsLinks[name] = itc->second;
      return true;
    }
    case MetaInfoType::TEMPLATE:
    {
      auto itc = metaInfoData->commonTemplatesLinks.find(link);
      if (itc == metaInfoData->commonTemplatesLinks.end()) // invalid link
        return false;
      auto it = metaInfoData->templatesLinks.find(name);
      if (it != metaInfoData->templatesLinks.end()) // already defined, same?
        return metaInfoData->templatesLinks[name] == itc->second;
      metaInfoData->templatesLinks[name] = itc->second;
      return true;
    }
  }
  return false;
}

void TemplateDBInfo::addMetaComp(const char *templ_name, const char *comp_name)
{
  if (!metaInfoData || !templ_name || !comp_name)
    return;
  const uint64_t key = mum_hash_step(ecs_str_hash<64>(templ_name), ecs_str_hash<64>(comp_name));
  metaInfoData->templatesComps.insert(key);
}

const DataBlock *TemplateDBInfo::getTemplateMetaInfo(const char *templ_name) const
{
  if (!metaInfoData || !templ_name)
    return nullptr;
  auto it = metaInfoData->templatesLinks.find(templ_name);
  if (it == metaInfoData->templatesLinks.end())
    return nullptr;
  return metaInfoData->metaDataBlock.getBlock(it->second);
}

const DataBlock *TemplateDBInfo::getComponentMetaInfo(const char *comp_name) const
{
  if (!metaInfoData || !comp_name)
    return nullptr;
  auto it = metaInfoData->componentsLinks.find(comp_name);
  if (it == metaInfoData->componentsLinks.end())
    return nullptr;
  return metaInfoData->metaDataBlock.getBlock(it->second);
}

bool TemplateDBInfo::hasComponentMetaInfo(const char *templ_name, const char *comp_name) const
{
  if (!metaInfoData || !templ_name || !comp_name)
    return false;
  const uint64_t key = mum_hash_step(ecs_str_hash<64>(templ_name), ecs_str_hash<64>(comp_name));
  return metaInfoData->templatesComps.count(key) > 0;
}


bool Template::isEqual(const Template &t) const
{
  if (t.components.size() != components.size())
    return false;
  if (templAllFlags() != t.templAllFlags() || ignoredSet() != t.ignoredSet())
    return false;
  if (trackedSet() != t.trackedSet() || replicatedSet() != t.replicatedSet())
    return false;

  if (!std::equal(components.begin(), components.end(), t.components.begin(), [&](auto const& a, auto const& b)
  {
      if (a.first != b.first)
          return false;
      if (a.second == b.second)
          return true;
      const type_index_t typeIdx = a.second.isNull() ? b.second.getTypeId() : a.second.getTypeId();
      // this is the only difference from default comparison
      // we assume, that templates with shared components ARE same.
      // that is not always true, but we can't differentiate change in shared component made by Entity and difference in templates
      //  (and that is universally used pattern for all shared components, as deferred common initialization)
      const bool isShared =
          (g_entity_mgr->getComponentTypes().getTypeInfo(typeIdx).flags & COMPONENT_TYPE_CREATE_ON_TEMPL_INSTANTIATE);
      return isShared;
  }))
      return false;
  return true;
}

}; // namespace ecs
