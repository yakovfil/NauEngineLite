// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#pragma once

#include <vector>

#include "nau/3d/dag_resId.h"
//#include <generic/dag_tabFwd.h>

enum
{
  TEXTAG_LAND,
  TEXTAG_RENDINST,
  TEXTAG_DYNMODEL,
  TEXTAG_FX,
  TEXTAG_USER_START,

  TEXTAG__COUNT = 16
};

struct TexTagInfo
{
  int texCount;
  int loadGeneration;
};


//! begins scope to mark all resolved (get_managed_texture_id) textures with tag; may be nested; thread-local
void textag_mark_begin(int textag);
//! ends scope to mark all resolved textures with tag
void textag_mark_end();

//! clears tag from all previously marked textures
void textag_clear_tag(int textag);

//! returns state info struct for specified tag
const TexTagInfo &textag_get_info(int textag);

//! returns list of textures that are marked with tag
void textag_get_list(int textag, std::vector<TEXTUREID>& out_list, bool skip_unused = true);
