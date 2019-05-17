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

#include "lullaby/contrib/grab/grab_system.h"

#include "lullaby/generated/grab_def_generated.h"
#include "lullaby/contrib/mutator/mutator_system.h"
#include "lullaby/systems/dispatcher/event.h"
#include "lullaby/contrib/input_behavior/input_behavior_system.h"
#include "lullaby/systems/transform/transform_system.h"
#include "lullaby/modules/flatbuffers/common_fb_conversions.h"

namespace lull {

namespace {
const HashValue kGrabDef = ConstHash("GrabDef");
}  // namespace

GrabSystem::GrabSystem(Registry* registry) : System(registry) {
  RegisterDependency<MutatorSystem>(this);
  RegisterDependency<TransformSystem>(this);
  RegisterDef<GrabDefT>(this);
}

void GrabSystem::Create(Entity entity, HashValue type, const Def* def) {
  if (type != kGrabDef) {
    LOG(DFATAL) << "Invalid type passed to Create. Expecting GrabDef!";
    return;
  }

  const auto* data = ConvertDef<GrabDef>(def);
  // Note: grabbable may already exist if any mutators or input interfaces have
  // already been set.
  Grabbable& grabbable = grabbables_[entity];

  const HashValue group = data->group();
  grabbable.group = group;
  grabbable.snap_to_final = data->snap_to_final();

  InputManager::DeviceType device =
      TranslateInputDeviceType(data->default_device());

  if (data->grab_events()) {
    ConnectEventDefs(
        registry_, entity, data->grab_events(),
        [this, entity, device](const EventWrapper&) { Grab(entity, device); });
  }
  if (data->release_events()) {
    ConnectEventDefs(registry_, entity, data->release_events(),
                     [this, entity](const EventWrapper&) { Release(entity); });
  }

  auto* input_behavior_system = registry_->Get<InputBehaviorSystem>();
  if (input_behavior_system) {
    input_behavior_system->SetDraggable(entity, true);
  }
}

void GrabSystem::Destroy(Entity entity) {
  EndGrab(entity, kDestroyed);
  grabbables_.erase(entity);
}

void GrabSystem::AdvanceFrame(const Clock::duration& delta_time) {
  auto* transform_system = registry_->Get<TransformSystem>();
  auto* mutator_system = registry_->Get<MutatorSystem>();
  std::vector<Entity> canceled_grabs;
  for (Entity grabbed : grabbed_) {
    Grabbable& grabbable = grabbables_[grabbed];
    const Sqt* original = transform_system->GetSqt(grabbed);
    if (!original) {
      LOG(DFATAL) << "missing Transform in GrabSystem::AdvanceFrame.";
      continue;
    }
    // Get the ideal sqt based on the input.
    Sqt current = grabbable.input->UpdateGrab(grabbed, grabbable.holding_device,
                                              *original);

    // Apply any mutations to the ideal sqt to get the actual sqt.
    mutator_system->ApplySqtMutator(grabbed, grabbable.group, &current, false);

    transform_system->SetSqt(grabbed, current);

    // Check if the mutated sqt is still valid for the current input.
    if (grabbable.input->ShouldCancel(grabbed, grabbable.holding_device)) {
      canceled_grabs.push_back(grabbed);
    }
  }
  for (auto canceled : canceled_grabs) {
    Cancel(canceled);
  }
}

void GrabSystem::SetInputHandler(Entity entity, GrabInputInterface* handler) {
  auto iter = grabbables_.find(entity);
  if (iter == grabbables_.end()) {
    // Grabbable didn't already exist, so just create it, add the handler, and
    // return.  This happens when another system registers an Input handler
    // the GrabDef is processed.
    grabbables_[entity].input = handler;
    return;
  }
  Grabbable& grabbable = iter->second;

  // If the entity was already being held, end the previous grab.
  const InputManager::DeviceType holding_device = grabbable.holding_device;
  if (holding_device != InputManager::kMaxNumDeviceTypes) {
    grabbable.input->EndGrab(entity, grabbable.holding_device);
  }

  // Update the handler
  grabbable.input = handler;

  // If the entity was being held, start the new handler.
  if (holding_device != InputManager::kMaxNumDeviceTypes) {
    const bool grab_worked = grabbable.input->StartGrab(entity, holding_device);
    if (!grab_worked) {
      Release(entity);
    }
  }
}

void GrabSystem::RemoveInputHandler(Entity entity,
                                    GrabInputInterface* handler) {
  auto iter = grabbables_.find(entity);
  if (iter == grabbables_.end()) {
    // Grabbable doesn't exist, so just early out.
    return;
  }
  if (iter->second.input == handler) {
    // Cancel the grab if the entity is being held.
    Cancel(entity);

    iter->second.input = nullptr;
  }
}

void GrabSystem::SetMutateGroup(Entity entity, HashValue group) {
  grabbables_[entity].group = group;
}

void GrabSystem::Grab(Entity entity, InputManager::DeviceType device) {
  auto iter = grabbables_.find(entity);
  if (iter == grabbables_.end()) {
    LOG(DFATAL) << "Grab called on entity that does not have a GrabDef.";
    return;
  }
  Grabbable& grabbable = iter->second;

  if (!grabbable.input) {
    LOG(DFATAL) << "Must set an input handler before an entity can be grabbed.";
    return;
  }

  auto* transform_system = registry_->Get<TransformSystem>();
  const Sqt* original = transform_system->GetSqt(entity);
  if (!original) {
    LOG(DFATAL) << "Grab called on entity that does not have a Transform.";
    return;
  }

  grabbable.holding_device = device;
  grabbable.starting_sqt = *original;
  grabbed_.insert(entity);

  const bool grab_worked = grabbable.input->StartGrab(entity, device);
  if (!grab_worked) {
    Cancel(entity);
  }
}

void GrabSystem::Release(Entity entity) { EndGrab(entity, kReleased); }

void GrabSystem::Cancel(Entity entity) { EndGrab(entity, kCanceled); }

void GrabSystem::EndGrab(Entity entity, EndGrabType type) {
  auto iter = grabbables_.find(entity);
  if (iter == grabbables_.end()) {
    if (type != kDestroyed) {
      LOG(DFATAL) << "EndGrab called on entity that does not have a GrabDef.";
    }
    return;
  }
  Grabbable& grabbable = iter->second;
  if (grabbable.holding_device == InputManager::kMaxNumDeviceTypes) {
    // Tried to release something that isn't actually being held.  This can
    // happen easily when there are multiple release conditions, so just return.
    return;
  }
  if (!grabbable.input) {
    LOG(DFATAL)
        << "Must set an input handler before an entity can be released.";
    return;
  }

  auto* transform_system = registry_->Get<TransformSystem>();
  auto* dispatcher = registry_->Get<Dispatcher>();
  auto* dispatcher_system = registry_->Get<DispatcherSystem>();
  if (type == kCanceled) {
    if (grabbable.snap_to_final) {
      transform_system->SetSqt(entity, grabbable.starting_sqt);
    }
    dispatcher->Send(GrabCanceledEvent(entity, grabbable.starting_sqt));
    if (dispatcher_system) {
      dispatcher_system->Send(
          entity, GrabCanceledEvent(entity, grabbable.starting_sqt));
    }
  } else if (type == kReleased) {
    // Need to calculate a valid position for the entity to end at.
    auto* mutator_system = registry_->Get<MutatorSystem>();
    const Sqt* original = transform_system->GetSqt(entity);
    if (original) {
      Sqt current(*original);
      mutator_system->ApplySqtMutator(entity, grabbable.group, &current, true);

      if (grabbable.snap_to_final) {
        transform_system->SetSqt(entity, current);
      }
      dispatcher->Send(GrabReleasedEvent(entity, current));
      if (dispatcher_system) {
        dispatcher_system->Send(entity, GrabReleasedEvent(entity, current));
      }
    }
  }

  grabbable.input->EndGrab(entity, grabbable.holding_device);
  grabbable.holding_device = InputManager::kMaxNumDeviceTypes;
  grabbed_.erase(entity);
}

}  // namespace lull
