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

#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>

#include "lullaby/modules/script/function_binder.h"
#include "lullaby/modules/lullscript/functions/functions.h"
#include "lullaby/modules/lullscript/script_env.h"
#include "lullaby/util/registry.h"
#include "readline/history.h"
#include "readline/readline.h"

// Holds the ScriptEnv and console-specific functions for the console repl.
class ScriptContext {
 public:
  ScriptContext() {
    registry_.Create<lull::FunctionBinder>(&registry_);
    RegisterFunctions();
    ResetScriptEnv();
  }

  bool ShouldQuit() const {
    return quit_;
  }

  // Evaluates the given source string using the ScriptEnv.
  std::string Evaluate(lull::string_view src) {
    const std::string result = lull::Stringify(env_->Exec(src));

    // Run any "side-effects" from the execution of a console-specific function.
    // We don't run these functions from within the script evaluation because
    // the functions may change the ScriptEnv itself.
    for (auto& fn : callbacks_) {
      fn();
    }
    callbacks_.clear();
    return result;
  }

 private:
  // Adds some console-specific functions to the function binder so that they
  // can be "executed" via the console.  All console functions are prefixed
  // with "!".
  void RegisterFunctions() {
    auto function_binder = registry_.Get<lull::FunctionBinder>();

    // !reset: Resets the environment (eg. clears out variables).
    function_binder->RegisterFunction("!reset", [this]() {
      callbacks_.emplace_back([this]() {
        ResetScriptEnv();
      });
    });

    // !quit: Signals the main loop to exit.
    function_binder->RegisterFunction("!quit", [this]() {
      callbacks_.emplace_back([this]() {
        quit_ = true;
      });
    });

    // !run: Loads the specified file and evaluates it.
    function_binder->RegisterFunction(
        "!run", [this](const std::string& filename) {
          callbacks_.emplace_back([this, filename]() {
            // Load the contents of the file to a string.
            std::ifstream fin(filename);
            std::string src((std::istreambuf_iterator<char>(fin)),
                            std::istreambuf_iterator<char>());
            fin.close();

            // Convert the loaded file into a script.
            const lull::Span<uint8_t> code(
                reinterpret_cast<const uint8_t*>(src.data()), src.size());
            auto script = env_->LoadOrRead(code);

            // Evaluate the script, printing its contents.
            const std::string result = lull::Stringify(env_->Eval(script));
            printf("> %s\n", result.c_str());
          });
        });
  }

  // Resets the ScriptEnv, effectively clearing out the "globals".
  void ResetScriptEnv() {
    auto function_binder = registry_.Get<lull::FunctionBinder>();
    env_.reset(new lull::ScriptEnv());
    env_->SetFunctionCallHandler([function_binder](lull::FunctionCall* call) {
      function_binder->Call(call);
    });
  }

  lull::Registry registry_;
  std::unique_ptr<lull::ScriptEnv> env_;
  std::vector<std::function<void()>> callbacks_;
  bool quit_ = false;
};

// Main read-eval-print loop.
int main() {
  ScriptContext script;
  while (!script.ShouldQuit()) {
    // Read input from the console.
    char* input = readline("$ ");
    if (input == nullptr) {
      break;
    }

    if (strlen(input)) {
      add_history(input);

      // Evaluate the code.
      const std::string result = script.Evaluate(input);

      // Print the result to the console.
      printf("> %s\n", result.c_str());
    }
    free(input);
  }
  return 0;
}
