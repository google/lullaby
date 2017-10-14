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

#ifndef LULLABY_SYSTEMS_RENDER_NEXT_SHADER_H_
#define LULLABY_SYSTEMS_RENDER_NEXT_SHADER_H_

#include <memory>
#include "fplbase/renderer.h"
#include "fplbase/shader.h"
#include "lullaby/systems/render/shader.h"

namespace lull {

// Owns fplbase::Shader and provides access to functionality needed for
// rendering.
class Shader {
 public:
  Shader() {}
  Shader(const Shader& rhs) = delete;
  Shader& operator=(const Shader& rhs) = delete;

  // Simple typedef of FPL's UniformHandle type to minimize fpl namespace usage.
  using UniformHnd = fplbase::UniformHandle;

  // Locates the uniform in the shader with the specified |name|.
  UniformHnd FindUniform(const char* name) const;

  // Sets the data for the uniform specified by |id| to the given |value|.
  void SetUniform(UniformHnd id, const float* value, size_t len);

  // Binds the shader (ie. glUseProgram) for rendering.  Common uniform values
  // are automatically updated from the FPL Renderer instance.
  void Bind();

 private:
  friend class ShaderFactory;
  void Init(fplbase::Renderer* renderer,
            std::unique_ptr<fplbase::Shader> shader_impl);

  fplbase::Renderer* renderer_ = nullptr;
  std::unique_ptr<fplbase::Shader> impl_;
};

}  // namespace lull

#endif  // LULLABY_SYSTEMS_RENDER_NEXT_SHADER_H_
