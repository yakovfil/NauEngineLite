// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#pragma once

#include <daECS/core/schemelessEvent.h>

#include <optional>

namespace danet
{
class BitStream;
}

namespace ecs
{

struct SchemelessEvent;
typedef std::optional<SchemelessEvent> MaybeSchemelessEvent;
void serialize_to(const SchemelessEvent &, danet::BitStream &bs);
MaybeSchemelessEvent deserialize_from(const danet::BitStream &bs);

} // namespace ecs
