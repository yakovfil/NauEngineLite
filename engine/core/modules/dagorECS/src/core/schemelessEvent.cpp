// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#include <daECS/core/componentTypes.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/schemelessEvent.h>

#include <utility>

namespace ecs
{

SchemelessEvent::SchemelessEvent(ecs::event_type_t tp, ecs::Object &&data_) :
  SchemelessEvent(tp, EVFLG_DESTROY | EVFLG_SERIALIZE | EVFLG_SCHEMELESS)
{
    data = std::move(data_);
}

SchemelessEvent::SchemelessEvent(ecs::event_type_t tp, ecs::event_flags_t flags_) : ecs::Event(tp, sizeof(SchemelessEvent), flags_)
{
#if NAU_DEBUG
  ecs::EventsDB::event_id_t eventId = g_entity_mgr->getEventsDb().findEvent(tp);
  if (eventId != ecs::EventsDB::invalid_event_id)
  {
    ecs::event_flags_t eventFlag = g_entity_mgr->getEventsDb().getEventFlags(eventId);
    flags |= eventFlag & ecs::EVFLG_CASTMASK;
  }
  else
  {
    logerr("attempt to create unregistered SchemelessEvent with event type <0x{}>", tp);
    flags |= ecs::EVCAST_BOTH;
  }
#else
  flags |= ecs::EVCAST_BOTH;
#endif
}

void SchemelessEvent::destroy(Event &e)
{
#if NAU_DEBUG
  NAU_ASSERT_RETURN(ecs::is_schemeless_event(e), );
#endif
  static_cast<SchemelessEvent &>(e).~SchemelessEvent();
}

void SchemelessEvent::move_out(void *__restrict allocateAt, Event &&from)
{
#if NAU_DEBUG
  if (!ecs::is_schemeless_event(from))
  {
    logerr("0x{} is not schemeless event", from.getType());
    memcpy(allocateAt, &from, sizeof(Event));
    memset(allocateAt, 0, 4); // overwrite type
    return;
  }
#endif
  new (allocateAt/*, _NEW_INPLACE*/) SchemelessEvent(static_cast<SchemelessEvent &&>(from));
  // work around, optimization, to prevent further call for destructor when event is moving from main queue to loadingEntities queue
  // destructor in that case WON'T be called, which is technically UB (but it is optimal behaviour, since we know there is nothing to
  // be done in destructor anyway)
  static_cast<SchemelessEvent &>(from).flags &= ~EVFLG_DESTROY;
}

} // namespace ecs
