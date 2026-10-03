// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#pragma once

#include <functional>
#include <ska_hash_map/flat_hash_map2.hpp>
#include <string>
#include <string_view>
#include <vector>

#include "nau/utils/dag_oaHashNameMap.h"
//#include <dag/dag_vector.h>


namespace drv3d_dx12::debug::event_marker
{
namespace core
{
class Tracker
{
  static constexpr char separator = '/';

  dag::OAHashNameMap<false> nameTable;
  using FullPathIdx = uint32_t;
  using NameIdx = uint32_t;
  const uint32_t INVALID_IDX = ~0u;
  struct EventPathEntry
  {
    FullPathIdx parent;
    NameIdx name;
    friend bool operator==(const EventPathEntry &l, const EventPathEntry &r) { return l.parent == r.parent && l.name == r.name; }
  };
  std::vector<EventPathEntry> pathTable;
  std::vector<std::string> fullpathTable;

  struct EventPathEntryHasher
  {
      size_t operator()(const EventPathEntry& e) const
      {
          return std::hash<uintptr_t>{}((e.parent << 16) | e.name);
      }
  };

  ska::flat_hash_map<EventPathEntry, FullPathIdx, EventPathEntryHasher> pathLookup;
  FullPathIdx currentPath = INVALID_IDX;
  NameIdx lastMarker = INVALID_IDX;

public:
    std::string_view beginEvent(std::string_view name)
    {
        NameIdx nameId = nameTable.addNameId(name.data(), name.length());

        const EventPathEntry previousEntry = {currentPath, nameId};

        if (pathLookup.find(previousEntry) == pathLookup.end())
        {
            currentPath = pathTable.size();
            pathTable.push_back(previousEntry);
            pathLookup[previousEntry] = currentPath;
            if (previousEntry.parent != INVALID_IDX)
            {
                fullpathTable.resize(pathTable.size());
                fullpathTable.back().reserve(fullpathTable[previousEntry.parent].length() + 1 + name.length());
                fullpathTable.back().assign(fullpathTable[previousEntry.parent].data(), fullpathTable[previousEntry.parent].length());
                fullpathTable.back().push_back(separator);
            }
            else
            {
                fullpathTable.resize(pathTable.size());
                fullpathTable.back().assign(name.data(), name.length());
            }
        }
        else
        {
            currentPath = pathLookup[previousEntry];
        }

        return {nameTable.getName(nameId), name.length()};
    }

  void endEvent()
  {
    if (currentPath != INVALID_IDX)
    {
      currentPath = pathTable[currentPath].parent;
    }
  }

  std::string_view marker(std::string_view name)
  {
    lastMarker = nameTable.addNameId(name.data(), name.length());
    return {nameTable.getName(lastMarker), name.length()};
  }

  std::string_view currentEventPath() const
  {
    if (currentPath != INVALID_IDX)
    {
      return {fullpathTable[currentPath].data(), fullpathTable[currentPath].length()};
    }
    else
    {
      return {};
    }
  }

  std::string_view currentEvent() const
  {
    if (currentPath != INVALID_IDX)
    {
      const uint32_t nameId = pathTable[currentPath].name;
      const char *eventName = nameTable.getName(nameId);
      if (!eventName)
        return {};
      return {eventName, strlen(eventName)};
    }
    else
    {
      return {};
    }
  }

  std::string_view currentMarker() const
  {
    if (lastMarker != INVALID_IDX)
    {
      const char *markerName = nameTable.getName(lastMarker);
      if (!markerName)
        return {};
      return {markerName, strlen(markerName)};
    }
    else
    {
      return {};
    }
  }

  constexpr bool isPersistent() const { return true; }
};
} // namespace core

namespace null
{
class Tracker
{
public:
    std::string_view beginEvent(std::string_view name)
    {
        return name;
    }
  void endEvent() {}
  std::string_view marker(std::string_view name)
  {
      return name;
  }
  constexpr std::string_view currentEventPath() const
  {
      return {};
  }
  constexpr std::string_view currentEvent() const
  {
      return {};
  }
  constexpr std::string_view currentMarker() const
  {
      return {};
  }
  constexpr bool isPersistent() const { return false; }
};
} // namespace null

#if DX12_TRACK_ACTIVE_DRAW_EVENTS
using namespace core;
#else
using namespace null;
#endif
} // namespace drv3d_dx12::debug::event_marker
