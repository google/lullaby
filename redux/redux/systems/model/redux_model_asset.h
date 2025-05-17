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

#ifndef REDUX_SYSTEMS_MODEL_REDUX_MODEL_ASSET_H_
#define REDUX_SYSTEMS_MODEL_REDUX_MODEL_ASSET_H_

#include <stdint.h>

#include <cstddef>
#include <functional>
#include <memory>
#include <string_view>

#include "redux/modules/base/registry.h"
#include "redux/modules/graphics/image_data.h"
#include "redux/systems/model/model_asset.h"

namespace redux {

// Parses a model file and extracts the relevant information so that it
// can be consumed at runtime.
class ReduxModelAsset : public ModelAsset {
 public:
  ReduxModelAsset(Registry* registry, std::string_view uri)
      : ModelAsset(registry, uri) {}

 protected:
  virtual void ProcessData();
  std::shared_ptr<ImageData> ReadImage(ImageData image);
};

}  // namespace redux

#endif  // REDUX_SYSTEMS_MODEL_REDUX_MODEL_ASSET_H_
