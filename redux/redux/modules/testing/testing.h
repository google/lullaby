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

#ifndef REDUX_MODULES_TESTING_TESTING_H_
#define REDUX_MODULES_TESTING_TESTING_H_

#include <string>
#include <string_view>

namespace redux {

// Returns the path that can be used to load a file for testing.
std::string ResolveTestFilePath(std::string_view uri);
std::string ResolveTestFilePath(std::string_view prefix, std::string_view uri);

}  // namespace redux

#endif  // REDUX_MODULES_TESTING_TESTING_H_
