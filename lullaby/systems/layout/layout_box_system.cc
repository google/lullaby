/*
Copyright 2017 Google Inc. All Rights Reserved.

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

#include "lullaby/systems/layout/layout_box_system.h"

#include "lullaby/events/layout_events.h"
#include "lullaby/modules/script/function_binder.h"
#include "lullaby/systems/dispatcher/event.h"
#include "lullaby/systems/transform/transform_system.h"

namespace lull {

LayoutBoxSystem::LayoutBoxSystem(Registry* registry)
    : System(registry) {
  RegisterDependency<TransformSystem>(this);

  FunctionBinder* binder = registry->Get<FunctionBinder>();
  if (binder) {
    binder->RegisterMethod("lull.LayoutBox.SetOriginalBox",
                           &lull::LayoutBoxSystem::SetOriginalBox);
    binder->RegisterFunction("lull.LayoutBox.GetOriginalBox",
                             [this](Entity e) { return *GetOriginalBox(e); });
    binder->RegisterMethod("lull.LayoutBox.SetDesiredSize",
                           &lull::LayoutBoxSystem::SetDesiredSize);
    binder->RegisterMethod("lull.LayoutBox.GetDesiredSizeX",
                           &lull::LayoutBoxSystem::GetDesiredSizeX);
    binder->RegisterMethod("lull.LayoutBox.GetDesiredSizeY",
                           &lull::LayoutBoxSystem::GetDesiredSizeY);
    binder->RegisterMethod("lull.LayoutBox.GetDesiredSizeZ",
                           &lull::LayoutBoxSystem::GetDesiredSizeZ);
    binder->RegisterMethod("lull.LayoutBox.SetActualBox",
                           &lull::LayoutBoxSystem::SetActualBox);
    binder->RegisterFunction("lull.LayoutBox.GetActualBox",
                             [this](Entity e) { return *GetActualBox(e); });
  }
}

LayoutBoxSystem::~LayoutBoxSystem() {
  FunctionBinder* binder = registry_->Get<FunctionBinder>();
  if (binder) {
    binder->UnregisterFunction("lull.LayoutBox.SetOriginalBox");
    binder->UnregisterFunction("lull.LayoutBox.GetOriginalBox");
    binder->UnregisterFunction("lull.LayoutBox.SetDesiredSize");
    binder->UnregisterFunction("lull.LayoutBox.GetDesiredSizeX");
    binder->UnregisterFunction("lull.LayoutBox.GetDesiredSizeY");
    binder->UnregisterFunction("lull.LayoutBox.GetDesiredSizeZ");
    binder->UnregisterFunction("lull.LayoutBox.SetActualBox");
    binder->UnregisterFunction("lull.LayoutBox.GetActualBox");
  }
}

void LayoutBoxSystem::Destroy(Entity e) { layout_boxes_.erase(e); }

void LayoutBoxSystem::SetOriginalBox(Entity e, const Aabb& original_box) {
  auto* layout_box = GetOrCreateLayoutBox(e);
  if (layout_box == nullptr) {
    return;
  }
  layout_box->original_box = original_box;
  // Also sets actual_box in case a Client doesn't support
  // DesiredSizeChangedEvent.
  // Layouts should be using actual_box for final calculations.
  layout_box->actual_box = original_box;

  SendEvent(registry_, e, OriginalBoxChangedEvent(e));
}

const Aabb* LayoutBoxSystem::GetOriginalBox(Entity e) const {
  const auto* layout_box = GetLayoutBox(e);
  if (layout_box && layout_box->original_box) {
    return layout_box->original_box.get();
  } else {
    return registry_->Get<TransformSystem>()->GetAabb(e);
  }
}

void LayoutBoxSystem::SetDesiredSize(Entity e, Entity source,
                                     const Optional<float>& x,
                                     const Optional<float>& y,
                                     const Optional<float>& z) {
  auto* layout_box = GetOrCreateLayoutBox(e);
  if (layout_box == nullptr) {
    return;
  }
  layout_box->desired_size_x = x;
  layout_box->desired_size_y = y;
  layout_box->desired_size_z = z;

  // DesiredSizeChangedEvent is sent *immediately* so that fast Clients can fix
  // their mesh before Layouts perform positioning calculations.
  SendEventImmediately(registry_, e,
                       DesiredSizeChangedEvent(e, source, x, y, z));
}

Optional<float> LayoutBoxSystem::GetDesiredSizeX(Entity e) const {
  const auto* layout_box = GetLayoutBox(e);
  return layout_box ? layout_box->desired_size_x : Optional<float>();
}

Optional<float> LayoutBoxSystem::GetDesiredSizeY(Entity e) const {
  const auto* layout_box = GetLayoutBox(e);
  return layout_box ? layout_box->desired_size_y : Optional<float>();
}

Optional<float> LayoutBoxSystem::GetDesiredSizeZ(Entity e) const {
  const auto* layout_box = GetLayoutBox(e);
  return layout_box ? layout_box->desired_size_z : Optional<float>();
}

void LayoutBoxSystem::SetActualBox(Entity e, Entity source,
                                   const Aabb& actual_box) {
  auto* layout_box = GetOrCreateLayoutBox(e);
  if (layout_box == nullptr) {
    return;
  }
  layout_box->actual_box = actual_box;

  SendEvent(registry_, e, ActualBoxChangedEvent(e, source));
}

const Aabb* LayoutBoxSystem::GetActualBox(Entity e) const {
  const auto* layout_box = GetLayoutBox(e);
  if (layout_box && layout_box->actual_box) {
    return layout_box->actual_box.get();
  } else {
    return registry_->Get<TransformSystem>()->GetAabb(e);
  }
}

LayoutBoxSystem::LayoutBox*
    LayoutBoxSystem::GetOrCreateLayoutBox(Entity e) {
  auto iter = layout_boxes_.find(e);
  if (iter == layout_boxes_.end()) {
    iter = layout_boxes_.emplace(e, LayoutBox()).first;
  }
  return &iter->second;
}

const LayoutBoxSystem::LayoutBox*
    LayoutBoxSystem::GetLayoutBox(Entity e) const {
  const auto iter = layout_boxes_.find(e);
  if (iter != layout_boxes_.end()) {
    return &iter->second;
  } else {
    return nullptr;
  }
}

}  // namespace lull
