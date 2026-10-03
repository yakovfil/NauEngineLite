// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#pragma once

#include "nau/dataBlock/dag_dataBlock.h"
//#include <util/dag_string.h>
#include <string_view>

namespace drv3d_dx12
{
namespace debug
{
namespace call_stack
{
namespace null
{
struct CommandData
{};

class ExecutionContextDataStore
{
public:
  CommandData getCommandData() const { return {}; }
  void setCommandData(const CommandData &, const char *) {}
  const char *getLastCommandName() const { return ""; }
};

class Generator
{
public:
  void configure(const nau::DataBlock *) {}
  CommandData generateCommandData() const { return {}; }
};

class Reporter
{
public:
  void report(const CommandData &) {}

  void append(nau::string &, const char *, const CommandData &) {}
  nau::string_view resolve(const CommandData &) { return {}; }
};
} // namespace null
} // namespace call_stack
} // namespace debug
} // namespace drv3d_dx12
