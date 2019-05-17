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

#ifndef LULLABY_CONTRIB_GRAB_GRAB_SYSTEM_H_
#define LULLABY_CONTRIB_GRAB_GRAB_SYSTEM_H_

#include <set>
#include <unordered_map>

#include "lullaby/modules/ecs/component.h"
#include "lullaby/modules/input/input_manager.h"
#include "lullaby/modules/ecs/system.h"
#include "lullaby/events/grab_events.h"
#include "lullaby/util/math.h"

namespace lull {

/// The GrabSystem handles picking up items and moving them around.  The
/// blueprint for a grabbable item should include a GrabInput component of some
/// type (i.e. PlanarGrabInputDef).  It may also include some SqtMutators, for
/// modifying the result of that GrabInput component. (i.e. a
/// SqtMutatorStayInBoxDef set up to keep the dragged entity within a region)
/// See lullaby/examples/grab_simple.
class GrabSystem : public System {
 public:
  // Input handling systems should implement this interface.
  class GrabInputInterface {
   public:
    virtual ~GrabInputInterface() {}
    // Begin the grab process.  Handler should store any offsets and initial
    // state data in this function.  If the entity needs re-parenting, that
    // should be done here.  If the grab fails to start for any reason, this
    // should return false.  Otherwise, it should return true.
    virtual bool StartGrab(Entity entity, InputManager::DeviceType device) = 0;
    // Handler should return a Sqt that moves |entity| according to the current
    // state of |device|.
    virtual Sqt UpdateGrab(Entity entity, InputManager::DeviceType device,
                           const Sqt& original_sqt) = 0;
    // Called after mutations have been applied to the result of UpdateGrab().
    // If this returns true, the grab will be canceled.
    virtual bool ShouldCancel(Entity entity,
                              InputManager::DeviceType device) = 0;
    // End the grab process.  Handler should restore the Entity to a non-grabbed
    // state.
    virtual void EndGrab(Entity entity, InputManager::DeviceType device) = 0;
  };

  explicit GrabSystem(Registry* registry);

  void Create(Entity entity, HashValue type, const Def* def) override;
  void Destroy(Entity entity) override;
  void AdvanceFrame(const Clock::duration& delta_time);

  /// Manually make this entity start being dragged.  Only works on entities
  /// that have a GrabDef.
  void Grab(Entity entity, InputManager::DeviceType device);

  /// Manually make this entity stop being dragged.
  void Release(Entity entity);

  /// Cancel the drag, restoring the entity to its original state.
  void Cancel(Entity entity);

  /// Sets how the local transform is processed after input but before being
  /// actually set on the entity.
  void SetMutateGroup(Entity entity, HashValue group);

  /// Sets how input is mapped to a grabbed object's motion.
  void SetInputHandler(Entity entity, GrabInputInterface* handler);

  /// Call when an input handler is destroyed.
  void RemoveInputHandler(Entity entity, GrabInputInterface* handler);

 private:
  struct Grabbable {
    Sqt starting_sqt = Sqt();
    HashValue group = 0;
    GrabInputInterface* input = nullptr;
    bool snap_to_final = false;
    // If this is set to a valid device, the entity is actively being grabbed.
    InputManager::DeviceType holding_device = InputManager::kMaxNumDeviceTypes;
  };

  enum EndGrabType {
    // Grab had to be canceled for some reason.
    kCanceled,
    // Grab was intentionally released.
    kReleased,
    // Grabbed entity was destroyed.
    kDestroyed
  };

  void EndGrab(Entity entity, EndGrabType type);

  std::unordered_map<Entity, Grabbable> grabbables_;
  std::unordered_set<Entity> grabbed_;
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::GrabSystem);

#endif  // LULLABY_CONTRIB_GRAB_GRAB_SYSTEM_H_
