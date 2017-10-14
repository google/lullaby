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

#include "lullaby/systems/layout/layout_system.h"

#include "lullaby/generated/layout_def_generated.h"
#include "lullaby/events/render_events.h"
#include "lullaby/modules/animation_channels/transform_channels.h"
#include "lullaby/modules/dispatcher/dispatcher.h"
#include "lullaby/modules/ecs/entity_factory.h"
#include "lullaby/modules/flatbuffers/mathfu_fb_conversions.h"
#include "lullaby/modules/script/function_binder.h"
#include "lullaby/systems/animation/animation_system.h"
#include "lullaby/systems/dispatcher/dispatcher_system.h"
#include "lullaby/systems/dispatcher/event.h"
#include "lullaby/systems/layout/layout_box_system.h"
#include "lullaby/systems/transform/transform_system.h"
#include "lullaby/util/logging.h"
#include "lullaby/util/math.h"
#include "lullaby/util/time.h"

namespace {
struct LayoutDirtyEvent {
  LayoutDirtyEvent() {}

  template <typename Archive>
  void Serialize(Archive archive) {}
};
const lull::HashValue kLayoutDef = lull::Hash("LayoutDef");
const lull::HashValue kLayoutElementDef = lull::Hash("LayoutElementDef");
const lull::HashValue kRadialLayoutDef = lull::Hash("RadialLayoutDef");
}  // namespace
LULLABY_SETUP_TYPEID(LayoutDirtyEvent);

namespace lull {

LayoutSystem::LayoutComponent::LayoutComponent(Entity e) : Component(e) {}

LayoutSystem::LayoutSystem(Registry* registry)
    : System(registry), layouts_(8), layout_elements_(16) {
  RegisterDef(this, kLayoutDef);
  RegisterDef(this, kLayoutElementDef);
  RegisterDef(this, kRadialLayoutDef);
  RegisterDependency<TransformSystem>(this);
  RegisterDependency<LayoutBoxSystem>(this);

  Dispatcher* dispatcher = registry_->Get<Dispatcher>();
  dispatcher->Connect(this, [this](const ParentChangedEvent& event) {
    OnParentChanged(event);
  });
  dispatcher->Connect(this, [this](const OriginalBoxChangedEvent& event) {
    OnOriginalBoxChanged(event.target);
  });
  dispatcher->Connect(this, [this](const DesiredSizeChangedEvent& event) {
    OnDesiredSizeChanged(event);
  });
  dispatcher->Connect(this, [this](const AabbChangedEvent& event) {
    OnAabbChanged(event.target);
  });
  dispatcher->Connect(this, [this](const ActualBoxChangedEvent& event) {
    OnActualBoxChanged(event);
  });
  // Process any layout changed events, such as parent changed, on the next
  // frame, so that we don't wind up redoing the layout for each added child.
  dispatcher->Connect(this, [this](const LayoutDirtyEvent& event) {
    ProcessDirty();
  });

  FunctionBinder* binder = registry_->Get<FunctionBinder>();
  if (binder) {
    binder->RegisterMethod("lull.Layout.SetCanvasSizeX",
                           &lull::LayoutSystem::SetCanvasSizeX);
    binder->RegisterMethod("lull.Layout.SetCanvasSizeY",
                           &lull::LayoutSystem::SetCanvasSizeY);
    binder->RegisterMethod("lull.Layout.SetSpacingX",
                           &lull::LayoutSystem::SetSpacingX);
    binder->RegisterMethod("lull.Layout.SetSpacingY",
                           &lull::LayoutSystem::SetSpacingY);
    binder->RegisterFunction("lull.Layout.SetFillOrder",
                             [this](Entity e, int fill_order) {
      SetFillOrder(e, static_cast<LayoutFillOrder>(fill_order));
    });
    binder->RegisterFunction("lull.Layout.SetHorizontalAlignment",
                             [this](Entity e, int horizontal_alignment) {
      SetHorizontalAlignment(e,
          static_cast<LayoutHorizontalAlignment>(horizontal_alignment));
    });
    binder->RegisterFunction("lull.Layout.SetVerticalAlignment",
                              [this](Entity e, int vertical_alignment) {
      SetVerticalAlignment(e,
          static_cast<LayoutVerticalAlignment>(vertical_alignment));
    });
    binder->RegisterFunction("lull.Layout.SetRowAlignment",
                              [this](Entity e, int row_alignment) {
      SetRowAlignment(e,
          static_cast<LayoutVerticalAlignment>(row_alignment));
    });
    binder->RegisterFunction("lull.Layout.SetColumnAlignment",
                              [this](Entity e, int column_alignment) {
      SetColumnAlignment(e,
          static_cast<LayoutHorizontalAlignment>(column_alignment));
    });
    binder->RegisterMethod("lull.Layout.SetElementsPerWrap",
                           &lull::LayoutSystem::SetElementsPerWrap);
    binder->RegisterMethod("lull.Layout.SetMaxElements",
                           &lull::LayoutSystem::SetMaxElements);

    // Expose enums for use in scripts.  These are functions you will need to
    // call (with parentheses)
    binder->RegisterFunction("lull.Layout.LayoutFillOrder.RightDown", []() {
      return static_cast<int>(LayoutFillOrder_RightDown);
    });
    binder->RegisterFunction("lull.Layout.LayoutFillOrder.LeftDown", []() {
      return static_cast<int>(LayoutFillOrder_LeftDown);
    });
    binder->RegisterFunction("lull.Layout.LayoutFillOrder.DownRight", []() {
      return static_cast<int>(LayoutFillOrder_DownRight);
    });
    binder->RegisterFunction("lull.Layout.LayoutFillOrder.DownLeft", []() {
      return static_cast<int>(LayoutFillOrder_DownLeft);
    });
    binder->RegisterFunction("lull.Layout.LayoutFillOrder.RightUp", []() {
      return static_cast<int>(LayoutFillOrder_RightUp);
    });
    binder->RegisterFunction("lull.Layout.LayoutFillOrder.LeftUp", []() {
      return static_cast<int>(LayoutFillOrder_LeftUp);
    });
    binder->RegisterFunction("lull.Layout.LayoutFillOrder.UpRight", []() {
      return static_cast<int>(LayoutFillOrder_UpRight);
    });
    binder->RegisterFunction("lull.Layout.LayoutFillOrder.UpLeft", []() {
      return static_cast<int>(LayoutFillOrder_UpLeft);
    });

    binder->RegisterFunction("lull.Layout.LayoutHorizontalAlignment.Left",
        []() {
      return static_cast<int>(LayoutHorizontalAlignment_Left);
    });
    binder->RegisterFunction("lull.Layout.LayoutHorizontalAlignment.Center",
        []() {
      return static_cast<int>(LayoutHorizontalAlignment_Center);
    });
    binder->RegisterFunction("lull.Layout.LayoutHorizontalAlignment.Right",
        []() {
      return static_cast<int>(LayoutHorizontalAlignment_Right);
    });

    binder->RegisterFunction("lull.Layout.LayoutVerticalAlignment.Top",
        []() {
      return static_cast<int>(LayoutVerticalAlignment_Top);
    });
    binder->RegisterFunction("lull.Layout.LayoutVerticalAlignment.Center",
        []() {
      return static_cast<int>(LayoutVerticalAlignment_Center);
    });
    binder->RegisterFunction("lull.Layout.LayoutVerticalAlignment.Bottom",
        []() {
      return static_cast<int>(LayoutVerticalAlignment_Bottom);
    });
  }
}

LayoutSystem::~LayoutSystem() {
  Dispatcher* dispatcher = registry_->Get<Dispatcher>();
  dispatcher->DisconnectAll(this);

  FunctionBinder* binder = registry_->Get<FunctionBinder>();
  if (binder) {
    binder->UnregisterFunction("lull.Layout.SetCanvasSizeX");
    binder->UnregisterFunction("lull.Layout.SetCanvasSizeY");
    binder->UnregisterFunction("lull.Layout.SetSpacingX");
    binder->UnregisterFunction("lull.Layout.SetSpacingY");
    binder->UnregisterFunction("lull.Layout.SetFillOrder");
    binder->UnregisterFunction("lull.Layout.SetHorizontalAlignment");
    binder->UnregisterFunction("lull.Layout.SetVerticalAlignment");
    binder->UnregisterFunction("lull.Layout.SetRowAlignment");
    binder->UnregisterFunction("lull.Layout.SetColumnAlignment");
    binder->UnregisterFunction("lull.Layout.SetElementsPerWrap");
    binder->UnregisterFunction("lull.Layout.SetMaxElements");

    binder->UnregisterFunction("lull.Layout.LayoutFillOrder.RightDown");
    binder->UnregisterFunction("lull.Layout.LayoutFillOrder.LeftDown");
    binder->UnregisterFunction("lull.Layout.LayoutFillOrder.DownRight");
    binder->UnregisterFunction("lull.Layout.LayoutFillOrder.DownLeft");
    binder->UnregisterFunction("lull.Layout.LayoutFillOrder.RightUp");
    binder->UnregisterFunction("lull.Layout.LayoutFillOrder.LeftUp");
    binder->UnregisterFunction("lull.Layout.LayoutFillOrder.UpRight");
    binder->UnregisterFunction("lull.Layout.LayoutFillOrder.UpLeft");

    binder->UnregisterFunction("lull.Layout.LayoutHorizontalAlignment.Left");
    binder->UnregisterFunction("lull.Layout.LayoutHorizontalAlignment.Center");
    binder->UnregisterFunction("lull.Layout.LayoutHorizontalAlignment.Right");

    binder->UnregisterFunction("lull.Layout.LayoutVerticalAlignment.Top");
    binder->UnregisterFunction("lull.Layout.LayoutVerticalAlignment.Center");
    binder->UnregisterFunction("lull.Layout.LayoutVerticalAlignment.Bottom");
  }
}

void LayoutSystem::Create(Entity e, HashValue type, const Def* def) {
  if (type == kLayoutElementDef) {
    auto pair = layout_elements_.emplace(e, LayoutElement(e));
    LayoutElement& layout_element = pair.first->second;
    const auto* data = ConvertDef<LayoutElementDef>(def);

    layout_element.horizontal_weight = data->horizontal_weight();
    layout_element.vertical_weight = data->vertical_weight();
    layout_element.duration = DurationFromMilliseconds(data->duration_ms());
  } else if (type == kLayoutDef) {
    LayoutComponent* layout = layouts_.Emplace(e);
    const auto* data = ConvertDef<LayoutDef>(def);

    if (data->empty_blueprint()) {
      layout->empty_blueprint = data->empty_blueprint()->c_str();
      SetDirty(e, kOriginal);
    }
    layout->max_elements = static_cast<size_t>(data->max_elements());

    layout->layout.reset(new LayoutParams());

    MathfuVec2FromFbVec2(data->canvas_size(), &layout->layout->canvas_size);
    MathfuVec2FromFbVec2(data->spacing(), &layout->layout->spacing);
    layout->layout->elements_per_wrap =
        static_cast<size_t>(data->elements_per_wrap());

    layout->layout->horizontal_alignment = data->horizontal_alignment();
    layout->layout->vertical_alignment = data->vertical_alignment();
    layout->layout->row_alignment = data->row_alignment();
    layout->layout->column_alignment = data->column_alignment();
    layout->layout->fill_order = data->fill_order();
    layout->layout->shrink_to_fit = data->shrink_to_fit();
  } else if (type == kRadialLayoutDef) {
    LayoutComponent* layout = layouts_.Emplace(e);
    const auto* data = ConvertDef<RadialLayoutDef>(def);

    if (data->empty_blueprint()) {
      layout->empty_blueprint = data->empty_blueprint()->c_str();
      SetDirty(e, kOriginal);
    }
    layout->max_elements = static_cast<size_t>(data->max_elements());

    layout->radial_layout.reset(new RadialLayoutParams());

    if (data->degrees_per_element() != 0) {
      layout->radial_layout->degrees_per_element = data->degrees_per_element();
    }
    MathfuVec3FromFbVec3(data->major_axis(),
                         &layout->radial_layout->major_axis);
    MathfuVec3FromFbVec3(data->minor_axis(),
                         &layout->radial_layout->minor_axis);
  } else {
    LOG(DFATAL) << "Invalid type passed to Create. Expecting RadialLayoutDef, "
                   "LayoutDef, or LayoutElementDef!";
  }
}

void LayoutSystem::Create(Entity e, const LayoutParams& params) {
  LayoutComponent *layout = layouts_.Emplace(e);
  layout->layout.reset(new LayoutParams(params));
}

void LayoutSystem::Destroy(Entity e) {
  layouts_.Destroy(e);
  layout_elements_.erase(e);
}

void LayoutSystem::SetLayoutParams(Entity e, const LayoutParams& params) {
  auto layout = layouts_.Get(e);
  if (layout) {
    layout->layout.reset(new LayoutParams(params));
    SetDirty(e, kOriginal);
  }
}

void LayoutSystem::SetCanvasSizeX(Entity e, float x) {
  auto layout = layouts_.Get(e);
  if (layout == nullptr || !layout->layout) {
    return;
  }

  layout->layout->canvas_size.x = x;
  SetDirty(e, kOriginal);
}

void LayoutSystem::SetCanvasSizeY(Entity e, float y) {
  auto layout = layouts_.Get(e);
  if (layout == nullptr || !layout->layout) {
    return;
  }

  layout->layout->canvas_size.y = y;
  SetDirty(e, kOriginal);
}

void LayoutSystem::SetSpacingX(Entity e, float x) {
  auto layout = layouts_.Get(e);
  if (layout == nullptr || !layout->layout) {
    return;
  }

  layout->layout->spacing.x = x;
  SetDirty(e, kOriginal);
}

void LayoutSystem::SetSpacingY(Entity e, float y) {
  auto layout = layouts_.Get(e);
  if (layout == nullptr || !layout->layout) {
    return;
  }

  layout->layout->spacing.y = y;
  SetDirty(e, kOriginal);
}

void LayoutSystem::SetFillOrder(Entity e, LayoutFillOrder fill_order) {
  auto layout = layouts_.Get(e);
  if (layout == nullptr || !layout->layout) {
    return;
  }

  layout->layout->fill_order = fill_order;
  SetDirty(e, kOriginal);
}

void LayoutSystem::SetHorizontalAlignment(Entity e,
    LayoutHorizontalAlignment horizontal_alignment) {
  auto layout = layouts_.Get(e);
  if (layout == nullptr || !layout->layout) {
    return;
  }

  layout->layout->horizontal_alignment = horizontal_alignment;
  SetDirty(e, kOriginal);
}

void LayoutSystem::SetVerticalAlignment(Entity e,
    LayoutVerticalAlignment vertical_alignment) {
  auto layout = layouts_.Get(e);
  if (layout == nullptr || !layout->layout) {
    return;
  }

  layout->layout->vertical_alignment = vertical_alignment;
  SetDirty(e, kOriginal);
}

void LayoutSystem::SetRowAlignment(Entity e,
    LayoutVerticalAlignment row_alignment) {
  auto layout = layouts_.Get(e);
  if (layout == nullptr || !layout->layout) {
    return;
  }

  layout->layout->row_alignment = row_alignment;
  SetDirty(e, kOriginal);
}

void LayoutSystem::SetColumnAlignment(Entity e,
    LayoutHorizontalAlignment column_alignment) {
  auto layout = layouts_.Get(e);
  if (layout == nullptr || !layout->layout) {
    return;
  }

  layout->layout->column_alignment = column_alignment;
  SetDirty(e, kOriginal);
}

void LayoutSystem::SetElementsPerWrap(Entity e, int elements_per_wrap) {
  auto layout = layouts_.Get(e);
  if (layout == nullptr || !layout->layout) {
    return;
  }

  layout->layout->elements_per_wrap = elements_per_wrap;
  SetDirty(e, kOriginal);
}

void LayoutSystem::SetMaxElements(Entity e, int max_elements) {
  auto layout = layouts_.Get(e);
  if (layout == nullptr) {
    return;
  }

  layout->max_elements = max_elements;
  SetDirty(e, kOriginal);
}

size_t LayoutSystem::GetInsertIndexForPosition(Entity entity,
      const mathfu::vec3& world_position) {
  const auto* transform_system = registry_->Get<TransformSystem>();
  const LayoutComponent* layout = layouts_.Get(entity);
  if (!layout || !layout->layout) {
    LOG(DFATAL) << "No layout component for entity: " << entity;
    return 0;
  }

  const mathfu::mat4* world_mat =
      transform_system->GetWorldFromEntityMatrix(entity);
  if (!world_mat) {
    LOG(DFATAL) << "No transform matrix for entity: " << entity;
    return 0;
  }

  // Translate the world coordinates into this entity's local coordinates to
  // compare with the laid-out elements' positions.
  const mathfu::vec3 local_position = world_mat->Inverse() * world_position;
  auto index = CalculateInsertIndexForPosition(layout->cached_positions,
                                               local_position);
  return index;
}

void LayoutSystem::Layout(Entity e) {
  LayoutImpl(DirtyLayout(e, kOriginal));
}

void LayoutSystem::SetDuration(Entity element, Clock::duration duration) {
  GetLayoutElement(element).duration = duration;
}

void LayoutSystem::SetLayoutPosition(Entity entity,
                                     const mathfu::vec2& position) {
  auto* animation_system = registry_->Get<AnimationSystem>();
  auto* transform_system = registry_->Get<TransformSystem>();
  auto& layout_element = GetLayoutElement(entity);
  // Preserve the z, only change xy.
  const mathfu::vec3 translation(
      position, transform_system->GetLocalTranslation(entity).z);

  if (!animation_system || layout_element.first ||
      layout_element.duration <= Clock::duration::zero()) {
    transform_system->SetLocalTranslation(entity, translation);
  } else {
    animation_system->SetTarget(entity, PositionChannel::kChannelName,
                                &translation[0], 3, layout_element.duration);
  }
  layout_element.first = false;
}

// When the parameters for determining a Layout change, e.g.
// OriginalBoxChanged, ParentChangedEvent, or any change in LayoutParams, then
// the Layout will set its original_size and its children's desired_size.
// When it is setting original_size it use its original canvas_size.
//
// OnDesiredSizeChanged, it does not set its original_size, only actual_size.
// When it sets actual_size it uses its assigned desired_size if previously set.
//
// OnActualBoxChanged, it does not set its original_size or any children's
// desired_size.
// However, if it is the source of an ActualBoxChangedEvent, it will
// SetOriginal instead of SetActual, since that event was a result of one of
// its previous kOriginal passes, but still won't set any children's
// desired_size.
void LayoutSystem::LayoutImpl(const DirtyLayout& dirty_layout) {
  const Entity e = dirty_layout.GetLayout();
  LayoutComponent* layout = layouts_.Get(e);
  auto* transform_system = registry_->Get<TransformSystem>();
  if (layout == nullptr) {
    return;
  }

  const std::vector<Entity>* children = transform_system->GetChildren(e);
  if (children == nullptr) {
    return;
  }

  // If needed, add empty placeholders to fill up the layout.
  auto* entity_factory = registry_->Get<EntityFactory>();
  if (!layout->empty_blueprint.empty()) {
    while (children->size() < layout->max_elements) {
      Entity empty_placeholder = transform_system->CreateChild(e,
          layout->empty_blueprint);
      if (empty_placeholder == kNullEntity) {
        LOG(WARNING) << "could not find blueprint and create placeholders";
        break;
      }
      layout->empty_placeholders.push(empty_placeholder);
    }
  }
  // And, remove placeholders if they aren't needed anymore.
  while (children->size() > layout->max_elements &&
         !layout->empty_placeholders.empty()) {
    entity_factory->Destroy(layout->empty_placeholders.front());
    layout->empty_placeholders.pop();
  }
  if (layout->max_elements > 0 && children->size() > layout->max_elements) {
    LOG(WARNING) << "layout " << e << " has more children than max_elements";
  }

  if (layout->layout) {
    std::vector<LayoutElement> elements;
    elements.reserve(children->size());
    for (const Entity& child : *children) {
      elements.emplace_back(GetLayoutElement(child));
    }

    auto* layout_box_system = registry_->Get<LayoutBoxSystem>();
    LayoutParams params = *layout->layout;
    if (dirty_layout.ShouldUseDesiredSize()) {
      // Use assigned desired_size if it has been set any time the original_size
      // is not calculated from scratch.
      Optional<float> x = layout_box_system->GetDesiredSizeX(e);
      Optional<float> y = layout_box_system->GetDesiredSizeY(e);
      if (x) {
        params.canvas_size.x = *x;
      }
      if (y) {
        params.canvas_size.y = *y;
      }
    }
    const auto set_pos_fn = [this](Entity entity, const mathfu::vec2& pos) {
      SetLayoutPosition(entity, pos);
    };
    const Aabb aabb = ApplyLayout(registry_, params, elements, set_pos_fn,
                                  dirty_layout.GetChildrensDesiredSource(),
                                  &layout->cached_positions);
    transform_system->SetAabb(e, aabb);

    if (dirty_layout.ShouldSetActualBox()) {
      layout_box_system->SetActualBox(e, dirty_layout.GetActualSource(), aabb);
    } else {
      layout_box_system->SetOriginalBox(e, aabb);
    }
  } else if (layout->radial_layout) {
    ApplyRadialLayout(registry_, *children, *layout->radial_layout);
  } else {
    LOG(DFATAL) << "Cannot layout LayoutComponent with no layout parameters.";
    return;
  }

  SendEvent(registry_, e, LayoutChangedEvent(e));
}

LayoutElement& LayoutSystem::GetLayoutElement(Entity e) {
  // Create a default element (weights = 0) if it doesn't exist yet, or get the
  // current one.
  auto iter = layout_elements_.find(e);
  if (iter != layout_elements_.end()) {
    return iter->second;
  } else {
    return layout_elements_.emplace(e, LayoutElement(e)).first->second;
  }
}

void LayoutSystem::ProcessDirty() {
  // Copy dirty layouts in case the Dispatcher is not Queued.
  std::unordered_map<Entity, DirtyLayout> dirty_layouts_copy;
  using std::swap;
  swap(dirty_layouts_copy, dirty_layouts_);
  for (const auto& pair : dirty_layouts_copy) {
    LayoutImpl(pair.second);
  }
}

void LayoutSystem::SetDirty(Entity e, LayoutPass pass, Entity source) {
  const bool was_clean = dirty_layouts_.empty();
  // Insert this before sending event in case the Dispatcher is not Queued.
  auto iter = dirty_layouts_.find(e);
  if (iter == dirty_layouts_.end()) {
    dirty_layouts_.emplace(e, DirtyLayout(e, pass, source));
  } else {
    iter->second.Update(registry_, pass, source);
  }
  if (was_clean) {
    Dispatcher* dispatcher = registry_->Get<Dispatcher>();
    dispatcher->Send(LayoutDirtyEvent());
  }
}

void LayoutSystem::SetParentDirty(Entity e, LayoutPass pass, Entity source) {
  const auto* transform_system = registry_->Get<TransformSystem>();
  Entity parent = transform_system->GetParent(e);
  if (parent != kNullEntity && layouts_.Get(parent)) {
    SetDirty(parent, pass, source);
  }
}

void LayoutSystem::OnParentChanged(const ParentChangedEvent& ev) {
  if (layouts_.Get(ev.new_parent)) {
    SetDirty(ev.new_parent, kOriginal);
  }
  if (layouts_.Get(ev.old_parent)) {
    SetDirty(ev.old_parent, kOriginal);
  }
}

void LayoutSystem::OnOriginalBoxChanged(Entity e) {
  // A changed mesh means the layout of the parent entity needs to be updated
  // (since the item might have changed size and require it's siblings to move).
  SetParentDirty(e, kOriginal);
}

void LayoutSystem::OnDesiredSizeChanged(const DesiredSizeChangedEvent& event) {
  if (layouts_.Get(event.target)) {
    SetDirty(event.target, kDesired, event.source);
  }
}

void LayoutSystem::OnAabbChanged(Entity e) {
  // A changed mesh means the layout of the parent entity needs to be updated
  // (since the item might have changed size and require it's siblings to move).
  SetParentDirty(e, kActual);
}

void LayoutSystem::OnActualBoxChanged(const ActualBoxChangedEvent& event) {
  // A changed mesh means the layout of the parent entity needs to be updated
  // (since the item might have changed size and require it's siblings to move).
  SetParentDirty(event.target, kActual, event.source);
}

LayoutSystem::DirtyLayout::DirtyLayout(Entity layout, LayoutPass pass,
                                       Entity source)
    : layout_(layout), pass_(pass) {
  switch (pass_) {
    case kOriginal:
      // Not used.
      break;
    case kDesired:
      desired_source_ = source;
      actual_source_ = source;
      break;
    case kActual:
      actual_source_ = source;
      break;
  }
}

void LayoutSystem::DirtyLayout::Update(Registry* registry, LayoutPass new_pass,
                                       Entity source) {
  // Keep highest priority (highest is kOriginal = 2)
  pass_ = std::max(pass_, new_pass);
  switch (new_pass) {
    case kOriginal:
      // Not used.
      break;
    case kDesired:
      desired_source_ = FindClosestParent(registry, desired_source_, source);
      actual_source_ = FindClosestParent(registry, actual_source_, source);
      break;
    case kActual:
      actual_source_ = FindClosestParent(registry, actual_source_, source);
      break;
  }
}

Entity LayoutSystem::DirtyLayout::FindClosestParent(Registry* registry,
                                                    Entity old_source,
                                                    Entity new_source) const {
  if (old_source == kNullEntity) {
    return new_source;
  }
  if (new_source == kNullEntity) {
    return old_source;
  }
  auto* transform_system = registry->Get<TransformSystem>();
  Entity entity = layout_;
  while (entity != kNullEntity) {
    if (old_source == entity || new_source == entity) {
      return entity;
    }
    entity = transform_system->GetParent(entity);
  }
  // Couldn't find either, leave unchanged.
  return old_source;
}

bool LayoutSystem::DirtyLayout::ShouldUseDesiredSize() const {
  switch (pass_) {
    case kOriginal:
      return false;
    case kDesired:
    case kActual:
      return true;
  }
  LOG(DFATAL) << "Unknown LayoutPass " << pass_;
  return true;
}

bool LayoutSystem::DirtyLayout::ShouldSetActualBox() const {
  return ShouldUseDesiredSize() && actual_source_ != layout_;
}

Entity LayoutSystem::DirtyLayout::GetChildrensDesiredSource() const {
  switch (pass_) {
    case kOriginal:
      return layout_;
    case kDesired:
      return desired_source_;
    case kActual:
      return kNullEntity;
  }
  LOG(DFATAL) << "Unknown LayoutPass " << pass_;
  return kNullEntity;
}

}  // namespace lull
