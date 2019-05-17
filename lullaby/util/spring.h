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

#ifndef LULLABY_UTIL_SPRING_H_
#define LULLABY_UTIL_SPRING_H_

#include "lullaby/util/math.h"

namespace lull {

// Simulate a 1D critically-damped spring, which is a spring with the minimal
// damping force required to prevent oscillation.
// * CriticallyDamped: Spring with the minimal damping force required to prevent
//   oscillation.
// * OverDamped: Spring with a high damping force relative to restitution force.
//   Does not oscillate, but comes to rest slower than a critically-damped
//   spring.
// A higher 'spring_factor' will approach the target position more quickly.
void CriticallyDampedSpringUpdate(float pos0, float vel0, float spring_factor,
                                  float dt, float* out_pos1, float* out_vel1);
void OverDampedSpringUpdate(float pos0, float vel0, float spring_factor,
                            float spring_damp, float dt, float* out_pos1,
                            float* out_vel1);

// Like *SpringUpdate, but with a goal position (rather than 0).
inline void CriticallyDampedSpringLerp(float goal_pos, float pos0, float vel0,
                                       float spring_factor, float dt,
                                       float* out_pos1, float* out_vel1) {
  float pos1;
  CriticallyDampedSpringUpdate(pos0 - goal_pos, vel0, spring_factor, dt, &pos1,
                               out_vel1);
  *out_pos1 = pos1 + goal_pos;
}
inline void OverDampedSpringLerp(float goal_pos, float pos0, float vel0,
                                 float spring_factor, float spring_damp,
                                 float dt, float* out_pos1, float* out_vel1) {
  float pos1;
  OverDampedSpringUpdate(pos0 - goal_pos, vel0, spring_factor, spring_damp, dt,
                         &pos1, out_vel1);
  *out_pos1 = pos1 + goal_pos;
}

// Like *SpringLerp, but the spring pulls in the direction of the minimum
// circular arc (in degrees or radians).
void CriticallyDampedSpringLerpDegrees(float goal_pos, float pos0, float vel0,
                                       float spring_factor, float dt,
                                       float* out_pos1, float* out_vel1);
void CriticallyDampedSpringLerpRadians(float goal_pos, float pos0, float vel0,
                                       float spring_factor, float dt,
                                       float* out_pos1, float* out_vel1);
void OverDampedSpringLerpDegrees(float goal_pos, float pos0, float vel0,
                                 float spring_factor, float spring_damp,
                                 float dt, float* out_pos1, float* out_vel1);
void OverDampedSpringLerpRadians(float goal_pos, float pos0, float vel0,
                                 float spring_factor, float spring_damp,
                                 float dt, float* out_pos1, float* out_vel1);

// N-dimensional spring.
template <typename T>
struct SpringT {
  T pos;
  T vel;

  void Assign(const T& pos, const T& vel) {
    this->pos = pos;
    this->vel = vel;
  }
};
typedef SpringT<float> Spring;
typedef SpringT<mathfu::vec2> Spring2;
typedef SpringT<mathfu::vec3> Spring3;

template <typename T>
struct VectorInfo;
template <>
struct VectorInfo<mathfu::vec2> {
  static constexpr size_t kComponentCount = 2;
};
template <>
struct VectorInfo<mathfu::vec3> {
  static constexpr size_t kComponentCount = 3;
};
template <>
struct VectorInfo<mathfu::vec4> {
  static constexpr size_t kComponentCount = 4;
};

template <typename T>
SpringT<T> CriticallyDampedSpringLerp(const T& goal_pos,
                                      const SpringT<T>& state0,
                                      const T& spring_factor, float dt) {
  const size_t component_count = VectorInfo<T>::kComponentCount;
  SpringT<T> state1;
  for (size_t i = 0; i != component_count; ++i) {
    CriticallyDampedSpringLerp(goal_pos[i], state0.pos[i], state0.vel[i],
                               spring_factor[i], dt, &state1.pos[i],
                               &state1.vel[i]);
  }
  return state1;
}

template <typename T>
SpringT<T> OverDampedSpringLerp(const T& goal_pos, const SpringT<T>& state0,
                                const T& spring_factor, const T& spring_damp,
                                float dt) {
  const size_t component_count = VectorInfo<T>::kComponentCount;
  SpringT<T> state1;
  for (size_t i = 0; i != component_count; ++i) {
    OverDampedSpringLerp(goal_pos[i], state0.pos[i], state0.vel[i],
                         spring_factor[i], spring_damp[i], dt, &state1.pos[i],
                         &state1.vel[i]);
  }
  return state1;
}

template <typename T>
SpringT<T> CriticallyDampedSpringLerpDegrees(const T& goal_pos,
                                             const SpringT<T>& state0,
                                             const T& spring_factor, float dt) {
  const size_t component_count = VectorInfo<T>::kComponentCount;
  SpringT<T> state1;
  for (size_t i = 0; i != component_count; ++i) {
    CriticallyDampedSpringLerpDegrees(goal_pos[i], state0.pos[i], state0.vel[i],
                                      spring_factor[i], dt, &state1.pos[i],
                                      &state1.vel[i]);
  }
  return state1;
}

template <typename T>
SpringT<T> CriticallyDampedSpringLerpRadians(const T& goal_pos,
                                             const SpringT<T>& state0,
                                             const T& spring_factor, float dt) {
  const size_t component_count = VectorInfo<T>::kComponentCount;
  SpringT<T> state1;
  for (size_t i = 0; i != component_count; ++i) {
    CriticallyDampedSpringLerpRadians(goal_pos[i], state0.pos[i], state0.vel[i],
                                      spring_factor[i], dt, &state1.pos[i],
                                      &state1.vel[i]);
  }
  return state1;
}

template <typename T>
SpringT<T> OverDampedSpringLerpDegrees(const T& goal_pos,
                                       const SpringT<T>& state0,
                                       const T& spring_factor,
                                       const T& spring_damp, float dt) {
  const size_t component_count = VectorInfo<T>::kComponentCount;
  SpringT<T> state1;
  for (size_t i = 0; i != component_count; ++i) {
    OverDampedSpringLerpDegrees(goal_pos[i], state0.pos[i], state0.vel[i],
                                spring_factor[i], spring_damp[i], dt,
                                &state1.pos[i], &state1.vel[i]);
  }
  return state1;
}

template <typename T>
SpringT<T> OverDampedSpringLerpRadians(const T& goal_pos,
                                       const SpringT<T>& state0,
                                       const T& spring_factor,
                                       const T& spring_damp, float dt) {
  const size_t component_count = VectorInfo<T>::kComponentCount;
  SpringT<T> state1;
  for (size_t i = 0; i != component_count; ++i) {
    OverDampedSpringLerpRadians(goal_pos[i], state0.pos[i], state0.vel[i],
                                spring_factor[i], spring_damp[i], dt,
                                &state1.pos[i], &state1.vel[i]);
  }
  return state1;
}

inline SpringT<float> CriticallyDampedSpringLerpDegrees(
    float goal_pos, const SpringT<float>& state0, float spring_factor,
    float dt) {
  SpringT<float> state1;
  CriticallyDampedSpringLerpDegrees(goal_pos, state0.pos, state0.vel,
                                    spring_factor, dt, &state1.pos,
                                    &state1.vel);
  return state1;
}

inline SpringT<float> CriticallyDampedSpringLerpRadians(
    float goal_pos, const SpringT<float>& state0, float spring_factor,
    float dt) {
  SpringT<float> state1;
  CriticallyDampedSpringLerpRadians(goal_pos, state0.pos, state0.vel,
                                    spring_factor, dt, &state1.pos,
                                    &state1.vel);
  return state1;
}

inline SpringT<float> OverDampedSpringLerpDegrees(float goal_pos,
                                                  const SpringT<float>& state0,
                                                  float spring_factor,
                                                  float spring_damp, float dt) {
  SpringT<float> state1;
  OverDampedSpringLerpDegrees(goal_pos, state0.pos, state0.vel, spring_factor,
                              spring_damp, dt, &state1.pos, &state1.vel);
  return state1;
}

inline SpringT<float> OverDampedSpringLerpRadians(float goal_pos,
                                                  const SpringT<float>& state0,
                                                  float spring_factor,
                                                  float spring_damp, float dt) {
  SpringT<float> state1;
  OverDampedSpringLerpRadians(goal_pos, state0.pos, state0.vel, spring_factor,
                              spring_damp, dt, &state1.pos, &state1.vel);
  return state1;
}

}  // namespace lull

#endif  // LULLABY_UTIL_SPRING_H_
