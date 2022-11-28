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

#include "linebreak.h"
#include "redux/engines/text/text_engine.h"

namespace redux {

static_assert(sizeof(TextCharacterBreakType) == sizeof(char));
static_assert(static_cast<int>(TextCharacterBreakType::kMustBreakNewLine) ==
              LINEBREAK_MUSTBREAK);
static_assert(static_cast<int>(TextCharacterBreakType::kCanBreakBetweenWords) ==
              LINEBREAK_ALLOWBREAK);
static_assert(static_cast<int>(TextCharacterBreakType::kNoBreakInGlyph) ==
              LINEBREAK_NOBREAK);
static_assert(static_cast<int>(TextCharacterBreakType::kNoBreakInCodepoint) ==
              LINEBREAK_INSIDEACHAR);

std::vector<TextCharacterBreakType> GetBreaks(std::string_view text,
                                              const TextParams& params) {
  static bool initialized = false;
  if (!initialized) {
    init_linebreak();
    initialized = true;
  }

  const char* language = "en-US";
  if (!params.language_iso_639.empty()) {
    language = params.language_iso_639.data();
  }

  std::vector<TextCharacterBreakType> breaks(text.length());
  set_linebreaks_utf8(reinterpret_cast<const utf8_t*>(text.data()),
                      text.length(), language,
                      reinterpret_cast<char*>(breaks.data()));
  return breaks;
}
}  // namespace redux
