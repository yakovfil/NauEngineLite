// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once

#include "usd_stage_watcher.h"

namespace UsdProxy
{

    PXR_NAMESPACE_USING_DIRECTIVE

    StageObjectChangedWatcher::StageObjectChangedWatcher(const UsdStageRefPtr& stage, StageObjectChangedWatcherCallback cbFunc)
    {
        m_stage = stage;
        m_cb = cbFunc;
        m_objectChangeKey = TfNotice::Register(TfWeakPtr<StageObjectChangedWatcher>(this), &StageObjectChangedWatcher::onObjectsChanged, m_stage);
    }

    StageObjectChangedWatcher::~StageObjectChangedWatcher()
    {
        TfNotice::Revoke(m_objectChangeKey);
    }


    void StageObjectChangedWatcher::blockNotifications(bool enable)
    {
        m_blockNotifications = enable;
    }

    void StageObjectChangedWatcher::onObjectsChanged(UsdNotice::ObjectsChanged const& notice, UsdStageWeakPtr const& sender)
    {
        if (!sender || get_pointer(sender) != get_pointer(m_stage) || m_blockNotifications)
            return;

        m_cb(notice);
    }


    StageEditTargetChangedWatcher::StageEditTargetChangedWatcher(const UsdStageRefPtr& stage, StageEditTargetChangedWatcherCallback cbFunc)
    {
        m_stage = stage;
        m_cb = cbFunc;
        m_editTargetChangeKey = TfNotice::Register(TfWeakPtr<StageEditTargetChangedWatcher>(this), &StageEditTargetChangedWatcher::onStageEditTargetChanged, m_stage);

    }

    void StageEditTargetChangedWatcher::onStageEditTargetChanged(UsdNotice::StageEditTargetChanged const& notice, UsdStageWeakPtr const& sender)
    {
        if (!sender || get_pointer(sender) != get_pointer(m_stage))
            return;

        m_cb(notice);
    }

    StageEditTargetChangedWatcher::~StageEditTargetChangedWatcher()
    {

    }

    SdfLayerDirtinessChangedWatcher::SdfLayerDirtinessChangedWatcher(SdfLayerHandle& layer, SdfLayerDirtinessChangedWatcherCallback cbFunc)
    {
        m_layer = layer;
        m_cb = cbFunc;

        m_layerDirtyChangeKey = TfNotice::Register(TfWeakPtr<SdfLayerDirtinessChangedWatcher>(this), &SdfLayerDirtinessChangedWatcher::onChangeNotice, m_layer);
    }

    SdfLayerDirtinessChangedWatcher::~SdfLayerDirtinessChangedWatcher()
    {
    }

    void SdfLayerDirtinessChangedWatcher::onChangeNotice(const SdfNotice::LayerDirtinessChanged& notice, const SdfLayerHandle& sender)
    {
        if (!sender || get_pointer(sender) != get_pointer(m_layer))
            return;

        m_cb(notice);
    }


}

