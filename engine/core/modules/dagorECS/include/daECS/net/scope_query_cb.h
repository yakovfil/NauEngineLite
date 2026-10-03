// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#pragma once

#include <functional>

// called once per update before generating packet in order to query scope status of all objects
namespace net
{
class Connection;
typedef std::function<void(Connection*)> scope_query_cb_t;
} // namespace net
