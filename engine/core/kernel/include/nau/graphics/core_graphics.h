// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once
#include <nau/math/dag_color.h>

#include <vector>

#include "nau/async/task_base.h"
#include "nau/math/math.h"
#include "nau/rtti/rtti_object.h"
#include "nau/utils/uid.h"

namespace nau
{
    namespace render
    {
        struct IRenderWindow;
    }
    /**
     */
    struct NAU_ABSTRACT_TYPE ICoreGraphics : virtual IRttiObject
    {
        NAU_INTERFACE(nau::ICoreGraphics, IRttiObject)

        virtual async::Task<bool> renderFrame() = 0;
        virtual math::mat4 getProjMatrix() = 0;
        virtual async::Task<> requestViewportResize(int32_t newWidth, int32_t newHeight, void* hwnd) = 0;

        virtual async::Task<> registerWindow(void* hwnd) = 0;  // TODO: deprecated
        virtual async::Task<> closeWindow(void* hwnd) = 0;     // TODO: deprecated

        virtual async::Task<WeakPtr<nau::render::IRenderWindow>> createRenderWindow(void* hwnd) = 0;
        virtual WeakPtr<nau::render::IRenderWindow> getDefaultRenderWindow() = 0;
        virtual void getRenderWindows(std::vector<WeakPtr<nau::render::IRenderWindow>>& windows) = 0;

        virtual void setObjectHighlight(nau::Uid uid, bool flag) = 0;

        virtual async::Executor::Ptr getPreRenderExecutor() = 0;
    };

}  // namespace nau
