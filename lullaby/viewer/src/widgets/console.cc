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

#include "lullaby/viewer/src/widgets/console.h"

#include "dear_imgui/imgui.h"
#include "lullaby/modules/lullscript/functions/functions.h"
#include "lullaby/modules/script/function_binder.h"
#include "lullaby/util/entity.h"

namespace lull {
namespace tool {

Console::Console(Registry* registry) : registry_(registry) {
  auto* binder = registry->Get<FunctionBinder>();
  binder->RegisterFunction("Entity", [](int x) { return Entity(x); });
}

void Console::Open() {
  open_ = true;
  input_buffer_[0] = 0;
}

void Console::Close() { open_ = false; }

void Console::AddLog(string_view command) {
  log_.emplace_back(command);
  scroll_to_bottom_ = true;
}

void Console::ClearLog() {
  log_.clear();
}

void Console::AdvanceFrame() {
  if (ImGui::Begin("Console")) {
    ImGui::BeginChild("ScrollingRegion",
                      ImVec2(0, -ImGui::GetItemsLineHeightWithSpacing()), false,
                      ImGuiWindowFlags_HorizontalScrollbar);

    // Tighten spacing
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1));
    for (const std::string& item : log_) {
      ImVec4 col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
      if (item[0] == '>') {
        col = ImColor(0.6f, 0.6f, 0.6f, 1.0f);
      }
      ImGui::PushStyleColor(ImGuiCol_Text, col);
      ImGui::TextUnformatted(item.c_str());
      ImGui::PopStyleColor();
    }

    if (scroll_to_bottom_) {
      ImGui::SetScrollHere();
      scroll_to_bottom_ = false;
    }
    ImGui::PopStyleVar();
    ImGui::EndChild();

    ImGui::Separator();

    // Execute the command in the input box.
    if (ImGui::InputText("##Input", input_buffer_, sizeof(input_buffer_),
                         ImGuiInputTextFlags_EnterReturnsTrue |
                             ImGuiInputTextFlags_CallbackCompletion |
                             ImGuiInputTextFlags_CallbackHistory,
                         &TextEditCallback, this)) {
      Execute(input_buffer_);
      input_buffer_[0] = 0;
    }

    ImGui::SameLine();
    if (ImGui::SmallButton("Clear")) {
      ClearLog();
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("Toggle Pause")) {
      env_.Exec("(pause)");
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("Single Step")) {
      env_.Exec("(step)");
    }
    ImGui::SameLine();
    if (ImGui::SmallButton(".")) {
      // do nothing
    }
    // if (ImGui::SmallButton("Scroll to bottom")) {
    //   scroll_to_bottom_ = true;
    // }

    // Keep focus on the input box.
    if (ImGui::IsItemHovered() ||
        (ImGui::IsRootWindowOrAnyChildFocused() && !ImGui::IsAnyItemActive() &&
         !ImGui::IsMouseClicked(0))) {
      ImGui::SetKeyboardFocusHere(-1);
    }
  }
  ImGui::End();
}

int Console::TextEditCallback(ImGuiTextEditCallbackData* data) {
  Console* console = reinterpret_cast<Console*>(data->UserData);
  return console->OnEditInput(data);
}

int Console::OnEditInput(ImGuiTextEditCallbackData* data) {
  switch (data->EventFlag) {
    case ImGuiInputTextFlags_CallbackHistory: {
      int index = history_index_;
      if (index == -1) {
        index = static_cast<int>(history_.size());
      }

      if (data->EventKey == ImGuiKey_UpArrow) {
        --index;
      } else if (data->EventKey == ImGuiKey_DownArrow) {
        ++index;
      }
      if (index < 0) {
        index = 0;
      }

      if (index < history_.size()) {
        history_index_ = index;
        const string_view text = history_[index];
        const int len =
            snprintf(data->Buf, (size_t)data->BufSize, "%s", text.data());
        data->CursorPos = len;
        data->SelectionStart = len;
        data->SelectionEnd = len;
        data->BufTextLen = len;
        data->BufDirty = true;
      }
      break;
    }
  }
  return 0;
}

void Console::Execute(string_view command) {
  AddLog(command);
  if (history_.empty() || history_.back() != command) {
    history_.emplace_back(command);
  }
  history_index_ = -1;

  ScriptValue result = env_.Exec(command);
  const std::string text = std::string("> ") + Stringify(result);
  AddLog(text);
}

}  // namespace tool
}  // namespace lull
