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

#include "redux/modules/testing/testing.h"

#include "redux/modules/base/filepath.h"
#include "redux/modules/base/logging.h"
#include "gtest/gtest.h"

namespace redux {

std::string ResolveTestFilePath(std::string_view prefix, std::string_view uri) {
  std::string result;
  result = JoinPath(result, prefix);
  result = JoinPath(result, uri);
  result = LocalizePath(result);
  result = CanonicalizePath(result);
  LOG(ERROR) << result;
  return result;
}

std::string ResolveTestFilePath(std::string_view uri) {
  return ResolveTestFilePath("", uri);
}

}  // namespace redux
