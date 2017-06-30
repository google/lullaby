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

#ifndef LULLABY_EVENTS_LIFETIME_EVENTS_H_
#define LULLABY_EVENTS_LIFETIME_EVENTS_H_

#include "lullaby/util/typeid.h"

namespace lull {

// This is dispatched immediately when the OnPause call happens (may not be on
// the main thread).  Any listeners will need to be coded to be thread safe.
struct OnPauseThreadUnsafeEvent {
  template <typename Archive>
  void Serialize(Archive archive) {}
};

// This is dispatched immediately when the OnResume call happens (may not be on
// the main thread).  Any listeners will need to be coded to be thread safe.
struct OnResumeThreadUnsafeEvent {
  template <typename Archive>
  void Serialize(Archive archive) {}
};

// This is added to the dispatcher in the normal way, and will be processed the
// frame after the resume occurs.
struct OnResumeEvent {
  template <typename Archive>
  void Serialize(Archive archive) {}
};

// This is dispatched the frame after the app begins to quit.  It is triggered
// by code requesting a quit, and may not run if the quit is triggered by the
// OS.
struct OnQuitRequestEvent {
  template <typename Archive>
  void Serialize(Archive archive) {}
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::OnPauseThreadUnsafeEvent);
LULLABY_SETUP_TYPEID(lull::OnResumeThreadUnsafeEvent);
LULLABY_SETUP_TYPEID(lull::OnResumeEvent);
LULLABY_SETUP_TYPEID(lull::OnQuitRequestEvent);

#endif  // LULLABY_EVENTS_LIFETIME_EVENTS_H_
