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

#ifndef LULLABY_UTIL_STRING_PREPROCESSOR_H_
#define LULLABY_UTIL_STRING_PREPROCESSOR_H_

#include <string>

#include "lullaby/util/typeid.h"

namespace lull {
// StringPreprocessor is an interface intended to allow for last minute string
// replacement and manipulation by lullaby applications.  Any time Text is
// displayed by the RenderSystem, it will check for the existence of a
// StringPreprocessor in the registry.  If there is a preprocessor, it will
// display the output of ProcessString rather than the string it received.
//
// Example usage:
// class MyLocalizer : public lull::StringPreprocessor {
//  public:
//   std::string ProcessString(const std::string& input) override {
//     const char* result = GetLocalizedString(input);
//     if (result != nullptr) {
//       return result;
//     }
//     return input;
//   }
// };
//
//
// In the application's setup, before any entities are loaded:
// registry_.Register(
//     std::unique_ptr<lull::StringPreprocessor>(new MyLocalizer()));
class StringPreprocessor {
 public:
  enum ProcessMode {
    kLocalize,
    kLocalizeToUpperCase,
    kLiteral,
    kNoPrefix
  };

  struct ProcessStringRequest {
    // TODO(b/31468905) Change this to a const char*.
    std::string text = "";
    ProcessMode mode = kNoPrefix;
  };

  // This prefix specifies to load the localized string resource named.
  // For example, "@tab_your_photos" should give the string named
  // "tab_your_photos" from whatever string localization resource file you use.
  static constexpr char kResourceNamePrefix = '@';

  // This prefix causes the resource string to be loaded as with
  // kResourceNamePrefix, but then it is converted to upper case in the current
  // locale.
  static constexpr char kResourceUpperCasePrefix = '^';

  // This prefix causes the remainder of the string to be returned (skipping
  // any resource lookup). This allows user-data strings to be shown.
  // For example, "'john.doe@email.com" will become "john.doe@email.com".
  static constexpr char kLiteralStringPrefix = '\'';
  static constexpr const char* kLiteralStringPrefixString = "'";

  virtual ~StringPreprocessor() {}
  // |input| will be the string passed to RenderSystem.SetText or set in a
  // RenderDef's text field.
  // This function should return a corresponding localized or modified string.
  virtual std::string ProcessString(const std::string& input) = 0;


// CheckPrefix will check for and remove a prefix, returning a mode to indicate
// how the string should be processed.  This is intended for localization
// frameworks.
// Depending on the prefix of the string, the string will be processed
// differently:
//   kResourceNamePrefix - The remainder of the string specifies the name of an
//     app string resource to use.
//   kResourceUpperCasePrefix - Same as kResourceNamePrefix, except that the
//     requested string resource will be put into all caps (in the correct
//     locale).
//   kLiteralStringPrefix - The remainder of the string is returned unchanged.
  static ProcessStringRequest CheckPrefix(const std::string& input);
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::StringPreprocessor);

#endif  // LULLABY_UTIL_STRING_PREPROCESSOR_H_
