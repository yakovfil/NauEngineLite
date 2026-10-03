// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#include "modfx_velocity.h"

#include <algorithm>

#include "math/vfx_random.h"

namespace nau::vfx::modfx
{
    void velocity::modfx_velocity_add(int rnd_seed, nau::math::Vector3 pos, nau::math::Vector3& o_velocity, const settings::FxVelocity& velocity)
    {
        nau::math::Vector3 res = nau::math::Vector3::zero();
        float len = nau::math::lerp(velocity.add.vel_min, velocity.add.vel_max, vfx::math::dafx_frnd(rnd_seed));

        if (velocity.add.type == settings::AddType::POINT)
        {
            nau::math::Vector3 offset = velocity.add.point.offset;

            res = normalize(pos - offset);
        }
        else if (velocity.add.type == settings::AddType::VEC)
        {
            res = velocity.add.vec.vec;
        }
        else if (velocity.add.type == settings::AddType::CONE)
        {
        }

        o_velocity = nau::math::lerp(res, vfx::math::dafx_srnd_vec3(rnd_seed), velocity.add.vec_rnd) * len;
    }

    void velocity::modfx_velocity_force_field_vortex(float life_k, int rnd_seed, nau::math::Vector3 pos, nau::math::Vector3& o_velocity, const settings::FxVelocity& velocity)
    {
        float rotation_speed = nau::math::lerp(velocity.force_field.vortex.rotation_speed_min, velocity.force_field.vortex.rotation_speed_max, vfx::math::dafx_frnd(rnd_seed));
        float pull_speed = nau::math::lerp(velocity.force_field.vortex.pull_speed_min, velocity.force_field.vortex.pull_speed_max, vfx::math::dafx_frnd(rnd_seed));

        nau::math::Vector3 axis_position = velocity.force_field.vortex.axis_position + mulPerElem(velocity.force_field.vortex.position_rnd, vfx::math::dafx_srnd_vec3(rnd_seed));
        
        nau::math::Vector3 axis_direction = nau::math::Vector3::zero();
        if (!velocity.force_field.vortex.axis_direction.similar(nau::math::Vector3::zero()))
        {
            axis_direction = normalize(nau::math::lerp(velocity.force_field.vortex.axis_direction, vfx::math::dafx_srnd_vec3(rnd_seed), velocity.force_field.vortex.direction_rnd));
        }

        nau::math::Vector3 to_particle = pos - axis_position;
        nau::math::Vector3 normal = to_particle - dot(axis_direction, to_particle) * axis_direction;
        float radius = length(normal);
        normal /= std::max(radius, 0.00001f);
        nau::math::Vector3 tangent = cross(axis_direction, normal);

        o_velocity = tangent * rotation_speed * radius - normal * pull_speed;
    }

    void velocity::modfx_velocity_force_resolver(float dt, float mass, float drag_c, float friction_k, nau::math::Vector3 grav_vec, nau::math::Vector3& o_pos, nau::math::Vector3& o_vel)
    {
        float c_p = 1.225f;

        float c_f = (0.5f * c_p * drag_c);

        float vel_len = length(o_vel);
        nau::math::Vector3 vel_norm = nau::math::Vector3::zero();
        if (vel_len > 0)
        {
            vel_norm = o_vel * (1.0f / vel_len);
        }

        float iter_dt = dt;
        float iter_dt_p2_half = iter_dt * iter_dt * 0.5f;

        float drag_force = vel_len * vel_len * c_f;
        float drag_limit = 0.5f;
        drag_force = std::min(drag_force, (vel_len * drag_limit) * (1.0f / iter_dt));
        nau::math::Vector3 drag_v = drag_force * vel_norm;

        nau::math::Vector3 acc = (-drag_v + grav_vec) * friction_k;
        o_pos = o_pos + o_vel * iter_dt + acc * iter_dt_p2_half;
        o_vel = o_vel + acc * iter_dt;
    }

    void velocity::modfx_velocity_init(nau::math::Vector3& pos, nau::math::Vector3& pos_v, nau::math::Vector3& o_velocity, int rnd_seed, const settings::FxVelocity& velocity)
    {
        float len = nau::math::lerp(velocity.start.vel_min, velocity.start.vel_min, vfx::math::dafx_frnd(rnd_seed));

        if (velocity.start.type == settings::StartType::POINT)
        {
            o_velocity = normalize(pos - velocity.start.point.offset);
        }
        else if (velocity.start.type == settings::StartType::VEC)
        {
            o_velocity = velocity.start.vec.vec;
        }
        else if (velocity.start.type == settings::StartType::START_SHAPE)
        {
            o_velocity = pos_v;
        }

        o_velocity = nau::math::lerp(o_velocity, vfx::math::dafx_srnd_vec3(rnd_seed), velocity.start.vec_rnd) * len;
    }

    void velocity::modfx_velocity_sim(int rnd_seed, float life_k, float dt, float radius, nau::math::Vector3& o_pos, nau::math::Vector3& o_ofs_pos, nau::math::Vector3& o_velocity, const settings::FxVelocity& velocity)
    {
        if (dt <= 0)
            return;

        float mass = 0.0f;
        float drag = 0.0f;

        nau::math::Vector3 grav_vec = nau::math::Vector3::zero();
        if (velocity.apply_gravity)
        {
            const float g = -9.81f;
            grav_vec = nau::math::Vector3(0, g, 0);
        }

        if (velocity.mass > 0)
        {
            mass = velocity.mass;
        }

        if (velocity.drag_coeff > 0)
        {
            float r = nau::math::lerp(1.0f, radius, velocity.drag_to_rad_k);
            float c_a = PI * (r * r);  // sphere projection
            drag = c_a * velocity.drag_coeff;
        }

        // without curve

        if (velocity.add.enabled && (velocity.add.vel_min > 0 || velocity.add.vel_max > 0))
        {
            nau::math::Vector3 add_v = nau::math::Vector3::zero();
            modfx_velocity_add(rnd_seed, o_pos, add_v, velocity);
            o_velocity += add_v * dt;
        }

        if (velocity.force_field.vortex.enabled)
        {
            nau::math::Vector3 add_v = nau::math::Vector3::zero();
            modfx_velocity_force_field_vortex(life_k, rnd_seed, o_pos, add_v, velocity);
            o_velocity += add_v * dt;
        }

        float friction_k = 1.0f;
        if (velocity.mass > 0.0f)
        {
            modfx_velocity_force_resolver(dt, mass, drag, friction_k, grav_vec, o_pos, o_velocity);
        }
        else
        {
            o_velocity += grav_vec * dt;
            o_pos += o_velocity * dt;
        }
    }
}
