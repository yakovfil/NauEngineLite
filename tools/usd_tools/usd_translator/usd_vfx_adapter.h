// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once

#include <nau/scene/scene.h>
#include <pxr/usd/usd/stage.h>

#include <string>

#include "nau/assets/reloadable_asset_view.h"
#include "nau/platform/windows/utils/uid.h"
#include "nau/utils/result.h"
#include "usd_prim_adapter.h"
#include "usd_prim_translator.h"
#include "usd_proxy/usd_proxy.h"

namespace UsdTranslator
{
    class USD_TRANSLATOR_API VFXAdapter : public IPrimAdapter
    {
    public:
        VFXAdapter(PXR_NS::UsdPrim prim);
        ~VFXAdapter();

        bool isValid() const override;
        nau::async::Task<> update() override;
        std::string_view getType() const override;

        nau::async::Task<nau::scene::ObjectWeakRef<nau::scene::SceneObject>> initializeSceneObject(nau::scene::ObjectWeakRef<nau::scene::SceneObject> dest) override;
        nau::scene::ObjectWeakRef<nau::scene::SceneObject> getSceneObject() const override;

    protected:
        void destroySceneObject() override;

    private:
        uint64_t m_vfxTimeStamp;
        nau::Uid m_textureUID;
        nau::ReloadableAssetView::Ptr m_texture;
        std::string m_path;

        nau::scene::ObjectWeakRef<nau::scene::SceneObject> m_obj;
    };
}  // namespace UsdTranslator
