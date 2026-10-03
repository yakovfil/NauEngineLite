// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#include <daECS/core/internal/eventsDb.h>

#include <string>
#include <utility>
namespace ecs
{

volatile uint32_t EventInfoLinkedList::generation = 0;
EventInfoLinkedList *EventInfoLinkedList::tail = nullptr;
EventInfoLinkedList *EventInfoLinkedList::registered_tail = nullptr;
static const char *EV_CAST_STR_TYPES[] = {"Unknowncast", "Unicast", "Broadcast", "Bothcast"};

void EventsDB::dump() const
{
  NAU_UNUSED(EV_CAST_STR_TYPES);
  for (const auto &em : eventsMap)
  {
    const auto &e = eventsInfo[em.second];
    NAU_UNUSED(e);
    // ECS_LOG("registered {} {}Event <{}|0x{}> sz {}",
    //   std::get<EventsDB::EVENT_FLAGS>(e) & EVFLG_DESTROY ? "non-trivially-destructible" : "",
    //   EV_CAST_STR_TYPES[std::get<EventsDB::EVENT_FLAGS>(e) & EVFLG_CASTMASK], std::get<EventsDB::EVENT_NAME>(e).c_str(),
    //   std::get<EventsDB::EVENT_TYPE>(e), std::get<EventsDB::EVENT_SIZE>(e));
  }

  const event_scheme_t *schemes = eventsScheme.get<event_scheme_t>();
  for (const auto &em : eventsMap)
  {
    const event_scheme_hash_t hash = eventsScheme.get<event_scheme_hash_t>()[em.second];
    if (hash == invalid_event_scheme_hash)
      continue;
    const event_scheme_t &scheme = schemes[em.second];
    ECS_LOG("Event '{}' hash=0x{} scheme:", std::get<EventsDB::EVENT_NAME>(eventsInfo[em.second]).c_str(), hash);
    const auto* names = scheme.get<std::string>();
    const auto *types = scheme.get<component_type_t>();
    const auto *offsets = scheme.get<uint8_t>();
    for (int i = 0, n = (int)scheme.size(); i < n; ++i)
    {
        ECS_LOG("  field #%i '{}' : 0x{} offset={}", i, names[i].c_str(), types[i], offsets[i]);
      NAU_UNUSED(names);
      NAU_UNUSED(types);
      NAU_UNUSED(offsets);
    }
  }
}

bool EventsDB::registerEvent(event_type_t type, event_size_t sz, event_flags_t flags, const char *name, destroy_event *d,
  move_out_event *m)
{
  if (sz >= Event::max_event_size || sz < sizeof(Event))
  {
    logerr("Can't register Event <0x{}|{}> of size <{}>, size not in [{},{})", type, name, sz, sizeof(Event), +Event::max_event_size);
    return false;
  }
  if ((flags & EVFLG_DESTROY) && (!d || !m))
  {
    logerr("Can't register Event <0x{}|{}>, which requires Destroy, but doesn't provides destroy/move functions", type, name);
    if (!d) // if there is destroy function, it is possible to work without move. Event just can't be sent to loading entity.
      return false;
  }
  if ((d || m) && !(flags & EVFLG_DESTROY))
  {
      logwarn("Event <0x{}|{}> provides destroy/move functions but is trivially destructible", type, name);
  }
  int evCast = flags & EVFLG_CASTMASK;
  if (!(evCast == EVCAST_UNICAST || evCast == EVCAST_BROADCAST))
  {
      logerr("Event <0x{}|{}> registered as {} instead of Unicast or Broadcast", type, name, EV_CAST_STR_TYPES[evCast]);
  }

  event_id_t id = findEvent(type);
  const bool ret = (id != invalid_event_id);
  if (id == invalid_event_id)
  {
    id = eventsInfo.size();
    eventsMap[type] = id;
    eventsInfo.emplace_back(std::move(sz), std::move(flags), std::move(type), std::string(name ? name : "#UnknownEvent#"));
    event_scheme_hash_t schemeHash = invalid_event_scheme_hash;
    eventsScheme.emplace_back(std::move(schemeHash), std::move(event_scheme_t{}));
  }
  else
  {
    NAU_ASSERT(!name || eventsInfo.get<EVENT_NAME>()[id] == name);
    if (name && eventsInfo.get<EVENT_NAME>()[id] != name)
    {
      logerr("Event hash collision found <0x{}|{}> collides with {}", type, eventsInfo.get<EVENT_NAME>()[id].c_str(), name);
      return false;
    }
    // change info. We don't allow to change name, though.
    if (eventsInfo.get<EVENT_SIZE>()[id] != sz || eventsInfo.get<EVENT_FLAGS>()[id] != flags)
    {
      logerr("Event (0x{}|{}) has changed it size {} -> {} or flags {} -> {}", type,
        name ? name : eventsInfo.get<EVENT_NAME>()[id].c_str(), eventsInfo.get<EVENT_SIZE>()[id], sz,
        eventsInfo.get<EVENT_FLAGS>()[id], flags);
    }
    else
        logmessage((flags & EVFLG_SCHEMELESS) ? ::nau::diag::LogLevel::Warning : ::nau::diag::LogLevel::Error, "event ({}|0x{}) registered twice",
        name ? name : eventsInfo.get<EVENT_NAME>()[id].c_str(), type);

    eventsInfo.get<EVENT_SIZE>()[id] = sz;
    eventsInfo.get<EVENT_FLAGS>()[id] = flags;
  }

  if (d)
    eventsDestroyMap[type] = d;
  if (m)
    eventsMoveMap[type] = m;
  return ret;
}

bool EventsDB::registerEventScheme(event_type_t type, event_scheme_hash_t scheme_hash, event_scheme_t &&scheme)
{
  const event_id_t eventId = findEvent(type);
  if (eventId != invalid_event_id)
  {
    eventsScheme.get<event_scheme_hash_t>()[eventId] = scheme_hash;
    eventsScheme.get<event_scheme_t>()[eventId] = std::move(scheme);
    return true;
  }
  return false;
}

void EventsDB::validateInternal()
{
  EventInfoLinkedList::remove_if([&](EventInfoLinkedList *ei) {
    ei->next = EventInfoLinkedList::registered_tail;
    EventInfoLinkedList::registered_tail = ei;
    registerEvent(ei->getEventType(), ei->getEventSize(), ei->getEventFlags(), ei->getEventName(), ei->getDestroyFunc(),
      ei->getMoveOutFunc());
    return true;
  });
}

// out of line
void EventsDB::destroy(Event &e) const
{
  DAECS_EXT_FAST_ASSERT(e.getFlags() & EVFLG_DESTROY);
  auto it = eventsDestroyMap.find(e.getType());
  if (it == eventsDestroyMap.end())
  {
    logerr("event 0x{}|{} has no registered destroy func", e.getType(), e.getName());
    return;
  }
  it->second(e);
}

void EventsDB::moveOut(void *__restrict to, Event &&e) const
{
  DAECS_EXT_FAST_ASSERT(e.getFlags() & EVFLG_DESTROY);
  auto it = eventsMoveMap.find(e.getType());
  DAECS_EXT_ASSERTF_RETURN(it != eventsMoveMap.end(), , "0x{}|{}", e.getType(), e.getName());
  it->second(to, std::move(e));
}


} // namespace ecs
