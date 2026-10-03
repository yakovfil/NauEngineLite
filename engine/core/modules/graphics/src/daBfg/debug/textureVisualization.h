// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#pragma once

#include <imgui.h>

#include <string>

#include "nau/3d/dag_resPtr.h"
#include "render/daBfg/detail/nodeNameId.h"
#include "render/daBfg/detail/resNameId.h"

namespace dabfg
{
struct InternalRegistry;
class NodeHandle;
} // namespace dabfg

void fg_texture_visualization_imgui_line(dabfg::ResNameId &selectedResId, const dabfg::InternalRegistry &registry,
  dabfg::NodeNameId selectedPrecedingNode);

void update_fg_debug_tex(dabfg::NodeNameId selectedPrecedingNode, dabfg::NodeNameId *selectedFollowingNode,
  dabfg::ResNameId selectedResId, UniqueTex &copiedTexture, dabfg::InternalRegistry &registry,
  dabfg::NodeHandle &debugTextureCopyNode);
