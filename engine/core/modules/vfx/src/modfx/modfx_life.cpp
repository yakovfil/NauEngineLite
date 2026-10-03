// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#include "modfx_life.h"

#include <algorithm>

#include "math/vfx_random.h"
#include "nau/math/math.h"

namespace nau::vfx::modfx
{
    void life::modfx_life_init(int rnd_seed, float& life_norm, const settings::FxLife& life)
    {
        if (life.part_life_rnd_offset > 0.05f)
        {
            life_norm = (std::min(life.part_life_rnd_offset, life.part_life_max) / life.part_life_max) * vfx::math::dafx_frnd(rnd_seed);
        }
    }

    void life::modfx_life_sim(int rnd_seed, float life_limit_rcp, float dt, const settings::FxLife& life, float& o_life_norm)
    {
        if (life.part_life_min != life.part_life_max)
        {
            float ratio = life.part_life_max / life.part_life_min;
            life_limit_rcp *= nau::math::lerp(1.0f, ratio, vfx::math::dafx_frnd(rnd_seed));
        }

        o_life_norm += dt * life_limit_rcp;
    }
}
