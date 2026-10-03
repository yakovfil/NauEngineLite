// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once

#include <map>

#include "nau/math/dag_color.h"
#include "nau/math/math.h"

namespace nau::vfx::math
{
    nau::math::Color4 interpolateColor(const nau::math::Color4& color1, const nau::math::Color4& color2, float t);

    class Gradient
    {
    public:
        Gradient() = default;
        ~Gradient() = default;

    public:
        void addStop(float position, const nau::math::Color4& color);
        nau::math::Color4 getColorAt(float position) const;

    private:
        static constexpr int MaxPoints = 64;
        std::map<float, nau::math::Color4> m_gradientStops;
    };
}  // namespace nau::vfx::math