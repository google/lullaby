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

#ifndef LULLABY_SYSTEMS_RENDER_NEXT_RENDER_STATE_H_
#define LULLABY_SYSTEMS_RENDER_NEXT_RENDER_STATE_H_

#include "fplbase/render_state.h"
#include "lullaby/generated/render_state_def_generated.h"

namespace lull {

// Applys the RenderState specified in |state| to the |target| renderstate.
void Apply(fplbase::RenderState* target, const AlphaTestStateT* state);
void Apply(fplbase::RenderState* target, const BlendStateT* state);
void Apply(fplbase::RenderState* target, const CullStateT* state);
void Apply(fplbase::RenderState* target, const DepthStateT* state);
void Apply(fplbase::RenderState* target, const PointStateT* state);
void Apply(fplbase::RenderState* target, const ScissorStateT* state);
void Apply(fplbase::RenderState* target, const StencilStateT* state);
void Apply(fplbase::RenderState* target, const RenderStateT& state);

// Converts the specified fplbase |state| to the lull equivalent.
AlphaTestStateT Convert(const fplbase::AlphaTestState& state);
BlendStateT Convert(const fplbase::BlendState& state);
CullStateT Convert(const fplbase::CullState& state);
DepthStateT Convert(const fplbase::DepthState& state);
PointStateT Convert(const fplbase::PointState& state);
ScissorStateT Convert(const fplbase::ScissorState& state);
StencilStateT Convert(const fplbase::StencilState& state);
RenderStateT Convert(const fplbase::RenderState& state);

}  // namespace lull

#endif  // LULLABY_SYSTEMS_RENDER_NEXT_RENDER_STATE_H_
