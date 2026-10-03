// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once

#include <unordered_set>
#include <vector>

#include "graphics_assets/material_asset.h"
#include "graphics_assets/shader_asset.h"
#include "modfx/emitter/emitter_data.h"
#include "modfx/emitter/emitter_state.h"
#include "modfx/settings/fx_color.h"
#include "modfx/settings/fx_life.h"
#include "modfx/settings/fx_position.h"
#include "modfx/settings/fx_radius.h"
#include "modfx/settings/fx_rotation.h"
#include "modfx/settings/fx_spawn.h"
#include "modfx/settings/fx_texture.h"
#include "modfx/settings/fx_velocity.h"
#include "nau/assets/asset_ref.h"
#include "nau/math/math.h"
#include "vfx_instance.h"

namespace nau::vfx::modfx
{
    class NAU_VFX_EXPORT VFXModFXInstance : public IVFXInstance
    {
        NAU_CLASS(nau::vfx::modfx::VFXModFXInstance, rtti::RCPolicy::Concurrent, IVFXInstance)

    public:
        VFXModFXInstance(const nau::MaterialAssetView::Ptr material);
        ~VFXModFXInstance() = default;

        void serialize(nau::DataBlock* blk) const override;
        bool deserialize(const nau::DataBlock* blk) override;

    public:
        void setSpawnSettings(const settings::FxSpawn& spawn);
        settings::FxSpawn spawnSettings() const;

        void setLifeSettings(const settings::FxLife& life);
        settings::FxLife lifeSettings() const;

        void setPositionSettings(const settings::FxPosition& position);
        settings::FxPosition positionSettings() const;

        void setRadiusSettings(const settings::FxRadius& radius);
        settings::FxRadius radiusSettings() const;

        void setColorSettings(const settings::FxColor& color);
        settings::FxColor colorSettings() const;

        void setRotationSettings(const settings::FxRotation& rotation);
        settings::FxRotation rotationSettings() const;

        void setVelocitySettings(const settings::FxVelocity& velocity);
        settings::FxVelocity velocitySettings() const;

        void setTextureSettings(const settings::FxTexture& texture);
        settings::FxTexture textureSettings() const;

    public:
        void setTexture(ReloadableAssetView::Ptr assetTexture);

    public:
        void play();
        void stop();

    public:
        void setTransform(const nau::math::Matrix4& transform) override;
        nau::math::Matrix4 transform() const override;

        void update(float dt) override;
        void render(const nau::math::Matrix4& view, const nau::math::Matrix4& projection) override;

    private:
        struct ModFxData
        {
            modfx::ModfxRenData rdata;
            modfx::ModfxSimData sdata;
        };

    private:
        void addParticles(int particleToSpawn);
        void initializeParticleData(ModFxData& data);

        void simulateParticles(float dt);

        void updateSpawnSettings();

    private:
        static inline constexpr int PoolSizeMultiplier = 2;
        static inline constexpr int MaxParticleCount = 20;

    private:
        settings::FxSpawn m_spawn;
        settings::FxLife m_life;
        settings::FxPosition m_position;
        settings::FxRadius m_radius;
        settings::FxColor m_color;
        settings::FxRotation m_rotation;
        settings::FxVelocity m_velocity;
        settings::FxTexture m_texture;

    private:
        // TODO Move out to base class
        Sbuffer* m_positionBuffer;
        Sbuffer* m_normalBuffer;
        Sbuffer* m_texCoordBuffer;

        Sbuffer* m_quadIndexBuffer;
        Sbuffer* m_instanceBuffer;

        MaterialAssetView::Ptr m_material;
        ReloadableAssetView::Ptr m_assetTexture;

        struct InstanceData
        {
            nau::math::Matrix4 worldMatrix;
            int frameID;
            nau::math::Color4 color;
            nau::math::uint3 dummy;
        };

        uint16_t quadIndices[6];

        void prepareQuadBuffer();
        void prepareInstanceBuffer();

    private:
        EmitterData m_emitterData;
        EmitterState m_emitterState;

        std::vector<ModFxData> m_particlePool;
        std::unordered_set<int> m_freeIndexPool;
        std::vector<InstanceData> m_instanceData;

    private:
        int m_actualParticlePoolSize;

        nau::math::Matrix4 m_transform;
        nau::math::Vector3 m_offset;

        bool m_isPause;

        std::mutex m_vfxMutex;
    };
}  // namespace nau::vfx::modfx