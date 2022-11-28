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

#ifndef REDUX_ENGINES_SCRIPT_REDUX_SCRIPT_FRAME_CONTEXT_H_
#define REDUX_ENGINES_SCRIPT_REDUX_SCRIPT_FRAME_CONTEXT_H_

#include <utility>

#include "absl/status/status.h"
#include "redux/engines/script/redux/script_frame.h"
#include "redux/modules/dispatcher/message.h"

namespace redux {

// Wraps a ScriptFrame for use as a CallNativeFunction context. (See
// call_native_function.h)
//
// This class actually serves two similar, but distinct purposes. In order to
// register a function with the ScriptEngine, we need a mechanism to pull
// arguments out of a ScriptFrame in order to invoke the native function. To
// support this, the ScriptEngine requires the context to provide the GetArg
// and SetReturnValue functions.
//
// Similarly, to register functions with the ScriptEnv internally, we need a
// context that conforms to the CallNativeFunction requirements, ie. ArgFromCpp
// and ReturnFromCpp. In this case, we also provide some extra niceties for
// dealing with things like VarArray, VarTable, and Message types.
class ScriptFrameContext {
 public:
  explicit ScriptFrameContext(ScriptFrame* frame) : frame_(frame) {
    if (frame_) {
      while (frame_->HasNext()) {
        args_.emplace_back(frame_->EvalNext());
      }
    }
  }

  template <typename T>
  absl::StatusCode ArgFromCpp(size_t index, T* out) {
    if (index >= args_.size()) {
      return absl::StatusCode::kOutOfRange;
    }

    constexpr bool kIsVarPtr =
        std::is_same_v<T, Var*> || std::is_same_v<T, const Var*>;
    constexpr bool kIsVarArrayPtr =
        std::is_same_v<T, VarArray*> || std::is_same_v<T, const VarArray*>;
    constexpr bool kIsVarTablePtr =
        std::is_same_v<T, VarTable*> || std::is_same_v<T, const VarTable*>;
    constexpr bool kIsMessagePtr =
        std::is_same_v<T, Message*> || std::is_same_v<T, const Message*>;

    bool ok = false;
    if constexpr (kIsVarPtr) {
      *out = args_[index].Get<Var>();
      ok = true;
    } else if constexpr (kIsVarArrayPtr) {
      *out = args_[index].Get<VarArray>();
      ok = true;
    } else if constexpr (kIsVarTablePtr) {
      *out = args_[index].Get<VarTable>();
      ok = true;
    } else if constexpr (kIsMessagePtr) {
      *out = args_[index].Get<Message>();
      ok = true;
    } else {
      ok = args_[index].GetAs(out);
    }
    return ok ? absl::StatusCode::kOk : absl::StatusCode::kInvalidArgument;
  }

  template <typename T>
  absl::StatusCode ReturnFromCpp(T&& value) {
    if (frame_) {
      frame_->Return(std::forward<T>(value));
      return absl::StatusCode::kOk;
    } else {
      return absl::StatusCode::kFailedPrecondition;
    }
  }

  Var* GetArg(size_t index) {
    if (index >= args_.size()) {
      return nullptr;
    }
    return args_[index].Get<Var>();
  }

  void SetReturnValue(Var var) { frame_->Return(std::move(var)); }

 private:
  ScriptFrame* frame_ = nullptr;
  std::vector<ScriptValue> args_;
};
}  // namespace redux

#endif  // REDUX_ENGINES_SCRIPT_REDUX_SCRIPT_FRAME_CONTEXT_H_
