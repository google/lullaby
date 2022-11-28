/*
Copyright 2017-2022 Google Inc. All Rights Reserved.

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

#ifndef REDUX_ENGINES_RENDER_FILAMENT_FILAMENT_UTILS_H_
#define REDUX_ENGINES_RENDER_FILAMENT_FILAMENT_UTILS_H_

#include <functional>
#include <memory>
#include <vector>

#include "filament/Box.h"
#include "filament/Engine.h"
#include "math/mat4.h"
#include "redux/modules/math/bounds.h"
#include "redux/modules/math/matrix.h"

namespace redux {

template <typename T>
using FilamentResourcePtr = std::unique_ptr<T, std::function<void(T*)>>;

template <typename T>
FilamentResourcePtr<T> MakeFilamentResource(T* ptr, filament::Engine* engine) {
  if constexpr (std::is_same_v<T, filament::Camera>) {
    return {ptr, [=](T* camera) {
              engine->destroyCameraComponent(camera->getEntity());
            }};
  } else {
    return {ptr, [=](T* obj) { engine->destroy(obj); }};
  }
}

inline filament::math::mat4f ToFilament(const mat4& src) {
  return filament::math::mat4f{
      filament::math::float4{src(0, 0), src(1, 0), src(2, 0), src(3, 0)},
      filament::math::float4{src(0, 1), src(1, 1), src(2, 1), src(3, 1)},
      filament::math::float4{src(0, 2), src(1, 2), src(2, 2), src(3, 2)},
      filament::math::float4{src(0, 3), src(1, 3), src(2, 3), src(3, 3)}};
}

inline filament::Box ToFilament(const Box& src) {
  filament::Box out;
  const filament::math::float3 min{src.min.x, src.min.y, src.min.z};
  const filament::math::float3 max{src.max.x, src.max.y, src.max.z};
  out.set(min, max);
  return out;
}

class Readiable {
 public:
  using OnReadyFn = std::function<void()>;

  bool IsReady() const { return ready_; }

  void OnReady(const OnReadyFn& cb) {
    if (ready_) {
      cb();
    } else {
      callbacks_.push_back(cb);
    }
  }

 protected:
  void NotifyReady() {
    ready_ = true;
    for (auto& cb : callbacks_) {
      cb();
    }
    callbacks_.clear();
  }

  bool ready_ = false;
  std::vector<OnReadyFn> callbacks_;
};

}  // namespace redux

#endif  // REDUX_ENGINES_RENDER_FILAMENT_FILAMENT_UTILS_H_
