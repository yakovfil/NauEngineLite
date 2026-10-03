// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#pragma once

#include <vector>

#include "nau/3d/dag_drv3dConsts.h"
#include "nau/3d/dag_drvDecl.h"
#include "nau/dataBlock/dag_dataBlock.h"
#include "nau/string/string.h"

struct GpuVideoSettings
{
  DriverCode drvCode;
  bool disableNvTweaks = false;
  bool disableAtiTweaks = false;
  bool ignoreOutdatedDriver = false;
  bool configCompatibilityMode = false;
  bool allowDx10Fallback = false;
  std::vector<nau::string> oldHardwareList;
  bool adjustVideoSettings = false;
  int lowVideoMemMb = 0;
  int ultraLowVideoMemMb = 0;
  int lowSystemMemAtMb = 0;
  int ultralowSystemMemAtMb = 0;
};

struct GpuUserConfig
{
  int primaryVendor = D3D_VENDOR_NONE;
  uint32_t physicalFrameBufferSize = 0;
  uint32_t deviceId = 0;
  bool vendorAAisOn = false;
  bool outdatedDriver = false;
  bool fallbackToCompatibilty = false;
  bool disableUav = false;
  union
  {
    bool integrated;
    bool usedSlowIntegrated = false;
  };
  bool usedSlowIntegratedSwitchableGpu = false;
  bool gradientWorkaroud = false;
  bool disableTexArrayCompression = false;
  bool disableSbuffers = false;
  bool disableMeshStreaming = false;
  bool disableDepthCopyResource = false;
  bool forceDx10 = false;
  bool hardwareDx10 = false;
  bool oldHardware = false;
  uint32_t driverVersion[4] = {0, 0, 0, 0};

  bool lowMem = false;
  bool ultraLowMem = false;
  uint32_t videoMemMb = 0;
  int freePhysMemMb = 0;
  int64_t freeVirtualMemMb = 0;
  int64_t totalVirtualMemMb = 0;

  nau::string generateDriverVersionString() const;
};

void d3d_read_gpu_video_settings(const nau::DataBlock &blk, GpuVideoSettings &out_video);
const GpuUserConfig &d3d_get_gpu_cfg();

void d3d_apply_gpu_settings(const GpuVideoSettings &video);
void d3d_apply_gpu_settings(const nau::DataBlock &blk);
