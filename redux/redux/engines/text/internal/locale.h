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

#ifndef REDUX_ENGINES_TEXT_INTERNAL_LOCALE_H_
#define REDUX_ENGINES_TEXT_INTERNAL_LOCALE_H_

#include "redux/engines/text/text_enums.h"

namespace redux {

// From wikipedia:
//
// ISO 639: concerned with representation of names for languages and language
// groups.
//
// ISO 15924: Codes for the representation of names of scripts, defines two sets
// of codes for a number of writing systems (scripts). ... Where possible the
// codes are derived from ISO 639-2.

// Returns the ISO 15924 string name of a language given in ISO 639 format.
const char* GetTextScriptIso15924(const char* language_iso_639);

// Returns the default text direction for a given language.
TextDirection GetDefaultTextDirection(const char* language_iso_639);

}  // namespace redux

#endif  // REDUX_ENGINES_TEXT_INTERNAL_LOCALE_H_
