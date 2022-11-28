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

#include "redux/engines/render/render_target_factory.h"

#include "redux/engines/render/filament/filament_render_target.h"

namespace redux {

RenderTargetFactory::RenderTargetFactory(Registry* registry)
    : registry_(registry) {}

RenderTargetPtr RenderTargetFactory::GetRenderTarget(HashValue name) const {
  return render_targets_.Find(name);
}

void RenderTargetFactory::ReleaseRenderTarget(HashValue name) {
  render_targets_.Release(name);
}

RenderTargetPtr RenderTargetFactory::CreateRenderTarget(
    HashValue name, const RenderTargetParams& params) {
  auto impl = std::make_shared<FilamentRenderTarget>(registry_, params);
  auto target = std::static_pointer_cast<RenderTarget>(impl);
  render_targets_.Register(name, target);
  return target;
}

}  // namespace redux
