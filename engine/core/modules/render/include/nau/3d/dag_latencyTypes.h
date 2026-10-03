// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#pragma once

// #include <util/dag_globDef.h>
#include <algorithm>
#include <limits>

namespace lowlatency
{
enum class LatencyMarkerType
{
  SIMULATION_START,
  SIMULATION_END,
  RENDERRECORD_START,
  RENDERRECORD_END,
  RENDERSUBMIT_START,
  RENDERSUBMIT_END,
  PRESENT_START,
  PRESENT_END,
  INPUT_SAMPLE_FINISHED,
  TRIGGER_FLASH
};

struct Statistics
{
    float min = std::numeric_limits<float>::infinity();
    float max = -std::numeric_limits<float>::infinity();
    float avg = 0;
    void apply(float value)
    {
        min = std::min(min, value);
        max = std::max(max, value);
        avg += value;
    }
  void finalize(uint32_t count) // count is not stored inside the struct
  {
    if (count > 0)
      avg /= float(count);
  }
};

struct LatencyData
{
  // times are measured in ms
  // time points are relative to simStartTime
  uint32_t frameCount = 0;

  // These are currently measured by only the NVIDIA low latency sdk
  // but they can be implemented without the sdk
  // implementation should be placed in the set_marker function
  Statistics gameLatency;
  Statistics inputSampleTime;
  Statistics simEndTime;
  Statistics renderSubmitStartTime;
  Statistics renderSubmitEndTime;
  Statistics presentStartTime;
  Statistics presentEndTime;

  // these are only available with NVIDIA low latency sdk
  Statistics gameToRenderLatency;
  Statistics renderLatency;
  Statistics driverStartTime;
  Statistics driverEndTime;
  Statistics osRenderQueueStartTime;
  Statistics osRenderQueueEndTime;
  Statistics gpuRenderStartTime;
  Statistics gpuRenderEndTime;
};

using LatencyModeFlag = uint8_t;

enum LatencyMode : LatencyModeFlag
{
  LATENCY_MODE_OFF = 0,

  // NVIDIA low latency sdk
  LATENCY_MODE_NV_ON = 1,    // present block + sleep before sampling input
  LATENCY_MODE_NV_BOOST = 2, // ON + gpu clock control

  // other modes
  LATENCY_MODE_EXPERIMENTAL = 4 // extra custom sleep (+ NV_BOOST if available)
};
} // namespace lowlatency
