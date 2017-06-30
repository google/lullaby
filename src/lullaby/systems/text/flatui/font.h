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

#ifndef LULLABY_SYSTEMS_TEXT_FLATUI_FONT_H_
#define LULLABY_SYSTEMS_TEXT_FLATUI_FONT_H_

#include <string>
#include <vector>

#include "flatui/font_manager.h"
#include "lullaby/util/typeid.h"

namespace lull {

// Wraps a preferred list of font files to choose from when rendering glyphs.
class Font {
 public:
  Font(flatui::FontManager* manager,
       const std::vector<std::string>& font_names);

  flatui::FontManager* GetFontManager() const { return font_manager_; }

  bool IsEmpty() const { return font_name_list_.empty(); }

  // Makes this font active in the FontManager.  All text buffers created after
  // this call will use this font.
  bool Bind();

 private:
  // Owned by TextSystem, which is responsible for creating and destroy Fonts.
  flatui::FontManager* font_manager_;

  // Prioritized list of font names to use when rendering glyphs.
  std::vector<std::string> font_name_list_;

  // Flatui takes a vector of char*, not std::string.
  std::vector<const char*> cstr_names_;

  Font(const Font& rhs) = delete;
  Font& operator=(const Font& rhs) = delete;
};

using FontPtr = std::shared_ptr<Font>;

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::Font);

#endif  // LULLABY_SYSTEMS_TEXT_FLATUI_FONT_H_
