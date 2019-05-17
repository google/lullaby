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

#ifndef LULLABY_VIEWER_SRC_WIDGETS_FILE_DIALOG_H_
#define LULLABY_VIEWER_SRC_WIDGETS_FILE_DIALOG_H_

#include <string>

namespace lull {
namespace tool {

std::string OpenFileDialog(const std::string& label,
                           const std::string& filter = "");

std::string OpenDirectoryDialog(const std::string& label);

std::string SaveFileDialog(const std::string& label);

}  // namespace tool
}  // namespace lull

#endif  // LULLABY_VIEWER_SRC_WIDGETS_FILE_DIALOG_H_
