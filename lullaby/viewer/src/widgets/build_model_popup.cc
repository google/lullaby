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

#include "lullaby/viewer/src/widgets/build_model_popup.h"

#include "dear_imgui/imgui.h"
#include "lullaby/util/filename.h"
#include "lullaby/viewer/src/builders/build_model.h"
#include "lullaby/viewer/src/widgets/file_dialog.h"

namespace lull {
namespace tool {

void BuildModelPopup::Open() {
  if (state_ == kClosed) {
    state_ = kEnable;
    open_ = true;
  }
}

void BuildModelPopup::Close() {
  if (open_) {
    ImGui::CloseCurrentPopup();
    open_ = false;
    state_ = kClosed;
  }
}

void BuildModelPopup::AdvanceFrame() {
  if (state_ == kEnable) {
    ImGui::OpenPopup("Build Model");
    state_ = kOpen;
  }
  if (ImGui::BeginPopupModal("Build Model", &open_)) {
    ImGui::Text("Filename: ");
    ImGui::SameLine();
    ImGui::Text("%s", basename_.c_str());
    ImGui::SameLine();
    if (ImGui::Button("...")) {
      filename_ = OpenFileDialog("Open File...", "FBX Files (*.fbx)");
      basename_ = GetBasenameFromFilename(filename_);
    }

    if (ImGui::Button("Compile")) {
      Close();
      BuildModel(registry_, filename_, "/tmp/");
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel")) {
      Close();
    }
    ImGui::EndPopup();
  }

  if (!open_) {
    state_ = kClosed;
  }
}

}  // namespace tool
}  // namespace lull
