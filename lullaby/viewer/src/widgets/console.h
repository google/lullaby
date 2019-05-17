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

#ifndef LULLABY_VIEWER_SRC_WIDGETS_CONSOLE_H_
#define LULLABY_VIEWER_SRC_WIDGETS_CONSOLE_H_

#include <string>
#include <vector>

#include "dear_imgui/imgui.h"
#include "lullaby/modules/lullscript/script_env.h"
#include "lullaby/util/registry.h"
#include "lullaby/util/string_view.h"

namespace lull {
namespace tool {

class Console {
 public:
  explicit Console(Registry* registry);

  void Open();
  void Close();
  void AdvanceFrame();

 private:
  static int TextEditCallback(ImGuiTextEditCallbackData* data);

  void Execute(string_view command);
  int OnEditInput(ImGuiTextEditCallbackData* data);

  void AddLog(string_view message);
  void ClearLog();

  Registry* registry_ = nullptr;
  bool open_ = true;

  ScriptEnv env_;
  char input_buffer_[256];
  std::vector<std::string> log_;
  std::vector<std::string> history_;
  bool scroll_to_bottom_ = false;
  int history_index_ = -1;
};

}  // namespace tool
}  // namespace lull

LULLABY_SETUP_TYPEID(lull::tool::Console);

#endif  // LULLABY_VIEWER_SRC_WIDGETS_CONSOLE_H_
