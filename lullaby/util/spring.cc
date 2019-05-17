/*
Copyright 2017-2019 Google Inc. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS-IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include "lullaby/util/spring.h"

namespace lull {
namespace {

// Angle modulus in degrees, adjusted such that the output is in the range
// (-180, 180].
float ModDegrees(float degrees) {
  const float m = fmodf(degrees + 180.0f, 360.0f);
  return m < 0.0f ? m + 180.0f : m - 180.0f;
}

// Angle modulus in radians, adjusted such that the output is in the range
// (-pi, pi].
float ModRadians(float radians) {
  constexpr float pi = static_cast<float>(M_PI);
  const float m = std::fmod(radians + pi, 2.0f * pi);
  return m < 0.0f ? m + pi : m - pi;
}

}  // namespace

void CriticallyDampedSpringUpdate(float pos0, float vel0, float spring_factor,
                                  float dt, float* out_pos1, float* out_vel1) {
  const float b = vel0 + spring_factor * pos0;
  const float e = expf(-spring_factor * dt);
  const float pos1 = (pos0 + b * dt) * e;
  const float vel1 = b * e - spring_factor * pos1;
  *out_pos1 = pos1;
  *out_vel1 = vel1;
}

void OverDampedSpringUpdate(float pos0, float vel0, float spring_factor,
                            float spring_damp, float dt, float* out_pos1,
                            float* out_vel1) {
  DCHECK_GT(spring_damp, 0.0f);

  const float c =
      sqrtf(4.0f * spring_factor * spring_factor + spring_damp * spring_damp);

  const float r0 = -0.5f * (c - spring_damp);
  const float r1 = -0.5f * (c + spring_damp);
  const float recip_dr = 1.0f / (r1 - r0);

  const float e1 = expf(r0 * dt);
  const float e2 = expf(r1 * dt);

  const float m00 = (e2 - e1) * recip_dr;
  const float m01 = e1 - r0 * m00;
  const float m10 = (r1 * e2 - r0 * e1) * recip_dr;
  const float m11 = r0 * (e1 - m10);

  *out_pos1 = m00 * vel0 + m01 * pos0;
  *out_vel1 = m10 * vel0 + m11 * pos0;
}

void CriticallyDampedSpringLerpDegrees(float goal_pos, float pos0, float vel0,
                                       float spring_factor, float dt,
                                       float* out_pos1, float* out_vel1) {
  const float delta = ModDegrees(pos0 - goal_pos);
  float pos1;
  CriticallyDampedSpringUpdate(delta, vel0, spring_factor, dt, &pos1, out_vel1);
  *out_pos1 = pos1 + goal_pos;
}

void CriticallyDampedSpringLerpRadians(float goal_pos, float pos0, float vel0,
                                       float spring_factor, float dt,
                                       float* out_pos1, float* out_vel1) {
  const float delta = ModRadians(pos0 - goal_pos);
  float pos1;
  CriticallyDampedSpringUpdate(delta, vel0, spring_factor, dt, &pos1, out_vel1);
  *out_pos1 = pos1 + goal_pos;
}

void OverDampedSpringLerpDegrees(float goal_pos, float pos0, float vel0,
                                 float spring_factor, float spring_damp,
                                 float dt, float* out_pos1, float* out_vel1) {
  const float delta = ModDegrees(pos0 - goal_pos);
  float pos1;
  OverDampedSpringUpdate(delta, vel0, spring_factor, spring_damp, dt, &pos1,
                         out_vel1);
  *out_pos1 = pos1 + goal_pos;
}

void OverDampedSpringLerpRadians(float goal_pos, float pos0, float vel0,
                                 float spring_factor, float spring_damp,
                                 float dt, float* out_pos1, float* out_vel1) {
  const float delta = ModRadians(pos0 - goal_pos);
  float pos1;
  OverDampedSpringUpdate(delta, vel0, spring_factor, spring_damp, dt, &pos1,
                         out_vel1);
  *out_pos1 = pos1 + goal_pos;
}

}  // namespace lull
