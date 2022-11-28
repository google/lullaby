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

#include "redux/modules/dispatcher/message.h"

namespace redux {

Message::Message(const Message& rhs)
    : obj_(rhs.obj_),
      table_(rhs.table_),
      type_(rhs.type_),
      pointer_(rhs.pointer_),
      handler_(rhs.handler_) {
  if (rhs.handler_) {
    rhs.handler_(kCopy, this, &rhs);
  } else {
    CHECK(pointer_.Empty());
    CHECK(obj_.has_value() == false);
  }
}

Message::Message(Message&& rhs)
    : obj_(std::move(rhs.obj_)),
      table_(std::move(rhs.table_)),
      type_(rhs.type_),
      pointer_(rhs.pointer_),
      handler_(rhs.handler_) {
  if (rhs.handler_) {
    rhs.handler_(kMove, this, &rhs);
  } else {
    CHECK(pointer_.Empty());
    CHECK(obj_.has_value() == false);
  }

  rhs.type_ = 0;
  rhs.pointer_.Reset();
  rhs.handler_ = nullptr;
}

Message& Message::operator=(const Message& rhs) {
  if (this != &rhs) {
    if (rhs.handler_) {
      rhs.handler_(kCopy, this, &rhs);
    } else {
      CHECK(rhs.pointer_.Empty());
      CHECK(rhs.obj_.has_value() == false);

      type_ = rhs.type_;
      pointer_ = TypedPtr();
      obj_.reset();
      table_ = rhs.table_;
      handler_ = rhs.handler_;
    }
  }
  return *this;
}

Message& Message::operator=(Message&& rhs) {
  if (this != &rhs) {
    if (rhs.handler_) {
      rhs.handler_(kMove, this, &rhs);
    } else {
      CHECK(rhs.pointer_.Empty());
      CHECK(rhs.obj_.has_value() == false);

      type_ = rhs.type_;
      pointer_ = TypedPtr();
      obj_.reset();
      table_ = std::move(rhs.table_);
      handler_ = rhs.handler_;

      rhs.type_ = 0;
      rhs.table_.Clear();
      rhs.handler_ = nullptr;
    }
  }
  return *this;
}
}  // namespace redux
