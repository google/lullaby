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

#include "lullaby/modules/dispatcher/event_wrapper.h"
#include "lullaby/modules/script/lull/functions/functions.h"
#include "lullaby/modules/script/lull/script_env.h"
#include "lullaby/modules/script/lull/script_frame.h"
#include "lullaby/modules/script/lull/script_types.h"

// This file implements the following script functions:
//
// (event [type] [(map ...)] )
//   Creates an event wrapper with the optional map of values.  The type
//   must have integer or hashvalue type.
//
// (event-type [event])
//   Returns the name of the event.
//
// (event-size [event])
//   Returns the number of elements in the event.
//
// (event-empty [event])
//   Returns true if the event is empty (ie. contains no elements).
//
// (event-get [event] [key])
//   Returns the value associated with the key in the event.  The key must be an
//   integer or hashvalue type.

namespace lull {
namespace {

Optional<HashValue> GetKey(ScriptArgList* args) {
  Optional<HashValue> result;
  ScriptValue key_value = args->EvalNext();
  if (key_value.Is<HashValue>()) {
    result = *key_value.Get<HashValue>();
  } else if (key_value.Is<int>()) {
    result = static_cast<HashValue>(*key_value.Get<int>());
  }
  return result;
}

void EventCreate(ScriptFrame* frame) {
  Optional<EventWrapper> event;
  if (frame->HasNext()) {
    Optional<HashValue> type;
    ScriptValue type_arg = frame->EvalNext();
    if (type_arg.Is<HashValue>()) {
      type = *type_arg.Get<HashValue>();
    } else if (type_arg.Is<int>()) {
      type = static_cast<HashValue>(*type_arg.Get<int>());
    } else {
      frame->Error("event: type argument must be a hash or int");
      return;
    }
    event = EventWrapper(*type, "from-script");
  }
  if (!event) {
    frame->Error("event: expected event type as first argument");
    return;
  }

  if (frame->HasNext()) {
    ScriptValue map_arg = frame->EvalNext();
    const VariantMap* map = map_arg.Get<VariantMap>();
    if (map) {
      event->SetValues(*map);
    }
  }
  frame->Return(*event);
}

void EventType(ScriptFrame* frame) {
  const ScriptValue event_value = frame->EvalNext();
  const auto* event = event_value.Get<EventWrapper>();
  if (event) {
    frame->Return(event->GetTypeId());
  } else {
    frame->Error("event-type: expected event as first argument");
  }
}

void EventSize(ScriptFrame* frame) {
  const ScriptValue event_value = frame->EvalNext();
  const auto* event = event_value.Get<EventWrapper>();
  if (event) {
    const auto* values = event->GetValues();
    if (values) {
      frame->Return(static_cast<int>(values->size()));
    } else {
      frame->Return(0);
    }
  } else {
    frame->Error("event-size: expected event as first argument");
  }
}

void EventEmpty(ScriptFrame* frame) {
  const ScriptValue event_value = frame->EvalNext();
  const auto* event = event_value.Get<EventWrapper>();
  if (event) {
    const auto* values = event->GetValues();
    if (values) {
      frame->Return(values->empty());
    } else {
      frame->Return(true);
    }
  } else {
    frame->Error("event-empty: expected event as first argument");
  }
}

void EventGet(ScriptFrame* frame) {
  const ScriptValue event_value = frame->EvalNext();
  const auto* event = event_value.Get<EventWrapper>();
  if (!event) {
    frame->Error("event-get: expected event as first argument");
    return;
  }
  Optional<HashValue> key = GetKey(frame);
  if (!key) {
    frame->Error("event-get: expected key as second argument");
    return;
  }
  const auto* values = event->GetValues();
  if (!values) {
    frame->Error("event-get: no data in event");
    return;
  }
  auto iter = values->find(*key);
  if (iter != values->end()) {
    frame->Return(iter->second);
  } else {
    frame->Error("event-get: no element at given key");
  }
}

LULLABY_SCRIPT_FUNCTION(EventCreate, "event");
LULLABY_SCRIPT_FUNCTION(EventType, "event-type");
LULLABY_SCRIPT_FUNCTION(EventSize, "event-size");
LULLABY_SCRIPT_FUNCTION(EventEmpty, "event-empty");
LULLABY_SCRIPT_FUNCTION(EventGet, "event-get");

}  // namespace
}  // namespace lull
