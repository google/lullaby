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

#ifndef LULLABY_VIEWER_SRC_WIDGETS_BUILD_MODEL_POPUP_H_
#define LULLABY_VIEWER_SRC_WIDGETS_BUILD_MODEL_POPUP_H_

#include <string>
#include "lullaby/util/registry.h"

namespace lull {
namespace tool {

// A popup that allows users to build a .lullmodel.
class BuildModelPopup {
 public:
  explicit BuildModelPopup(Registry* registry) : registry_(registry) {}

  // Shows the popup.
  void Open();

  // Hides the popup.
  void Close();

  // Updates the popup.
  void AdvanceFrame();

 private:
  enum State {
    kClosed,
    kEnable,
    kOpen,
  };
  Registry* registry_ = nullptr;
  State state_ = kClosed;
  bool open_ = false;
  std::string filename_;
  std::string basename_;
};

}  // namespace tool
}  // namespace lull

LULLABY_SETUP_TYPEID(lull::tool::BuildModelPopup);

#endif  // LULLABY_VIEWER_SRC_WIDGETS_BUILD_MODEL_POPUP_H_
