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

#include "lullaby/modules/config/config.h"

#include "lullaby/modules/file/asset_loader.h"
#include "lullaby/modules/flatbuffers/variant_fb_conversions.h"

namespace lull {

std::string Config::LoadFile(Registry* registry, string_view filename) {
  if (!registry) {
    return "";
  }

  AssetLoader* asset_loader = registry->Get<AssetLoader>();
  if (!asset_loader) {
    return "";
  }
  auto asset = asset_loader->LoadNow<SimpleAsset>(std::string(filename));
  if (!asset || asset->GetSize() == 0) {
    return "";
  }
  return asset->ReleaseData();
}

void Config::LoadConfig(Registry* registry,  string_view filename) {
  const std::string data = LoadFile(registry, filename);
  if (data.empty()) {
    return;
  }

  flatbuffers::Verifier verifier(reinterpret_cast<const uint8_t*>(data.data()),
                                 data.size());
  if (!verifier.VerifyBuffer<ConfigDef>()) {
    LOG(DFATAL) << "Invalid flatbuffer object.";
    return;
  }

  const auto* config_def = flatbuffers::GetRoot<ConfigDef>(data.data());
  if (config_def == nullptr) {
    return;
  }

  const auto* values = config_def->values();
  if (values == nullptr) {
    return;
  }

  for (auto iter = values->begin(); iter != values->end(); ++iter) {
    const auto* key = iter->key();
    const void* variant_def = iter->value();
    if (key == nullptr || variant_def == nullptr) {
      LOG(ERROR) << "Invalid (nullptr) key-value data in ConfigDef.";
      continue;
    }

    Variant var;
    if (VariantFromFbVariant(iter->value_type(), variant_def, &var)) {
      const HashValue key_hash = Hash(key->c_str());
      Set(key_hash, var);
    }
  }
}

}  // namespace lull
