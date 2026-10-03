// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once

#include <string>
#include <string_view>

#include "2d/CCNode.h"
#include "nau/math/math.h"
#include "nau/ui/elements/node.h"

namespace nau::ui {

/**
 * @brief Governs canvas behavior when the window size changes.
 */
enum class RescalePolicy
{

    FitToSize, ///< The canvas will be stretched proportionally to fit the window. However, there will be empty stripes along the larger axis in case proportions mismatch.

    FitVertically, ///< The canvas will be stetched proportinally to vertically fit the window. However, either there will be horizontal empty stripes or horizontal edges will be cropped in case proportions mismatch.

    FitHorizontally, ///< The canvas will be stetched proportinally to horizontally fit the window. However, either there will be vertical empty stripes or vertical edges will be cropped in case proportions mismatch.

    Stretch, ///< The canvas will be streched to fill the entire window possibly violating proportions.

    NoRescale, ///< The canvas will keep its size, but either an empty area surrounding it will appear or it will be cropped.
};

class NAU_UI_EXPORT Canvas : public nau::ui::Node
{
public:
    static const std::string DEFAULT_NAME;

    Canvas(const std::string&);
    ~Canvas();

    /**
     * @brief Creates canvas object.
     * 
     * @param [in] size     Canvas size in virtual pixels.
     * @param [in] rescale  Canvas rescaling policy.
     * @return              A pointer to the created canvas object.
     * 
     * @note The canvas will have a default name. If a named canvas creation is desired, use different overload.
     */
    static Canvas* create(
        math::vec2 size = {0.f, 0.f}, 
        RescalePolicy rescale = RescalePolicy::NoRescale);

    /**
     * @brief Creates canvas object.
     *
     * @param [in] name     Canvas name.
     * @param [in] size     Canvas size in virtual pixels.
     * @param [in] rescale  Canvas rescaling policy.
     * @return              A pointer to the created canvas object.
     */
    static Canvas* create(
        const std::string& name,
        math::vec2 size = {0.f, 0.f},
        RescalePolicy rescale = RescalePolicy::NoRescale);

    /**
     * @brief Changes the canvas reference size.
     * 
     * @param [in] size Value to assign.
     * 
     * @note Reference size of the canvas is it unscaled size. Depending on the actual window size and rescaling policy it will be changed in the UI scene.
     */
    void setReferenceSize(math::vec2 size);
    
    /**
     * @brief Retrieves the canvas reference size.
     * 
     * @return Canvas reference size in virtual pixels.
     * 
     * @note Reference size of the canvas is it unscaled size. Depending on the actual window size and rescaling policy it will be changed in the UI scene.
     */
    math::vec2 getReferenceSize() const;

    /**
     * @brief Retrieves canvas rescaling policy.
     * 
     * @return Current rescaling policy of the canvas.
     */
    RescalePolicy getRescalePolicy() const;

    /**
     * @brief Changes the canvas rescaling policy.
     * 
     * @param [in] rescale Policy to select.
     */
    void setRescalePolicy(RescalePolicy rescale);

    /**
     * @brief Retrieves the canvas name.
     * 
     * @return Canvas name.
     */
    const std::string& getCanvasName() const
    {
        return m_canvasName;
    }

    /**
     * @brief Retrieves a GUI element attached to the canvas.
     * 
     * @tparam TUIElement Type of the element to retrieve.
     * 
     * @param [in] name Name of the element to retrieve.
     * @return          A pointer to the canvas element or `NULL` if the element has not been found or in case types mismatch.
     */
    template <typename TUIElement>
    TUIElement* getUIElement(const std::string& name)
    {
        nau::ui::Node* searchResult = getNestedNodeByName(name);
        if (searchResult) 
        {
            TUIElement* element = dynamic_cast<TUIElement*>(searchResult);
            if (element) 
            {
                return element;
            } 
            else 
            {
                NAU_LOG_ERROR("Element found, but type mismatch for:{}", name);
            }
        } else 
        {
            NAU_LOG_ERROR("Element not found for:{}", name);
        }

        return nullptr;
    }
 
private:
    RescalePolicy m_rescale = { RescalePolicy::NoRescale };

    // Size in virtual pixels, actual size will depend on size of
    // the window and rescale policy
    math::vec2 m_size = {0.f, 0.f};

    std::string m_canvasName{};
};

}
