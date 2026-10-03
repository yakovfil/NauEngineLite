// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "render_list.h"

#include <vector>

#include "nau/shaders/shader_globals.h"

nau::RenderList::RenderList(std::vector<RenderList::Ptr>&& vec)
{
    for (auto rendList : vec)
    {
        m_entities.insert(m_entities.end(), rendList->m_entities.begin(), rendList->m_entities.end());
    }
}
