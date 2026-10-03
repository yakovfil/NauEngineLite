// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#pragma once

#include <memory>

#include "drv3d_commonCode/ngx_wrapper_base.h"

namespace drv3d_dx12
{
class XessWrapperImpl;

class XessWrapper
{
public:
  bool xessInit(void *device);
  bool xessCreateFeature(int quality, uint32_t target_width, uint32_t target_height);
  bool xessShutdown();

  XessState getXessState() const;
  void getXeSSRenderResolution(int &w, int &h) const;
  bool evaluateXess(void *context, const void *dlss_params);
  void setVelocityScale(float x, float y);
  bool isXessQualityAvailableAtResolution(uint32_t target_width, uint32_t target_height, int xess_quality) const;
  XessWrapper();
  ~XessWrapper();

private:
    std::unique_ptr<XessWrapperImpl> pImpl;
};
} // namespace drv3d_dx12
