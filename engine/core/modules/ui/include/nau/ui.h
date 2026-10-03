// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once

#include <functional>
#include <string>

#include "nau/3d/dag_tex3d.h"
#include "nau/rtti/rtti_object.h"
#include "nau/scene/scene.h"

#ifndef NODE_DEBUG_SEARCH
    #define NODE_DEBUG_SEARCH 0
#endif

namespace nau::ui
{
    class Canvas;
    class Node;
    struct Uid;

    /**
     * @brief Provides basic control over canvases and GUI elements within GUI hierarchy.
     */
    struct NAU_ABSTRACT_TYPE UiManager
    {
        NAU_TYPEID(nau::ui::UiManager)

        using TElementChangedCallback = std::function<void(class Node*)>;
        using TCanvasLoadedCallback = std::function<void(const std::string& canvasName)>;
        using TCallbackHandle = uint32_t;
        const static TCallbackHandle INVALID_CB_HANDLE = 0;

        virtual void applicationDidEnterBackground() = 0;
        virtual void applicationWillEnterForeground() = 0;
        virtual void update() = 0;
        virtual void update(float dt) = 0;
        virtual void render(BaseTexture* backBuffer) = 0;
        virtual void setAnimationInterval(float interval) = 0;
        virtual bool shouldShutDown() const = 0;
        virtual void setScreenSize(float width, float height) = 0;
        virtual void setReferenceResolution(float width, float height) = 0;

        // Work with canvases

        /**
         * @brief Adds canvas to the GUI hierarchy.
         * 
         * @param [in] canvas A pointer to the canvas to add.
         */
        virtual void addCanvas(Canvas* canvas) = 0;

        /**
         * @brief Retrieves the canvas from the GUI hierarchy.
         * 
         * @param [in] canvasName   Name of the canvas object to retrieve.
         * @return                  A pointer to the requested canvas object or `NULL` if the canvas has not been found.
         */
        virtual Canvas* getCanvas(const std::string& canvasName) = 0;

        /**
         * @brief Removes the canvas from the GUI hierarchy.
         * 
         * @param [in] canvasName Name of the canvas to delete.
         */
        virtual void removeCanvas(const std::string& canvasName) = 0;
        virtual void updateCanvases() = 0;
        virtual TCallbackHandle setOnCanvasLoadedCallback(TCanvasLoadedCallback&& callback) = 0;
        virtual bool setOnCanvasLoadedCallback(TCallbackHandle handle) = 0;

        virtual void configureResourcePath() = 0;
        virtual void setEngineScene(scene::IScene::WeakRef scene) = 0;
        virtual scene::IScene* getEngineScene() = 0;

        /**
         * @brief Retrieves a GUI element attached to the canvas that is currently located under the cursor.
         * 
         * @param [in] canvasName   Name of the canvas that the object is attached to.
         * @return                  A pointer to the object that the cursor is currenrly pointing at or `NULL` if the cursor is not currently pointing at anything.
         */
        virtual Node* getCanvasChildUnderCursor(const std::string& canvasName) = 0;

        /**
         * @brief Sets a function that is called when a GUI element is modified in update.
         * 
         * @param [in] elementUid   Identifier of the changed element.
         * @param [in] callback     Callback to assign.
         * 
         * @warning This is a debug-only function. Do not use it in release builds.
         */
        virtual void setElementChangedCallback(nau::Uid elementUid, TElementChangedCallback&& callback) = 0;

        /**
         * @brief Unbinds the onElementChangedCallback callback.
         * 
         * @param [in] id Identifier of the element to unbind the callback from.
         * 
         * See setElementChangedCallback.
         */
        virtual void removeElementChangedCallback(nau::Uid id) = 0;
    };
}  // namespace nau::ui
