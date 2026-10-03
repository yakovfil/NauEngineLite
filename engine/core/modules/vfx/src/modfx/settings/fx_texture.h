// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#pragma once

#include <string>

#include "nau/dataBlock/dag_dataBlock.h"
#include "nau/math/math.h"

namespace nau::vfx::modfx::settings
{
    struct FxTexture
    {
        FxTexture() :
            enabled(false),
            tex_0_name(std::string()),
            frames_x(1),
            frames_y(1),
            current_frame(0),
            frame_scale(1.0f),
            start_frame_min(0),
            start_frame_max(1),
            random_flip_x(true),
            random_flip_y(true),
            looped(false)
        {
        }

        // TODO Move to common methods
        void save(nau::DataBlock* blk) const
        {
            blk->addBool("enabled", enabled);
            blk->addInt("frames_x", frames_x);
            blk->addInt("frames_y", frames_y);
            blk->addInt("current_frame", current_frame);
            blk->addReal("frame_scale", frame_scale);
            blk->addInt("start_frame_min", start_frame_min);
            blk->addInt("start_frame_max", start_frame_max);
            blk->addBool("random_flip_x", random_flip_x);
            blk->addBool("random_flip_y", random_flip_y);
            blk->addBool("looped", looped);
        }

        bool load(const nau::DataBlock* blk)
        {
            if (!blk)
            {
                return false;
            }

            enabled = blk->getBool("enabled", false);
            frames_x = blk->getInt("frames_x", 1);
            frames_y = blk->getInt("frames_y", 1);
            current_frame = blk->getInt("current_frame", 0);
            frame_scale = blk->getReal("frame_scale", 1.0f);
            start_frame_min = blk->getInt("start_frame_min", 0);
            start_frame_max = blk->getInt("start_frame_max", 1);
            random_flip_x = blk->getBool("random_flip_x", true);
            random_flip_y = blk->getBool("random_flip_y", true);
            looped = blk->getBool("looped", false);

            return true;
        }

        bool enabled;

        std::string tex_0_name;

        int frames_x;
        int frames_y;
        int current_frame;

        float frame_scale;

        int start_frame_min;
        int start_frame_max;

        bool random_flip_x;
        bool random_flip_y;

        bool looped;
    };
}  // namespace nau::vfx::modfx::settings