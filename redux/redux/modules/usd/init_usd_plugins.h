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

#ifndef REDUX_MODULES_USD_INIT_USD_PLUGINS_H_
#define REDUX_MODULES_USD_INIT_USD_PLUGINS_H_

#include <string>
#include <vector>
#include "redux/modules/base/registry.h"

namespace redux {

// Registers plugins with the USD library and configures it for use with the
// redux library.
void InitUsdPlugins(Registry* registry,
                    const std::vector<std::string>& plugin_paths);

}  // namespace redux

#endif  // REDUX_MODULES_USD_INIT_USD_PLUGINS_H_
