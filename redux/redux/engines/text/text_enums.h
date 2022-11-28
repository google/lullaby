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

#ifndef REDUX_ENGINES_TEXT_TEXT_TYPES_H_
#define REDUX_ENGINES_TEXT_TEXT_TYPES_H_

#include "redux/modules/base/typeid.h"

namespace redux {

enum class HorizontalTextAlignment {
  kLeft,
  kCenter,
  kRight,
};

enum class VerticalTextAlignment {
  kTop,
  kCenter,
  kBaseline,
  kBottom,
};

enum class TextWrapMode {
  // Do not wrap lines.
  kNone,
  // Wraps text, preferring to break lines between words. If a single word is
  // too wide, it will be broken without being hyphenated to remain in bounds.
  kBetweenWords,
};

// Controls the reading direction of text.
enum class TextDirection {
  kLanguageDefault,
  kLeftToRight,
  kRightToLeft,
};

// Until there is an alternate breaker implementation, these should match the
// type size and values returned by libunibreak.
enum class TextCharacterBreakType : char {
  kMustBreakNewLine = 0,
  kCanBreakBetweenWords = 1,
  kNoBreakInGlyph = 2,
  kNoBreakInCodepoint = 3,
};
}  // namespace redux

REDUX_SETUP_TYPEID(redux::HorizontalTextAlignment);
REDUX_SETUP_TYPEID(redux::VerticalTextAlignment);
REDUX_SETUP_TYPEID(redux::TextWrapMode);
REDUX_SETUP_TYPEID(redux::TextDirection);

#endif  // REDUX_ENGINES_TEXT_TEXT_TYPES_H_
