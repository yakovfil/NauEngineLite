// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#pragma once

#include <array>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "nau/osApiWrappers/dag_stackHlp.h"
#include "nau/threading/critical_section.h"
//#include <util/dag_string.h>



namespace drv3d_dx12
{
namespace debug
{
namespace call_stack
{
namespace full_stack
{
inline constexpr uint32_t max_call_stack_depth = 32;
using CallStack = std::array<void*, max_call_stack_depth>;

struct CallStackHasher
{
  // very simple hashing, each pointer is the hash value and we simply combine all of them.
  inline size_t operator()(const CallStack &call_stack) const
  {
    size_t value = 0;
    for (auto &p : call_stack)
    {
      value ^= reinterpret_cast<size_t>(p) + 0x9e3779b9 + (value << 6) + (value >> 2);
    }

    return value;
  }
};

struct CommandData
{
  const CallStack *callStack;
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
  }
  const char *getLastCommandName() const { return lastCommandName; }
};

class Generator
{
  dag::CriticalSection mutex;
  std::unordered_set<CallStack, CallStackHasher> callstacks;

  public:
  void configure(const nau::DataBlock *) {}
  const CallStack *captureCallStack(uint32_t offset)
  {
    CallStack callstack;
    stackhlp_fill_stack(callstack.data(), callstack.size(), offset);

    dag::CSAutoLock lock{mutex};
    auto ref = callstacks.insert(callstack).first;
    return &*ref;
  }

public:
  CommandData generateCommandData() { return {captureCallStack(2)}; }
};

class Reporter
{
    std::unordered_map<const CallStack*, nau::string> callStackCache;
    const nau::string& doResolve(const CommandData& data)
    {
        auto ref = callStackCache.find(data.callStack);
        if (end(callStackCache) == ref)
        {
            char strBuf[4096];
            auto name = stackhlp_get_call_stack(strBuf, sizeof(strBuf) - 1, data.callStack->data(), data.callStack->size());
            strBuf[sizeof(strBuf) - 1] = '\0';
            ref = callStackCache.emplace(data.callStack, name).first;
        }
        return ref->second;
    }

public:
  void report(const CommandData &data)
  {
    if (!data.callStack)
    {
      return;
    }
    NAU_LOG_DEBUG(doResolve(data).c_str());
  }

  void append(nau::string &buffer, const char *prefix, const CommandData &data)
  {
    if (!data.callStack)
    {
      return;
    }
    buffer.append(prefix);
    auto &str = doResolve(data);
    buffer.append(str);
  }

  nau::string_view resolve(const CommandData &data) { return doResolve(data); }
};
} // namespace full_stack
} // namespace call_stack
} // namespace debug
} // namespace drv3d_dx12