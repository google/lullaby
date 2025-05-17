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

#include "redux/modules/usd/init_usd_plugins.h"

#include <string>
#include <vector>

#include "absl/log/check.h"
#include "redux/modules/base/registry.h"
#include "redux/modules/usd/usd_asset_resolver.h"
#include "pxr/base/plug/registry.h"
#include "pxr/usd/ar/resolver.h"

namespace redux {

void InitUsdPlugins(Registry* registry,
                    const std::vector<std::string>& plugin_paths) {
  pxr::PlugRegistry::GetInstance().RegisterPlugins(plugin_paths);

  const char* type_name = "redux::UsdAssetResolver";

  const pxr::TfType type = pxr::PlugRegistry::FindTypeByName(type_name);
  CHECK(type && type.IsA<pxr::ArResolver>())
      << "Custom resolver plugin has not been found: " << type_name;

  pxr::ArSetPreferredResolver(type_name);
  UsdAssetResolver* resolver = GetGlobalUsdAssetResolver();
  resolver->BindRegistry(registry);
}
}  // namespace redux
