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

#include "redux/engines/render/filament/filament_render_scene.h"

#include "redux/engines/render/filament/filament_indirect_light.h"
#include "redux/engines/render/filament/filament_light.h"
#include "redux/engines/render/filament/filament_render_engine.h"
#include "redux/engines/render/filament/filament_renderable.h"

namespace redux {

FilamentRenderScene::FilamentRenderScene(Registry* registry) {
  fengine_ = redux::GetFilamentEngine(registry);
  fscene_ = MakeFilamentResource(fengine_->createScene(), fengine_);
}

void FilamentRenderScene::Add(const Renderable& renderable) {
  const auto* obj = static_cast<const FilamentRenderable*>(&renderable);
  obj->AddToScene(this);
}

void FilamentRenderScene::Remove(const Renderable& renderable) {
  const auto* obj = static_cast<const FilamentRenderable*>(&renderable);
  obj->RemoveFromScene(this);
}

void FilamentRenderScene::Add(const Light& light) {
  const auto* obj = static_cast<const FilamentLight*>(&light);
  obj->AddToScene(this);
}

void FilamentRenderScene::Remove(const Light& light) {
  const auto* obj = static_cast<const FilamentLight*>(&light);
  obj->RemoveFromScene(this);
}

void FilamentRenderScene::Add(const IndirectLight& light) {
  const auto* obj = static_cast<const FilamentIndirectLight*>(&light);
  obj->AddToScene(this);
}

void FilamentRenderScene::Remove(const IndirectLight& light) {
  const auto* obj = static_cast<const FilamentIndirectLight*>(&light);
  obj->RemoveFromScene(this);
}

}  // namespace redux
