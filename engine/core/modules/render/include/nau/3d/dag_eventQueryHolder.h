// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#pragma once

#include <memory>

#include "nau/3d/dag_drv3d.h"

struct EventFenceDeleter
{
  void operator()(D3dEventQuery *ptr) { ptr ? d3d::release_event_query(ptr) : (void)0; }
};
using EventQueryHolder = std::unique_ptr<D3dEventQuery, EventFenceDeleter>;
