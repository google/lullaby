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

#ifndef REDUX_ENGINES_RENDER_THUNKS_INDIRECT_LIGHT_H_
#define REDUX_ENGINES_RENDER_THUNKS_INDIRECT_LIGHT_H_

#include "redux/engines/render/indirect_light.h"

namespace redux {

// Thunk functions to call the actual implsementation.
void IndirectLight::Disable() { Upcast(this)->Disable(); }
void IndirectLight::Enable() { Upcast(this)->Enable(); }
bool IndirectLight::IsEnabled() const { return Upcast(this)->IsEnabled(); }
void IndirectLight::SetTransform(const mat4& transform) {
  Upcast(this)->SetTransform(transform);
}

}  // namespace redux

#endif  // REDUX_ENGINES_RENDER_THUNKS_INDIRECT_LIGHT_H_
