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

#include "lullaby/modules/config/config.h"

#include "lullaby/modules/file/asset_loader.h"
#include "lullaby/modules/flatbuffers/variant_fb_conversions.h"

namespace lull {

void SetConfigFromVariantMap(Config* config, const VariantMap* variant_map) {
  if (!config || !variant_map) {
    return;
  }
  for (const auto& iter : *variant_map) {
    config->Set(iter.first, iter.second);
  }
}

void SetConfigFromFlatbuffer(Config* config, const ConfigDef* config_def) {
  if (!config || !config_def) {
    return;
  }

  const auto* values = config_def->values();
  if (!values) {
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
      config->Set(key_hash, var);
    }
  }
}

void LoadConfigFromFile(Registry* registry, Config* config,
                        string_view filename) {
  AssetLoader* asset_loader = registry->Get<AssetLoader>();
  if (!asset_loader) {
    return;
  }
  auto asset = asset_loader->LoadNow<SimpleAsset>(filename.to_string());
  if (!asset) {
    return;
  }
  const auto* config_def = flatbuffers::GetRoot<ConfigDef>(asset->GetData());
  SetConfigFromFlatbuffer(config, config_def);
}

}  // namespace lull
