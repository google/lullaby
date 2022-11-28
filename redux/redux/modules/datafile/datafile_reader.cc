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

#include "redux/modules/datafile/datafile_reader.h"

#include <string>
#include <utility>

#include "redux/modules/base/logging.h"

namespace redux::detail {

void DatafileReader::Key(std::string_view value) { key_ = Hash(value); }

void DatafileReader::BeginObject() {
  if (stack_.empty()) {
    using std::swap;
    swap(root_, stack_.emplace_back());
  } else {
    auto next = stack_.back()->Begin(key_);
    CHECK(next != nullptr);
    stack_.emplace_back(std::move(next));
  }
}

void DatafileReader::EndObject() {
  CHECK(!stack_.empty());
  stack_.pop_back();
}

void DatafileReader::BeginArray() {
  CHECK(!stack_.empty());
  auto next = stack_.back()->Begin(key_);
  CHECK(next != nullptr);
  stack_.emplace_back(std::move(next));
}

void DatafileReader::EndArray() {
  CHECK(!stack_.empty());
  stack_.pop_back();
}

void DatafileReader::Null() { SetValue(Var()); }

void DatafileReader::Boolean(bool value) { SetValue(Var(value)); }

void DatafileReader::Number(double value) { SetValue(Var(value)); }

void DatafileReader::String(std::string_view value) {
  SetValue(Var(std::string(value)));
}

void DatafileReader::Expression(std::string_view value) {
  CHECK(env_) << "Must provide ScriptEnv if datafile contains expressions.";
  if (env_) {
    ScriptValue script = env_->Read(value);
    ScriptValue result = env_->Eval(script);
    const Var* var = result.Get<Var>();
    CHECK(var != nullptr);
    SetValue(*var);
  }
}

void DatafileReader::ParseError(std::string_view context,
                                std::string_view message) {
  CHECK(false) << message;
}

void DatafileReader::SetValue(const Var& var) {
  CHECK(!stack_.empty());
  stack_.back()->SetValue(key_, var);
}

}  // namespace redux::detail
