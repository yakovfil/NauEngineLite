// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#pragma once

#include <daECS/core/entityId.h>
#include <generic/dag_span.h>

#include <optional>
#include <string>

#include "nau/utils/fixed_function.h"

namespace bind_dascript
{
struct DasEvent;
}
class SqModules;

namespace net
{
class IConnection;
void register_dasevents(SqModules *module_mgr);
void send_dasevent(ecs::EntityManager*, bool broadcast, const ecs::EntityId eid, bind_dascript::DasEvent* evt, const char* event_name, std::optional<nau::ConstSpan<net::IConnection*>> explicitConnections, nau::FixedFunction<sizeof(void*), std::string()> debug_msg);
} // namespace net
