// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once

#include <memory>
#include <string>
#include <vector>

#include "nau/assets/asset_accessor.h"
#include "nau/assets/asset_ref.h"
#include "nau/async/task.h"
#include "nau/math/dag_color.h"
#include "nau/math/dag_e3dColor.h"
#include "nau/math/math.h"
#include "nau/math/transform.h"

namespace nau
{
    struct NAU_COREASSETS_EXPORT UiElementAssetCustomData
    {
        virtual ~UiElementAssetCustomData();
    };

    struct NauLabelAssetData final : UiElementAssetCustomData
    {
        std::string text;
        std::string fontRef;
        int horizontalAlignment = 0;
        int verticalAlignment = 0;
        int overflow = 0;
        int wrapping = 0;
    };

    struct NauButtonStateAssetData
    {
        std::string imageFileName;
        math::Color4 color;
        float scale = 1.f;
        AnimationAssetRef animationAsset;
    };

    struct NauButtonAssetData final : UiElementAssetCustomData
    {
        NauButtonStateAssetData normalStateData;
        NauButtonStateAssetData hoveredStateData;
        NauButtonStateAssetData pressedStateData;
        NauButtonStateAssetData disabledStateData;
    };


    struct SpriteAssetData final : UiElementAssetCustomData
    {
        std::string fileName;
    };

    struct DrawNodeDrawPolygon
    {
        math::vec2 points[4];
        math::Color4 fillColor;
        math::Color4 borderColor;
        float borderWidth;
    };

    struct ScrollAssetData final : UiElementAssetCustomData
    {
        std::string scrollType;
    };

    struct DrawNodeAssetData final : UiElementAssetCustomData
    {
        DrawNodeDrawPolygon drawPolygon;
    };

    enum class UiElementType
    {
        Invalid,
        Node,
        Label,
        Button,
        Sprite,
        Scroll,
        DrawNode,
        Layer,
    };

    struct UiElementAssetData final
    {
        UiElementType elementType;

        std::string name;
        math::vec2 translation;
        float rotation;
        math::vec2 scale;
        int zOrder;
        bool visible;
        math::vec2 anchorPoint;
        math::vec2 contentSize;
        math::vec2 scew;
        math::vec2 rotationSkew;
        math::E3DCOLOR color;
        bool cascadeColorEnabled;
        bool cascadeOpacityEnabled;
        bool enableDebugDraw;

        std::shared_ptr<UiElementAssetCustomData> customData;

        std::vector<UiElementAssetData> children;
    };

    struct NAU_ABSTRACT_TYPE IUiAssetAccessor : IAssetAccessor
    {
        NAU_INTERFACE(nau::IUiAssetAccessor, IAssetAccessor)

        virtual async::Task<> copyUiElements(std::vector<UiElementAssetData>& elements) = 0;
    };

}  // namespace nau
