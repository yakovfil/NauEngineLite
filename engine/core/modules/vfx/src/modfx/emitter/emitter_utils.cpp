// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#include "emitter_utils.h"

#include <algorithm>

namespace nau::vfx::modfx
{
    void emitter_utils::create_emitter_state(EmitterState& state, const EmitterData& data, int elem_limit, float emission_factor)
    {
        emission_factor = std::max(emission_factor, data.minEmissionFactor);
        float delay = data.delay;
        if (data.type == EmitterType::LINEAR)  // we only need that for linear ticks
            delay *= emission_factor;          // we need to compensate for altered dt

        state.emissionLimit = elem_limit;
        state.localTickRate = 1.0f;
        state.totalTickRate = state.localTickRate;
        state.globalLifeLimit = std::max(data.globalLifeLimitMin, 0.0f);
        state.globalLifeLimit += state.globalLifeLimit > 0 ? delay : 0;
        state.globalLifeLimitRef = state.globalLifeLimit;

        if (data.type == EmitterType::FIXED)
        {
            state.isDistanceBased = false;
            state.lifeLimit = 150; // we need a large fixed lifetime
            state.batchSize = std::max((int)(data.fixedData.count * emission_factor), 1);
            state.cyclesCount = 1;

            state.spawnTick = 0;
            state.shrinkTick = 0;
            state.tickLimit = 0;
        }
        else if (data.type == EmitterType::BURST)
        {
            state.isDistanceBased = false;
            state.lifeLimit = data.burstData.lifeLimit;
            state.batchSize = std::max((int)(data.burstData.countMax * emission_factor), 1);
            state.cyclesCount = data.burstData.cycles > 0 ? data.burstData.cycles : -1;

            state.tickLimit = data.burstData.period;

            state.spawnTick = state.tickLimit - delay;
            state.shrinkTick = state.tickLimit - data.burstData.lifeLimit - delay;
        }
        else if (data.type == EmitterType::LINEAR)
        {
            state.isDistanceBased = false;
            state.lifeLimit = data.linearData.lifeLimit;

            state.batchSize = 1;
            state.cyclesCount = -1;

            int count = std::max((int)(data.linearData.countMax * emission_factor), 1);
            state.tickLimit = data.linearData.lifeLimit / count;

            state.spawnTick = -delay;
            state.shrinkTick = -data.linearData.lifeLimit - delay;

            // we can't force this for all fx, because old system (flowps2.cpp) was waiting for the first tick
            if (data.linearData.instant)
            {
                state.spawnTick += state.tickLimit;
                state.shrinkTick += state.tickLimit;
            }
        }
        else
        {
            NAU_LOG_ERROR("Unsupported Emitter Type");
        }

        state.cyclesCountRef = state.cyclesCount;
        state.spawnTickRef = state.spawnTick;
        state.shrinkTickRef = state.shrinkTick;
        state.garbageTick = 0;
    }

    int emitter_utils::update_emitter(EmitterState& state, float dt)
    {
        if (state.globalLifeLimit > 0)
        {
            state.globalLifeLimit -= dt;
        }

        bool allowSpawn = dt > 0 && state.globalLifeLimit >= 0;

        int spawnStep = 0;
        int shrinkStep = 0;

        state.spawnTick += allowSpawn ? dt * state.totalTickRate : 0;
        state.shrinkTick += dt * state.totalTickRate;

        // Shrink particles
        if (state.shrinkTick >= state.tickLimit && state.lifeLimit > 0)
        {
            int count = state.tickLimit > 0 ? static_cast<int>(std::floor(state.shrinkTick / state.tickLimit)) : 1;
            state.shrinkTick -= count * state.tickLimit;
            shrinkStep = count * state.batchSize;
        }

        // Spawn particles
        if (allowSpawn && state.spawnTick >= state.tickLimit && state.cyclesCount != 0)
        {
            int count = state.tickLimit > 0 ? static_cast<int>(std::floor(state.spawnTick / state.tickLimit)) : 1;
            state.spawnTick -= count * state.tickLimit;

            if (state.cyclesCount > 0)
            {
                state.cyclesCount--;
            }

            spawnStep = count * state.batchSize;

            // callback(spawnStep);
        }

        // Handle emitter finish
        if (state.cyclesCount == 0)
        {
            state.garbageTick += dt;
            if (state.garbageTick > state.lifeLimit && state.lifeLimit > 0)
            {
                state.spawnTick = state.spawnTickRef;
                state.shrinkTick = state.shrinkTickRef;
                state.garbageTick = 0;
            }
        }

        return spawnStep;
    }
}  // namespace nau::vfx
