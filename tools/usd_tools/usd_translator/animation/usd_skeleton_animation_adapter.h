// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once

#include <nau/scene/scene.h>

#include <string>
#include <vector>

#include "nau/animation/components/skeleton_component.h"
#include "nau/assets/skeleton_asset_accessor.h"
#include "usd_proxy/usd_proxy.h"
#include "usd_translator/usd_prim_adapter.h"
#include "usd_translator/usd_translator_api.h"

namespace UsdTranslator
{
    class USD_TRANSLATOR_API UsdAnimationSkelAccessor final : public nau::ISkeletonAssetAccessor
    {
        NAU_CLASS_(UsdTranslator::UsdAnimationSkelAccessor, ISkeletonAssetAccessor);

    public:
        static std::vector<std::string> getOzzAnimationPaths(const pxr::UsdPrim& skelPrim, const std::string& assetPath);
        static std::string getOzzSkeletonPath(const std::string& assetPath);

        UsdAnimationSkelAccessor(const pxr::UsdPrim& prim, const std::string& assetPath);

        void reset(const std::string& assetPath);

        nau::SkeletonDataDescriptor getDescriptor() const override;
        void copyInverseBindMatrices(std::vector<nau::math::mat4>& data) const override;

    private:
        pxr::UsdPrim m_skelPrim;
        std::string m_skeletonAssetPath;
        nau::SkeletonDataDescriptor m_skeletonDesc;
        std::vector<nau::math::mat4> m_bindMatrixList;
    };


    class USD_TRANSLATOR_API SkeletonAnimationAdapter : public IPrimAdapter
    {
    public:
        SkeletonAnimationAdapter(pxr::UsdPrim prim);

        std::string_view getType() const override;
        nau::async::Task<nau::scene::ObjectWeakRef<nau::scene::SceneObject>> initializeSceneObject(nau::scene::ObjectWeakRef<nau::scene::SceneObject> dest) override;
        nau::scene::ObjectWeakRef<nau::scene::SceneObject> getSceneObject() const override;
        nau::async::Task<> update() override;
        bool isValid() const override;

    protected:
        void destroySceneObject() override;

    private:
        nau::scene::ObjectWeakRef<nau::scene::SceneObject> m_obj = nullptr;
        nau::scene::ObjectWeakRef<nau::SkeletonComponent> m_skeleton = nullptr;
        std::string m_typeName;
    };
}