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

#ifndef LULLABY_MODULES_DISPATCHER_EVENT_WRAPPER_H_
#define LULLABY_MODULES_DISPATCHER_EVENT_WRAPPER_H_

#include <cstddef>
#include <unordered_map>

#include "lullaby/modules/serialize/serialize.h"
#include "lullaby/modules/serialize/serialize_traits.h"
#include "lullaby/modules/serialize/variant_serializer.h"
#include "lullaby/util/aligned_alloc.h"
#include "lullaby/util/string_view.h"
#include "lullaby/util/typeid.h"
#include "lullaby/util/variant.h"

// If the editor is active, we need to track event names.
#ifndef LULLABY_TRACK_EVENT_NAMES
#if LULLABY_ENABLE_EDITOR
#define LULLABY_TRACK_EVENT_NAMES 1
#else
#define LULLABY_TRACK_EVENT_NAMES 0
#endif
#endif

namespace lull {

/// Wraps Events of different types in a consistent way to allow Dispatchers to
/// handle and store them in a generic way.
///
/// An Event is just an identifier and some associated data.  There are two
/// forms in which Events are instantiated:
///
/// * Concrete Events are instances of C++ data structures.  For example:
///     struct ClickEvent { int value; };
///     ClickEvent click{123};
///
///   In this case, |click| is a type of Event that represents a ClickEvent with
///   a value of 123.
///
/// * Runtime Events are dynamically generated data containers.  For example,
///   the |click| example above can also be represented like so:
///     const TypeId event_type = Hash("ClickEvent");
///     VariantMap event_data;
///     event_data["value"] = 123;
///     auto click = std::make_pair(event_type, event_data);
///
/// The EventWrapper is used to "wrap" either type of Event in a single
/// container so that it can be used by the Dispatcher.  The EventWrapper can
/// also be used to convert between the Concrete and Runtime instances,
/// allowing senders and receivers to use the form of their choice.  Conversion
/// between Concrete and Runtime events is non-trivial and, as such, it is
/// recommended that conversion only be done when necessary.  The results of the
/// conversion are cached by the EventWrapper so multiple requests are not
/// expensive.
///
/// Concrete events may or may not be owned by the EventWrapper.  When a
/// concrete Event is first wrapped, only a pointer to the instance is stored by
/// the EventWrapper.  Since the EventWrapper does not own the concrete Event,
/// the lifetime of the EventWrapper must be less than the lifetime of the
/// wrapped Event.  However, copying the EventWrapper results in the Concrete
/// event being copied and, furthermore, the copied Event is now owned by the
/// copied EventWrapper.
class EventWrapper {
 public:
  /// Default constructor.
  EventWrapper() {}

  /// Creates an EventWrapper that wraps the concrete |event|.  The EventWrapper
  /// stores a pointer to the |event| and, as such, should not outlive |event|.
  template <typename Event>
  explicit EventWrapper(const Event& event);

  /// Creates an EventWrapper for a runtime Event representing |type|.
  explicit EventWrapper(TypeId type, string_view name = "");

  /// Clones the Event wrapped in |rhs| such that |this| now owns a copy of the
  /// Event.
  EventWrapper(const EventWrapper& rhs);

  /// Takes ownership of the Event in |rhs|.
  EventWrapper(EventWrapper&& rhs);

  /// Destroys all owned Event data.
  ~EventWrapper();

  /// Clones the Event wrapped in |rhs| such that |this| now owns a copy of the
  /// Event.
  EventWrapper& operator=(const EventWrapper& rhs);

  /// Swaps ownership of the Events in |this| and |rhs|.
  EventWrapper& operator=(EventWrapper&& rhs);

  /// Associates |value| with the |key| for a Runtime Event.  Internally, the
  /// |value| is stored as a Variant.
  template <typename T>
  void SetValue(HashValue key, const T& value);

  /// Sets the Runtime Event values directly from a VariantMap.
  void SetValues(const VariantMap& values);

  /// Gets the TypeId of the wrapped Event.
  TypeId GetTypeId() const { return type_; }

  /// Returns true if the event wrapper already is already represented as a
  /// variant map. If true, calls to GetValue will not incur any additional data
  /// transformation cost.
  bool IsRuntimeEvent() const { return data_ != nullptr; }

  /// Returns true if the event supports serialization.  This is usually only
  /// false if an event is a struct that doesn't define a serialize function.
  bool IsSerializable() const { return serializable_; }

#if LULLABY_TRACK_EVENT_NAMES
  /// Returns the name of the event - either the class name or the string that
  /// was hashed to create the event.  By default this is not available in
  /// production environments.
  const std::string& GetName() const { return name_; }
#endif

  /// Gets the pointer to the wrapped Event if it is of the type specified by
  /// |Event|, otherwise returns nullptr.  If the EventWrapper currently only
  /// wraps a Runtime Event, then this function will attempt to generate a
  /// Concrete Event containing the same data.
  template <typename Event>
  const Event* Get() const;

  /// Gets the value associated with |key| in the wrapped Event.  If the
  /// EventWrapper currently only wraps a Concrete Event, then this function
  /// will attempt to generate a Runtime Event containing the same data.
  /// Returns nullptr if no value is associated with |key|.
  template <typename T>
  const T* GetValue(HashValue key) const;

  /// Similar to GetValue, but returns a const reference to the value associated
  /// with |key|.  If there is no association, returns the provided
  /// |default_value| instead.
  template <typename T>
  const T& GetValueWithDefault(HashValue key, const T& default_value) const;

  /// Gets the underlying VariantMap that stores the values for a Runtime Event.
  const VariantMap* GetValues() const;

 private:
  enum Operation {
    kCopy,
    kDestroy,
    kSaveToVariant,
    kLoadFromVariant,
  };

  enum OwnershipFlag {
    kDoNotOwn,
    kTakeOwnership,
  };

  using HandlerFn = void (*)(Operation, void*, const void*);

  /// Performs the specified Operation on the provided pointers.  Specifically,
  /// if |op| is:
  ///   kCopy: copies an |Event| from |dst| to |src| using the Event copy
  ///     constructor.
  ///   kDestroy: destroys the |Event| in |dst| using the Event destructor.
  ///   kSaveToVariant: serializes the |Event| in |src| to the VariantMap |dst|.
  ///   kLoadFromVariant: serializes the VariantMap |src| to the |Event| in
  ///     |dst|.
  template <typename Event>
  static void Handler(Operation op, void* dst, const void* src);

  /// Ensures the EventWrapper has a Concrete Event which may require converting
  /// a Runtime Event into a Concrete Event.
  template <typename Event>
  void EnsureConcreteEventAvailable() const;

  /// Ensures the EventWrapper has a Runtime Event which may require converting
  /// a Concrete Event into a Runtime Event.
  void EnsureRuntimeEventAvailable() const;

  /// The TypeId of the wrapped event.
  TypeId type_ = 0;

  /// The sizeof() the wrapped concrete event.
  mutable size_t size_ = 0;

  /// The alignof() the wrapped concrete event.
  mutable size_t align_ = 0;

  /// Pointer to the wrapped concrete event.
  mutable void* ptr_ = nullptr;

  /// Data associated with the wrapped runtime event.
  mutable std::unique_ptr<VariantMap> data_ = nullptr;

  /// Function that performs the specified Operation on the wrapped event.
  mutable HandlerFn handler_ = nullptr;

  /// Determines whether or not |ptr_| should be deleted when the EventWrapper
  /// is destroyed.
  mutable OwnershipFlag owned_ = kDoNotOwn;

  /// Tracks whether the event can safely be serialized.
  bool serializable_ = false;

#if LULLABY_TRACK_EVENT_NAMES
  /// Store the string that was hashed to get the type_.  Used for debugging.
  std::string name_ = "";
#endif
};

template <typename Event>
EventWrapper::EventWrapper(const Event& event)
    : type_(lull::GetTypeId<Event>()),
      size_(sizeof(Event)),
      align_(alignof(Event)),
      ptr_(const_cast<Event*>(&event)),
      data_(nullptr),
      handler_(&Handler<Event>),
      owned_(kDoNotOwn),
      serializable_(detail::IsSerializable<Event, SaveToVariant>::kValue) {
  assert(ptr_ != nullptr);
#if LULLABY_TRACK_EVENT_NAMES
  name_ = GetTypeName<Event>();
#endif
}

template <typename Event>
const Event* EventWrapper::Get() const {
  if (type_ != lull::GetTypeId<Event>()) {
    return nullptr;
  }

  EnsureConcreteEventAvailable<Event>();
  return reinterpret_cast<const Event*>(ptr_);
}

template <typename T>
void EventWrapper::SetValue(HashValue key, const T& value) {
  if (ptr_) {
    LOG(ERROR) << "Cannot set value on a concrete event.";
    return;
  }

  if (data_) {
    data_->emplace(key, Variant(value));
  }
}

template <typename T>
const T* EventWrapper::GetValue(HashValue key) const {
  EnsureRuntimeEventAvailable();
  auto iter = data_->find(key);
  if (iter != data_->end()) {
    return iter->second.Get<T>();
  }
  return nullptr;
}

template <typename T>
const T& EventWrapper::GetValueWithDefault(HashValue key,
                                           const T& default_value) const {
  const T* ptr = GetValue<T>(key);
  return ptr ? *ptr : default_value;
}

template <typename Event>
void EventWrapper::Handler(Operation op, void* dst, const void* src) {
  switch (op) {
    case kCopy: {
      const Event* other = reinterpret_cast<const Event*>(src);
      new (dst) Event(*other);
      break;
    }
    case kDestroy: {
      Event* event = reinterpret_cast<Event*>(dst);
      event->~Event();
      // On certain toolchains, ~Event() is a no-op and thus |event| is
      // considered an unused variable.  The following will prevent the compiler
      // from warning about an unused variable.
      (void)event;
      break;
    }
    case kSaveToVariant: {
      VariantMap* map = reinterpret_cast<VariantMap*>(dst);
      const Event* event = reinterpret_cast<const Event*>(src);
      SaveToVariant serializer(map);
      Serialize(&serializer, const_cast<Event*>(event), 0);
      break;
    }
    case kLoadFromVariant: {
      const VariantMap* map = reinterpret_cast<const VariantMap*>(src);
      Event* event = reinterpret_cast<Event*>(dst);
      LoadFromVariant serializer(const_cast<VariantMap*>(map));
      Serialize(&serializer, event, 0);
      break;
    }
  }
}

template <typename Event>
void EventWrapper::EnsureConcreteEventAvailable() const {
  if (ptr_) {
    return;
  }

  size_ = sizeof(Event);
  align_ = alignof(Event);
  ptr_ = AlignedAlloc(size_, align_);
  owned_ = kTakeOwnership;
  handler_ = &Handler<Event>;

  new (ptr_) Event();
  handler_(kLoadFromVariant, ptr_, data_.get());
}

inline void EventWrapper::EnsureRuntimeEventAvailable() const {
  if (data_) {
    return;
  }

  data_.reset(new VariantMap());
  handler_(kSaveToVariant, data_.get(), ptr_);
}

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::EventWrapper);

#endif  // LULLABY_MODULES_DISPATCHER_EVENT_WRAPPER_H_
