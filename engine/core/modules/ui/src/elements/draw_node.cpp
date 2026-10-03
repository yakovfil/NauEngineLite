// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "nau/ui/elements/draw_node.h"

#include <vector>

#include "2d/CCDrawNode.h"
#include "ccTypes.h"
#include "math/Vec2.h"

namespace nau::ui {

DrawNode* DrawNode::create()
{
    DrawNode * ret = new (std::nothrow) DrawNode();
    if (ret && ret->init())
    {
        ret->autorelease();
    }
    else
    {
        CC_SAFE_DELETE(ret);
    }
    return ret;
}

void DrawNode::drawPoint(const math::vec2& point, const float pointSize, const math::Color4 &color)
{
    cocos2d::DrawNode::drawPoint(point, pointSize, color);
}

void DrawNode::drawPoints(const math::vec2 *position, unsigned int numberOfPoints, const math::Color4 &color)
{
    std::vector<cocos2d::Vec2> cocosPosition(numberOfPoints);
    for (int i = 0; i < numberOfPoints; ++i)
    {
        cocosPosition[i] = position[i];
    }

    cocos2d::DrawNode::drawPoints(cocosPosition.data(), numberOfPoints, cocos2d::Color4F{color});
}

void DrawNode::drawPoints(const math::vec2 *position, unsigned int numberOfPoints, const float pointSize, const math::Color4 &color)
{
    std::vector<cocos2d::Vec2> cocosPosition(numberOfPoints);
    for (int i = 0; i < numberOfPoints; ++i)
    {
        cocosPosition[i] = position[i];
    }

    cocos2d::DrawNode::drawPoints(cocosPosition.data(), numberOfPoints, pointSize, cocos2d::Color4F{color});
}

void DrawNode::drawLine(const math::vec2 &origin, const math::vec2 &destination, const math::Color4 &color)
{
    cocos2d::DrawNode::drawLine(origin, destination, color);
}

void DrawNode::drawRect(const math::vec2 &origin, const math::vec2 &destination, const math::Color4 &color)
{
    cocos2d::DrawNode::drawRect(origin, destination, color);
}

void DrawNode::drawPoly(const math::vec2 *poli, unsigned int numberOfPoints, bool closePolygon, const math::Color4 &color)
{
    std::vector<cocos2d::Vec2> cocosPoli(numberOfPoints);
    for(int i = 0; i < numberOfPoints; ++i)
    {
        cocosPoli[i] = poli[i];
    }

    cocos2d::DrawNode::drawPoly(cocosPoli.data(), numberOfPoints, closePolygon, color);
}

void DrawNode::drawCircle(const math::vec2& center, float radius, float angle, unsigned int segments, bool drawLineToCenter, float scaleX, float scaleY, const math::Color4 &color)
{
    cocos2d::DrawNode::drawCircle(center, radius, angle, segments, drawLineToCenter, scaleX, scaleY, color);
}

void DrawNode::drawCircle(const math::vec2 &center, float radius, float angle, unsigned int segments, bool drawLineToCenter, const math::Color4 &color)
{
    cocos2d::DrawNode::drawCircle(center, radius, angle, segments, drawLineToCenter, color);
}

void DrawNode::drawQuadBezier(const math::vec2 &origin, const math::vec2 &control, const math::vec2 &destination, unsigned int segments, const math::Color4 &color)
{
    cocos2d::DrawNode::drawQuadBezier(origin, control, destination, segments, color);
}

void DrawNode::drawCubicBezier(const math::vec2 &origin, const math::vec2 &control1, const math::vec2 &control2, const math::vec2 &destination, unsigned int segments, const math::Color4 &color)
{
    cocos2d::DrawNode::drawCubicBezier(origin, control1, control2, destination, segments, color);
}

void DrawNode::drawDot(const math::vec2 &pos, float radius, const math::Color4 &color)
{
    cocos2d::DrawNode::drawDot(pos, radius, color);
}

void DrawNode::drawRect(const math::vec2 &p1, const math::vec2 &p2, const math::vec2 &p3, const math::vec2& p4, const math::Color4 &color)
{
    cocos2d::DrawNode::drawRect(p1, p2, p3, p4, color);
}

void DrawNode::drawSolidRect(const math::vec2 &origin, const math::vec2 &destination, const math::Color4 &color)
{
    cocos2d::DrawNode::drawSolidRect(origin, destination, color);
}

void DrawNode::drawSolidPoly(const math::vec2 *poli, unsigned int numberOfPoints, const math::Color4 &color)
{
    std::vector<cocos2d::Vec2> cocosPoli(numberOfPoints);
    for(int i = 0; i < numberOfPoints; ++i)
    {
        cocosPoli[i] = poli[i];
    }
    cocos2d::DrawNode::drawSolidPoly(cocosPoli.data(), numberOfPoints, color);
}

void DrawNode::drawSolidCircle(const math::vec2& center, float radius, float angle, unsigned int segments, float scaleX, float scaleY, const math::Color4 &color)
{
    cocos2d::DrawNode::drawSolidCircle(center, radius, angle, segments, scaleX, scaleY, color);
}

void DrawNode::drawSolidCircle(const math::vec2& center, float radius, float angle, unsigned int segments, const math::Color4& color)
{
    cocos2d::DrawNode::drawSolidCircle(center, radius, angle, segments, color);
}

void DrawNode::drawSegment(const math::vec2 &from, const math::vec2 &to, float radius, const math::Color4 &color)
{
    cocos2d::DrawNode::drawSegment(from, to, radius, color);
}

void DrawNode::drawPolygon(const math::vec2 *verts, int count, const math::Color4 &fillColor, float borderWidth, const math::Color4 &borderColor)
{
    std::vector<cocos2d::Vec2> cocosVerts(count);
    for(int i = 0; i < count; ++i)
    {
        cocosVerts[i] = verts[i];
    }
    cocos2d::DrawNode::drawPolygon(cocosVerts.data(), count, fillColor, borderWidth, borderColor);
}

void DrawNode::drawTriangle(const math::vec2 &p1, const math::vec2 &p2, const math::vec2 &p3, const math::Color4 &color)
{
    cocos2d::DrawNode::drawTriangle(p1, p2, p3, color);
}

void DrawNode::clearDrawNode()
{
    cocos2d::DrawNode::clear();
}

} //namespace nau::ui
