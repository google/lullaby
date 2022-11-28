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

#include "redux/engines/platform/keycodes.h"

namespace redux {

struct ChordMaker {
  ChordMaker(const KeycodeBitset& keys, const KeyModifier& modifiers)
      : keys(keys), modifiers(modifiers) {}

  std::string Build() const;
  void TryAppend(std::string& str, KeyCode code, char c) const;
  void TryRangedAppend(std::string& str, KeyCode first, KeyCode last,
                       char c) const;

  const KeycodeBitset& keys;
  const KeyModifier& modifiers;
};

void ChordMaker::TryRangedAppend(std::string& str, KeyCode first, KeyCode last,
                                 char c) const {
  for (int i = first; i <= last; ++i) {
    if (keys[i]) {
      const char append_char = c + i - first;
      str.push_back(append_char);
    }
  }
}

void ChordMaker::TryAppend(std::string& str, KeyCode code, char c) const {
  if (keys[code]) {
    str.push_back(c);
  }
}

std::string ChordMaker::Build() const {
  std::string str;
  const bool capitalized =
      modifiers & (KEYMOD_LSHIFT | KEYMOD_RSHIFT | KEYMOD_CAPS);

  TryRangedAppend(str, KEYCODE_A, KEYCODE_Z, capitalized ? 'A' : 'a');
  TryRangedAppend(str, KEYCODE_0, KEYCODE_9, '0');
  TryRangedAppend(str, KEYCODE_KP_0, KEYCODE_KP_9, '0');
  TryAppend(str, KEYCODE_KP_PLUS, '+');
  TryAppend(str, KEYCODE_KP_MINUS, '-');
  TryAppend(str, KEYCODE_KP_MULTIPLY, '*');
  TryAppend(str, KEYCODE_KP_DIVIDE, '/');
  TryAppend(str, KEYCODE_KP_ENTER, '\n');
  TryAppend(str, KEYCODE_KP_PERIOD, '.');
  TryAppend(str, KEYCODE_KP_EQUALS, '=');
  TryAppend(str, KEYCODE_TAB, '\t');
  TryAppend(str, KEYCODE_RETURN, '\n');
  TryAppend(str, KEYCODE_SPACE, ' ');
  TryAppend(str, KEYCODE_COMMA, ',');
  TryAppend(str, KEYCODE_PERIOD, '.');
  TryAppend(str, KEYCODE_SLASH, '/');
  TryAppend(str, KEYCODE_BACKSLASH, '\\');
  TryAppend(str, KEYCODE_COLON, ':');
  TryAppend(str, KEYCODE_SEMICOLON, ';');
  TryAppend(str, KEYCODE_LEFT_BRACKET, '[');
  TryAppend(str, KEYCODE_RIGHT_BRACKET, ']');
  TryAppend(str, KEYCODE_LEFT_PAREN, '(');
  TryAppend(str, KEYCODE_RIGHT_PAREN, ')');
  TryAppend(str, KEYCODE_SINGLE_QUOTE, '\'');
  TryAppend(str, KEYCODE_DOUBLE_QUOTE, '"');
  TryAppend(str, KEYCODE_BACK_QUOTE, '`');
  TryAppend(str, KEYCODE_EXCLAMATION, '!');
  TryAppend(str, KEYCODE_AT, '@');
  TryAppend(str, KEYCODE_HASH, '#');
  TryAppend(str, KEYCODE_DOLLAR, '$');
  TryAppend(str, KEYCODE_PERCENT, '%');
  TryAppend(str, KEYCODE_CARET, '^');
  TryAppend(str, KEYCODE_AMPERSAND, '&');
  TryAppend(str, KEYCODE_ASTERISK, '*');
  TryAppend(str, KEYCODE_QUESTION, '?');
  TryAppend(str, KEYCODE_PLUS, '+');
  TryAppend(str, KEYCODE_MINUS, '-');
  TryAppend(str, KEYCODE_LESS, '<');
  TryAppend(str, KEYCODE_EQUALS, '=');
  TryAppend(str, KEYCODE_GREATER, '>');
  TryAppend(str, KEYCODE_UNDERSCORE, '_');
  return str;
}

std::string Chord(const KeycodeBitset& keys, const KeyModifier& modifiers) {
  ChordMaker c(keys, modifiers);
  return c.Build();
}
}  // namespace redux
