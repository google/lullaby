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
  // A unique_ptr to the underlying fplbase::Shader.
  typedef std::unique_ptr<fplbase::Shader,
      std::function<void(const fplbase::Shader*)>>
      ShaderImplPtr;

  // Simple typedef of FPL's UniformHandle type to minimize fpl namespace usage.
  typedef fplbase::UniformHandle UniformHnd;

  // Takes ownership of the specified FPL shader and uses the |renderer| when
  // binding the shader for use.
  Shader(fplbase::Renderer* renderer, ShaderImplPtr shader);

  // Locates the uniform in the shader with the specified |name|.
  UniformHnd FindUniform(const char* name) const;

  // Sets the data for the uniform specified by |id| to the given |value|.
  void SetUniform(UniformHnd id, const float* value, size_t len);

  // Binds the shader (ie. glUseProgram) for rendering.  Common uniform values
  // are automatically updated from the FPL Renderer instance.
  void Bind();

 private:
  ShaderImplPtr impl_;
  fplbase::Renderer* renderer_;

  Shader(const Shader& rhs);
  Shader& operator=(const Shader& rhs);
};

}  // namespace lull

#endif  // LULLABY_SYSTEMS_RENDER_NEXT_SHADER_H_
