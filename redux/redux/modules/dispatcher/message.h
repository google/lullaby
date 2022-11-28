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

#ifndef REDUX_MODULES_DISPATCHER_MESSAGE_H_
#define REDUX_MODULES_DISPATCHER_MESSAGE_H_

#include "absl/types/any.h"
#include "redux/modules/base/logging.h"
#include "redux/modules/base/serialize.h"
#include "redux/modules/base/typed_ptr.h"
#include "redux/modules/base/typeid.h"
#include "redux/modules/var/var_serializer.h"
#include "redux/modules/var/var_table.h"

namespace redux {

// A payload of data associated with a type that is broadcast by dispatchers.
//
// All Messages have a type identifier (e.g. OnClickEvent) and a payload of data
// to go along with it. These messages can be "sent" to the various handlers
// registered to a Dispatcher.
//
// The payload of a Message can exist in three forms:
//
// 1) A pointer to a C++ object instance.
// 2) A copy of a C++ object for when the lifetime of a Message exceeds the
//    lifetime of the referenced object as per the above use-case.
// 3) A "dynamic" message that allows for reading/writing of arbitrary key/value
//    pairs of data. This is useful, for example, for sending/receiving messages
//    in scripting.
//
// The Message class will automatically convert between the above forms
// depending on the use-case. This conversion is non-trivial and should be
// avoided if possible.
//
// This class is not thread safe regardless of any const-ness.
class Message {
 public:
  Message() = default;

  // Creates a Message that stores a pointer to the `obj`.
  template <typename T>
  explicit Message(const T& obj);

  // Creates a dynamic Message.
  explicit Message(TypeId type, VarTable values = {});

  Message(const Message& rhs);
  Message(Message&& rhs);

  Message& operator=(const Message& rhs);
  Message& operator=(Message&& rhs);

  ~Message() = default;

  // Associates `value` with the `key` for dynamic Messages.
  template <typename T>
  void SetValue(HashValue key, T&& value);

  // Returns the Message's type.
  TypeId GetTypeId() const { return type_; }

  // Returns a pointer of type `T` if it matches the Message's type.
  template <typename T>
  const T* Get() const;

  // Returns the value associated with the `key`, or `default_value` if no such
  // association exists.
  template <typename T>
  T ValueOr(HashValue key, T&& default_value) const;

 private:
  // The different operations that may need to be performed on the stored ddata.
  enum HandlerOp {
    kCopy,     // Copy the native object to another message.
    kMove,     // Move the native object to another message.
    kToVar,    // Convert the native object into a Var.
    kFromVar,  // Convert the Var into a native object.
  };

  // The function that performs the above operations.
  using HandlerFn = void(HandlerOp, Message*, const Message*);

  template <typename T>
  static void Handler(HandlerOp op, Message* dst, const Message* src);

  // Ensures that the payload is accessible as a native object view the pointer_
  // member, performing any necessary conversions.
  template <typename T>
  void EnsureIsConcrete() const;

  // Ensures that the payload is accessible as data in the table_ member,
  // performing any necessary conversions.
  void EnsureIsDynamic() const;

  absl::any obj_;     // Type-erased instance of the payload.
  Var table_;         // The payload stored as a dynamic Var.
  TypeId type_ = 0;   // The typeid of the message.
  TypedPtr pointer_;  // A pointer to an payload. This coule be externally
                      // owned or a pointer to the any obj_ member.

  mutable HandlerFn* handler_ = nullptr;
};

inline Message::Message(TypeId type, VarTable values)
    : table_(std::move(values)), type_(type) {}

template <typename T>
Message::Message(const T& obj)
    : type_(redux::GetTypeId<T>()),
      pointer_(const_cast<T*>(&obj)),
      handler_(&Handler<T>) {}

template <typename T>
const T* Message::Get() const {
  if constexpr (std::is_same_v<T, VarTable>) {
    EnsureIsDynamic();
    return table_.Get<VarTable>();
  } else {
    if (type_ != redux::GetTypeId<T>()) {
      return nullptr;
    }
    EnsureIsConcrete<T>();
    return pointer_.Get<T>();
  }
}

template <typename T>
void Message::SetValue(HashValue key, T&& value) {
  CHECK(pointer_.Empty()) << "Cannot set values if message has been converted.";
  EnsureIsDynamic();
  table_[key] = std::forward<T>(value);
}

template <typename T>
T Message::ValueOr(HashValue key, T&& default_value) const {
  EnsureIsDynamic();
  return table_[key].ValueOr(std::forward<T>(default_value));
}

template <typename T>
void Message::Handler(HandlerOp op, Message* dst, const Message* src) {
  CHECK(src->type_ == redux::GetTypeId<T>());
  CHECK(dst->type_ == redux::GetTypeId<T>());
  switch (op) {
    case kCopy: {
      CHECK_NE(src, dst) << "Cannot clone to self.";
      if (src->pointer_.Is<T>()) {
        dst->obj_ = *src->pointer_.Get<T>();
        dst->pointer_ = TypedPtr(absl::any_cast<T>(&dst->obj_));
      }
      CHECK(dst->pointer_.Is<T>());
      break;
    }
    case kMove: {
      CHECK_NE(src, dst) << "Cannot clone to self.";
      if (src->pointer_.Is<T>()) {
        if (src->obj_.has_value()) {
          dst->obj_ = std::move(src->obj_);
        } else {
          dst->obj_ = *src->pointer_.Get<T>();
        }
        dst->pointer_ = TypedPtr(absl::any_cast<T>(&dst->obj_));
      }
      CHECK(dst->pointer_.Is<T>());
      break;
    }
    case kFromVar: {
      if (src->table_.Is<VarTable>() && dst->pointer_.Empty()) {
        T obj;
        LoadFromVar loader(const_cast<Var*>(&src->table_));
        Serialize(loader, obj);

        dst->obj_ = obj;
        dst->pointer_ = TypedPtr(absl::any_cast<T>(&dst->obj_));
      }
      CHECK(dst->pointer_.Is<T>());
      break;
    }
    case kToVar: {
      if (src->pointer_.Is<T>() && dst->table_.Empty()) {
        const T* ptr = src->pointer_.Get<T>();

        dst->table_ = VarTable();
        SaveToVar saver(&dst->table_);
        Serialize(saver, *const_cast<T*>(ptr));
      }
      CHECK(dst->table_.Is<VarTable>());
      break;
    }
  }
}

template <typename T>
void Message::EnsureIsConcrete() const {
  CHECK_EQ(type_, redux::GetTypeId<T>());
  if (pointer_.Is<T>()) {
    return;
  }
  handler_ = &Handler<T>;
  handler_(kFromVar, const_cast<Message*>(this), this);
}

inline void Message::EnsureIsDynamic() const {
  if (table_.Is<VarTable>()) {
    return;
  }
  if (handler_) {
    handler_(kToVar, const_cast<Message*>(this), this);
  }
}

}  // namespace redux

REDUX_SETUP_TYPEID(redux::Message);

#endif  // REDUX_MODULES_DISPATCHER_MESSAGE_H_
