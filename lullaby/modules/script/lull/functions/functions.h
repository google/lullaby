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

#ifndef LULLABY_MODULES_SCRIPT_LULL_FUNCTIONS_FUNCTIONS_H_
#define LULLABY_MODULES_SCRIPT_LULL_FUNCTIONS_FUNCTIONS_H_

#include <functional>
#include <sstream>
#include <string>

#include "lullaby/modules/function/call_native_function.h"
#include "lullaby/modules/script/lull/functions/converter.h"
#include "lullaby/modules/script/lull/script_frame.h"
#include "lullaby/util/string_view.h"

namespace lull {

class ScriptFrame;
class ScriptValue;

using ScriptFunction = std::function<void(ScriptFrame*)>;

// Converts the provided ScriptValue into a string for debugging purposes.
std::string Stringify(const ScriptValue& value);

// Converts the script snippet contained in the execution frame into a string
// for debugging purposes.
std::string Stringify(ScriptFrame* frame);

// Linked-list class used to simplify registering "built-in" functions with a
// script env.  Static instances of these classes are bound to C++ functions
// forming a linked list that can then be processed during ScriptEnv creation.
struct ScriptFunctionEntry {
  ScriptFunctionEntry(ScriptFunction fn, string_view name);

  ScriptFunction fn = nullptr;
  string_view name;
  ScriptFunctionEntry* next = nullptr;
};

// Register a ScriptFunction.
#define LULLABY_SCRIPT_FUNCTION(f, n) \
  ScriptFunctionEntry ScriptFunction_##f(&f, n);

namespace detail {
// ScriptFunctionContext is passed to CallNativeFunction to handle getting args
// and returning results.
class ScriptFunctionContext {
 public:
  static constexpr int kMaxArgs = 16;

  explicit ScriptFunctionContext(ScriptFrame* frame)
      : frame_(frame), num_args_(0) {
    while (frame->HasNext()) {
      if (num_args_ < kMaxArgs) {
        args_[num_args_] = frame->EvalNext();
        ++num_args_;
      } else {
        frame->Error("Too many args");
        break;
      }
    }
  }

  template <typename T>
  bool ArgToCpp(const char* function_name, size_t arg_index, T* value) {
    if (ScriptConverter<T>::Convert(args_[arg_index], value)) {
      return true;
    } else {
      std::stringstream msg;
      msg << function_name << " expects the type of arg " << arg_index + 1
          << " to be " << ScriptConverter<T>::TypeName();
      frame_->Error(msg.str().c_str());
      return false;
    }
  }

  template <typename T>
  bool ReturnFromCpp(const char* function_name, const T& value) {
    frame_->Return(value);
    return true;
  }

  bool CheckNumArgs(const char* function_name, size_t expected_args) {
    if (num_args_ != expected_args) {
      std::stringstream msg;
      msg << function_name << " expects " << expected_args << " args, but got "
          << num_args_;
      frame_->Error(msg.str().c_str());
      return false;
    }
    return true;
  }

 private:
  ScriptFrame* frame_;
  size_t num_args_;
  ScriptValue args_[kMaxArgs];
};

// If the first argument of the function is a ScriptFrame*, this will bind the
// frame as the first arg, so that CallNativeFunction doesn't see that arg.
template <typename Fn>
struct MaybePassScriptFrame {
  static inline Fn Wrap(ScriptFrame* frame, Fn fn) { return fn; }
};

template <typename Ret, typename... Args>
struct MaybePassScriptFrame<Ret (*)(ScriptFrame*, Args...)> {
  static inline std::function<Ret(Args...)> Wrap(ScriptFrame* frame,
                                                 Ret (*fn)(ScriptFrame*,
                                                           Args...)) {
    return [fn, frame](Args... args) { return fn(frame, args...); };
  }
};

// Wraps arbitrary functions and presents them as ScriptFunctions.
template <typename Fn>
ScriptFunction WrapScriptFunction(Fn fn, string_view name) {
  return [fn, name](ScriptFrame* frame) {
    detail::ScriptFunctionContext context(frame);
    CallNativeFunction(&context, name.data(),
                       MaybePassScriptFrame<Fn>::Wrap(frame, fn));
  };
}

}  // namespace detail

// Register any kind of function.
#define LULLABY_SCRIPT_FUNCTION_WRAP(f, n) \
  ScriptFunctionEntry ScriptFunction_##f(detail::WrapScriptFunction(&f, n), n);

}  // namespace lull

#endif  // LULLABY_SCRIPT_LULL_FUNCTIONS_FUNCTIONS_H_
