// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#include "modfx_sim.h"

#include <algorithm>

#include "modfx_color.h"
#include "modfx_life.h"
#include "modfx_radius.h"
#include "modfx_texture.h"
#include "modfx_velocity.h"

namespace nau::vfx::modfx
{
    void sim::modfx_apply_sim(modfx::ModfxRenData& rdata, modfx::ModfxSimData& sdata, float dt, const settings::FxLife& life, const settings::FxRadius& radius, const settings::FxVelocity& velocity, const settings::FxColor& color, const settings::FxTexture& texture)
    {
        const float lifeTimeRCP = 1.0f / (life.part_life_max != 0.0f ? life.part_life_max : 1.0f);
        life::modfx_life_sim(sdata.rnd_seed, lifeTimeRCP, dt, life, sdata.life_norm);

        bool dead = sdata.life_norm >= 1.0f;
        sdata.life_norm = std::clamp(sdata.life_norm, 0.0f, 1.0f);

        if (radius.enabled)
        {
            radius::modfx_radius_sim(sdata.rnd_seed, sdata.life_norm, dt, radius);
        }

        if (velocity.enabled)
        {
            velocity::modfx_velocity_sim(sdata.rnd_seed, sdata.life_norm, dt, rdata.radius, rdata.pos, rdata.pos_offset, sdata.velocity, velocity);
            rdata.pos += rdata.pos_offset;
        }

        if (color.enabled)
        {
            color::modfx_color_sim(sdata.rnd_seed, sdata.life_norm, rdata.color, color);
        }

        if (texture.enabled)
        {
            texture::modfx_texture_sim(sdata.life_norm, rdata.frame_idx, texture);
        }

        rdata.life_norm = sdata.life_norm;

        if (dead)
            rdata.radius = 0;
    }
}

