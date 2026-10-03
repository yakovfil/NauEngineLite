// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#pragma once

#include <string>
#include <unordered_map>

#include "nau/dataBlock/dag_dataBlock.h"
//#include <debug/dag_log.h>
#include <string_view>

#if COMMANDS_STORE_RETURN_ADDRESS

    #include <drv_returnAddrStore.h>
    #include <osApiWrappers/dag_stackHlp.h>

    #include <unordered_map>

namespace drv3d_dx12
{
namespace debug
{
namespace call_stack
{
namespace return_address
{
struct CommandData
{
  const void *returnAddress;
};

class ExecutionContextDataStore
{
  const char *lastCommandName = nullptr;
  CommandData data{};

public:
  const CommandData &getCommandData() const { return data; }
  void setCommandData(const CommandData &update, const char *name)
  {
    data = update;
    lastCommandName = name;
    NAU_ASSERT(data.returnAddress,
      "%s had no returnAddress stored.\n"
      "Possible issues: Cmd wasn't created with the make_command function. \n"
      "STORE_RETURN_ADDRESS() call is missing on the d3d interface.",
      name);
  }
  const char *getLastCommandName() const { return lastCommandName; }
};

class Generator
{
public:
  void configure(const DataBlock *) {}
  CommandData generateCommandData() const { return {ScopedReturnAddressStore::get_threadlocal_saved_address()}; }
};

class Reporter
{
  // cache of return address pointers to avoid looking it up over and over again
  std::unordered_map<const void*, std::string> addressCache;

  const std::string& doResolve(const CommandData& data)
  {
    auto ref = addressCache.find(data.returnAddress);
    if (end(addressCache) == ref)
    {
      char strBuf[4096];
      auto name = stackhlp_get_call_stack(strBuf, sizeof(strBuf) - 1, &data.returnAddress, 1);
      strBuf[sizeof(strBuf) - 1] = '\0';
      ref = addressCache.emplace(data.returnAddress, name).first;
    }
    return ref->second;
  }

public:
  void report(const CommandData &data)
  {
    if (!data.returnAddress)
    {
      return;
    }
    NAU_LOG_DEBUG(doResolve(data).c_str());
  }

  void append(String &buffer, const char *prefix, const CommandData &data)
  {
    if (!data.returnAddress)
    {
      return;
    }
    buffer.append(prefix);
    auto &str = doResolve(data);
    buffer.append(str.data(), str.length());
  }

  std::string_view resolve(const CommandData& data)
  {
      return doResolve(data);
  }
};
} // namespace return_address
} // namespace call_stack
} // namespace debug
} // namespace drv3d_dx12

#else

namespace drv3d_dx12
{
namespace debug
{
namespace call_stack
{
namespace return_address
{
struct CommandData
{};

class ExecutionContextDataStore
{
public:
  CommandData getCommandData() const { return {}; }
  void setCommandData(const CommandData &, const char *) {}
}; // namespace return_address

class Generator
{
public:
  void configure(const nau::DataBlock *)
  {
    NAU_LOG_DEBUG("DX12: debug::call_stack::return_address using null implementation! No return addresses are available!");
  }
  CommandData generateCommandData() const { return {}; }
  const char *getLastCommandName() const { return ""; }
};

class Reporter
{
public:
  void report(const CommandData &) {}
  void append(nau::string &, const char *, const CommandData &) {}
  nau::string_view resolve(const CommandData &) { return {}; }
};
} // namespace return_address
} // namespace call_stack
} // namespace debug
} // namespace drv3d_dx12


#endif
