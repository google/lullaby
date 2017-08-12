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

#include "lullaby/modules/script/lull/script_env.h"
#include "lullaby/modules/script/lull/script_frame.h"
#include "lullaby/modules/script/lull/script_types.h"
#include "mathfu/glsl_mappings.h"

namespace lull {

void CreateVec2(ScriptFrame* frame) {
  const ScriptValue x = frame->EvalNext();
  const ScriptValue y = frame->EvalNext();
  const mathfu::vec2 res(x.GetAs<float>(), y.GetAs<float>());
  frame->Return(res);
}

void CreateVec3(ScriptFrame* frame) {
  const ScriptValue x = frame->EvalNext();
  const ScriptValue y = frame->EvalNext();
  const ScriptValue z = frame->EvalNext();
  const mathfu::vec3 res(x.GetAs<float>(), y.GetAs<float>(), z.GetAs<float>());
  frame->Return(res);
}

void CreateVec4(ScriptFrame* frame) {
  const ScriptValue x = frame->EvalNext();
  const ScriptValue y = frame->EvalNext();
  const ScriptValue z = frame->EvalNext();
  const ScriptValue w = frame->EvalNext();
  const mathfu::vec4 res(x.GetAs<float>(), y.GetAs<float>(), z.GetAs<float>(),
                         w.GetAs<float>());
  frame->Return(res);
}

void CreateQuat(ScriptFrame* frame) {
  const ScriptValue x = frame->EvalNext();
  const ScriptValue y = frame->EvalNext();
  const ScriptValue z = frame->EvalNext();
  const ScriptValue w = frame->EvalNext();
  const mathfu::quat res(x.GetAs<float>(), y.GetAs<float>(), z.GetAs<float>(),
                         w.GetAs<float>());
  frame->Return(res);
}

}  // namespace lull
