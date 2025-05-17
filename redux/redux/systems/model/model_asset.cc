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

#include "redux/systems/model/model_asset.h"

#include <memory>
#include <string_view>
#include <utility>

#include "redux/modules/base/data_container.h"
#include "redux/modules/base/hash.h"

namespace redux {

void ModelAsset::OnLoad(std::shared_ptr<DataContainer> data) {
  asset_data_ = std::move(data);
  ProcessData();
}

const ModelAsset::TextureData* ModelAsset::GetTextureData(
    std::string_view name) const {
  auto iter = textures_.find(Hash(name));
  if (iter != textures_.end()) {
    return &iter->second;
  } else {
    return nullptr;
  }
}
}  // namespace redux
