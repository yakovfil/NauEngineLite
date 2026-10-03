// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "nau/ui/elements/canvas.h"

#include <algorithm>
#include <string>

#include "CCDirector.h"
#include "math/Vec2.h"
#include "nau/diag/assertion.h"
#include "nau/math/math.h"

namespace nau::ui
{
    const std::string Canvas::DEFAULT_NAME = "[unnamed]";

    Canvas::Canvas(const std::string& name) :
        m_canvasName(name)
    {
    }

Canvas::~Canvas() {}

Canvas* Canvas::create(math::vec2 size, RescalePolicy rescale)
{
    Canvas * ret = new (std::nothrow) Canvas(DEFAULT_NAME);
    if (ret && ret->init())
    {
        ret->autorelease();
        ret->setReferenceSize(size);
        ret->setRescalePolicy(rescale);
    }
    else
    {
        CC_SAFE_DELETE(ret);
    }
    return ret;
}

Canvas* Canvas::create(const std::string& name, math::vec2 size, RescalePolicy rescale)
{
    Canvas * ret = new (std::nothrow) Canvas(name);
    if (ret && ret->init())
    {
        ret->autorelease();
        ret->setReferenceSize(size);
        ret->setRescalePolicy(rescale);
    }
    else
    {
        CC_SAFE_DELETE(ret);
    }
    return ret;
}

void Canvas::setReferenceSize(math::vec2 size)
{
    markDirty();
    m_size = size;
}


math::vec2 Canvas::getReferenceSize() const
{
    return m_size;
}

RescalePolicy Canvas::getRescalePolicy() const
{
    return m_rescale;
}

void Canvas::setRescalePolicy(RescalePolicy rescale)
{
    markDirty();
    m_rescale = rescale;
    const math::vec2 windowSize = cocos2d::Director::getInstance()->getWinSize();

    switch (rescale)
    {
        case nau::ui::RescalePolicy::FitToSize:
            {
                float scaleFactor = 1.f;
                if (m_size.getX() > windowSize.getX())
                {
                    scaleFactor = windowSize.getX() / m_size.getX();
                }

                if (m_size.getX() > windowSize.getX())
                {
                    scaleFactor = std::min(scaleFactor, windowSize.getY() / m_size.getY());
                }

                setScale(scaleFactor);
            }
            break;
        case nau::ui::RescalePolicy::FitVertically:
            setScale(windowSize.getY() / m_size.getY());
            break;
        case nau::ui::RescalePolicy::FitHorizontally:
            setScale(windowSize.getX() / m_size.getX());
            break;
        case nau::ui::RescalePolicy::Stretch:
            setScaleX(windowSize.getX() / m_size.getX());
            setScaleY(windowSize.getY() / m_size.getY());
            break;
        case nau::ui::RescalePolicy::NoRescale:
            setScale(1.f);
            break;
        default:
            NAU_FAILURE("Unsupported scene rescale policy");
    }
}


}

