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

#ifndef REDUX_ENGINES_RENDER_SHADER_FACTORY_H_
#define REDUX_ENGINES_RENDER_SHADER_FACTORY_H_

#include "redux/engines/render/shader.h"
#include "redux/modules/base/registry.h"
#include "redux/modules/base/resource_manager.h"

namespace redux {

// Creates Shader objects.
class ShaderFactory {
 public:
  explicit ShaderFactory(Registry* registry);

  ShaderFactory(const ShaderFactory&) = delete;
  ShaderFactory& operator=(const ShaderFactory&) = delete;

  // Creates the shader asset associated with the shading model. If the shader
  // was previously loaded, then this will return the cached shader (since they
  // are shareable). The shader will only be unloaded once all references to it
  // are released.
  ShaderPtr CreateShader(std::string_view shading_model);

 private:
  class ShaderAsset;

  Registry* registry_ = nullptr;
  ResourceManager<ShaderAsset> assets_;
};

}  // namespace redux

REDUX_SETUP_TYPEID(redux::ShaderFactory);

#endif  // REDUX_ENGINES_RENDER_SHADER_FACTORY_H_
