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

#ifndef REDUX_ENGINES_RENDER_SHADER_H_
#define REDUX_ENGINES_RENDER_SHADER_H_

#include <memory>

namespace redux {

// Shaders are used to "color in" the surface of a renderable in a particular
// way. For example, some shaders can be "flat" which will color the entire
// surface of a renderable with a single color. Other shaders can use
// "physically-based rendering" which uses complex algorithms to simulate the
// physical properties of light bouncing of the surface of a renderable.
class Shader {
 public:
  virtual ~Shader() = default;

  Shader(const Shader&) = delete;
  Shader& operator=(const Shader&) = delete;

 protected:
  Shader() = default;
};

using ShaderPtr = std::shared_ptr<Shader>;

}  // namespace redux

#endif  // REDUX_ENGINES_RENDER_SHADER_H_
