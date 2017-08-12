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

#include "lullaby/systems/text/flatui/font.h"

#include "lullaby/util/logging.h"

namespace lull {
namespace {

constexpr const char* kSystemFont = ".SystemFont";

}  // namespace

Font::Font(flatui::FontManager* manager,
           const std::vector<std::string>& font_names)
    : font_manager_(manager), font_name_list_() {
  // Try to load each of font_names, only adding ones that succeed.
  bool has_system_font = false;
  for (const auto& name : font_names) {
    has_system_font = (has_system_font || name.compare(kSystemFont) == 0);

    if (manager->Open(name)) {
      font_name_list_.emplace_back(name);
    } else {
      LOG(ERROR) << "Cannot not load font '" << name << "'!";
    }
  }

#ifdef __ANDROID__
  // Always fall back on the system font if possible, even if it wasn't
  // requested.
  if (!has_system_font) {
    if (manager->Open(kSystemFont)) {
      font_name_list_.emplace_back(kSystemFont);
    } else {
      LOG(ERROR) << "Cannot not load system font!";
    }
  }
#endif

  // Build list of char*s which point to font_name_list_.
  cstr_names_.reserve(font_name_list_.size());
  for (const auto& name : font_name_list_) {
    cstr_names_.emplace_back(name.c_str());
  }
}

bool Font::Bind() {
  return (!cstr_names_.empty() &&
          font_manager_->SelectFont(cstr_names_.data(),
                                    static_cast<int>(cstr_names_.size())));
}

}  // namespace lull
