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

#include "lullaby/systems/render/next/render_state.h"

namespace lull {

static fplbase::RenderFunction Convert(RenderFunction value) {
  switch (value) {
    case RenderFunction_Always:
      return fplbase::kRenderAlways;
    case RenderFunction_Equal:
      return fplbase::kRenderEqual;
    case RenderFunction_Greater:
      return fplbase::kRenderGreater;
    case RenderFunction_GreaterEqual:
      return fplbase::kRenderGreaterEqual;
    case RenderFunction_Less:
      return fplbase::kRenderLess;
    case RenderFunction_LessEqual:
      return fplbase::kRenderLessEqual;
    case RenderFunction_Never:
      return fplbase::kRenderNever;
    case RenderFunction_NotEqual:
      return fplbase::kRenderNotEqual;
    default:
      LOG(DFATAL) << "Unknown render function.";
      return fplbase::kRenderCount;
  }
}

static fplbase::BlendState::BlendFactor Convert(BlendFactor value) {
  switch (value) {
    case BlendFactor_Zero:
      return fplbase::BlendState::kZero;
    case BlendFactor_One:
      return fplbase::BlendState::kOne;
    case BlendFactor_SrcColor:
      return fplbase::BlendState::kSrcColor;
    case BlendFactor_OneMinusSrcColor:
      return fplbase::BlendState::kOneMinusSrcColor;
    case BlendFactor_DstColor:
      return fplbase::BlendState::kDstColor;
    case BlendFactor_OneMinusDstColor:
      return fplbase::BlendState::kOneMinusDstColor;
    case BlendFactor_SrcAlpha:
      return fplbase::BlendState::kSrcAlpha;
    case BlendFactor_OneMinusSrcAlpha:
      return fplbase::BlendState::kOneMinusSrcAlpha;
    case BlendFactor_DstAlpha:
      return fplbase::BlendState::kDstAlpha;
    case BlendFactor_OneMinusDstAlpha:
      return fplbase::BlendState::kOneMinusDstAlpha;
    case BlendFactor_ConstantColor:
      return fplbase::BlendState::kConstantColor;
    case BlendFactor_OneMinusConstantColor:
      return fplbase::BlendState::kOneMinusConstantColor;
    case BlendFactor_ConstantAlpha:
      return fplbase::BlendState::kConstantAlpha;
    case BlendFactor_OneMinusConstantAlpha:
      return fplbase::BlendState::kOneMinusConstantAlpha;
    case BlendFactor_SrcAlphaSaturate:
      return fplbase::BlendState::kSrcAlphaSaturate;
    default:
      LOG(DFATAL) << "Unknown blend factor.";
      return fplbase::BlendState::kCount;
  }
}

static fplbase::CullState::CullFace Convert(CullFace value) {
  switch (value) {
    case CullFace_Front:
      return fplbase::CullState::kFront;
    case CullFace_Back:
      return fplbase::CullState::kBack;
    case CullFace_FrontAndBack:
      return fplbase::CullState::kFrontAndBack;
    default:
      LOG(DFATAL) << "Unknown cull face value.";
      return fplbase::CullState::kCullFaceCount;
  }
}

static fplbase::StencilOperation::StencilOperations Convert(
    StencilAction value) {
  switch (value) {
    case StencilAction_Keep:
      return fplbase::StencilOperation::kKeep;
    case StencilAction_Zero:
      return fplbase::StencilOperation::kZero;
    case StencilAction_Replace:
      return fplbase::StencilOperation::kReplace;
    case StencilAction_Increment:
      return fplbase::StencilOperation::kIncrement;
    case StencilAction_IncrementAndWrap:
      return fplbase::StencilOperation::kIncrementAndWrap;
    case StencilAction_Decrement:
      return fplbase::StencilOperation::kDecrement;
    case StencilAction_DecrementAndWrap:
      return fplbase::StencilOperation::kDecrementAndWrap;
    case StencilAction_Invert:
      return fplbase::StencilOperation::kInvert;
    default:
      LOG(DFATAL) << "Unknown stencil action.";
      return fplbase::StencilOperation::kCount;
  }
}

void Apply(fplbase::RenderState* target, const AlphaTestStateT* state) {
  if (target == nullptr || state == nullptr) {
    return;
  }
  target->alpha_test_state.enabled = state->enabled;
  target->alpha_test_state.ref = state->ref;
  target->alpha_test_state.function = Convert(state->function);
}

void Apply(fplbase::RenderState* target, const BlendStateT* state) {
  if (target == nullptr || state == nullptr) {
    return;
  }
  target->blend_state.enabled = state->enabled;
  target->blend_state.src_alpha = Convert(state->src_alpha);
  target->blend_state.src_color = Convert(state->src_color);
  target->blend_state.dst_alpha = Convert(state->dst_alpha);
  target->blend_state.dst_color = Convert(state->dst_color);
}

void Apply(fplbase::RenderState* target, const CullStateT* state) {
  if (target == nullptr || state == nullptr) {
    return;
  }
  target->cull_state.enabled = state->enabled;
  target->cull_state.face = Convert(state->face);
}

void Apply(fplbase::RenderState* target, const DepthStateT* state) {
  if (target == nullptr || state == nullptr) {
    return;
  }
  target->depth_state.test_enabled = state->test_enabled;
  target->depth_state.write_enabled = state->write_enabled;
  target->depth_state.function = Convert(state->function);
}

void Apply(fplbase::RenderState* target, const PointStateT* state) {
  if (target == nullptr || state == nullptr) {
    return;
  }
  target->point_state.point_sprite_enabled = state->point_sprite_enabled;
  target->point_state.program_point_size_enabled =
      state->program_point_size_enabled;
  target->point_state.point_size = state->point_size;
}

void Apply(fplbase::RenderState* target, const ScissorStateT* state) {
  if (target == nullptr || state == nullptr) {
    return;
  }
  target->scissor_state.enabled = state->enabled;
}

void Apply(fplbase::RenderState* target, const StencilStateT* state) {
  if (target == nullptr || state == nullptr) {
    return;
  }
  target->stencil_state.enabled = state->enabled;

  target->stencil_state.back_function.function =
      Convert(state->back_function.function);
  target->stencil_state.back_function.ref = state->back_function.ref;
  target->stencil_state.back_function.mask = state->back_function.mask;

  target->stencil_state.back_op.stencil_fail =
      Convert(state->back_op.stencil_fail);
  target->stencil_state.back_op.depth_fail = Convert(state->back_op.depth_fail);
  target->stencil_state.back_op.pass = Convert(state->back_op.pass);

  target->stencil_state.front_function.function =
      Convert(state->front_function.function);
  target->stencil_state.front_function.ref = state->front_function.ref;
  target->stencil_state.front_function.mask = state->front_function.mask;

  target->stencil_state.front_op.stencil_fail =
      Convert(state->front_op.stencil_fail);
  target->stencil_state.front_op.depth_fail =
      Convert(state->front_op.depth_fail);
  target->stencil_state.front_op.pass = Convert(state->front_op.pass);
}

void Apply(fplbase::RenderState* target, const RenderStateT& state) {
  Apply(target, state.alpha_test_state.get());
  Apply(target, state.blend_state.get());
  Apply(target, state.cull_state.get());
  Apply(target, state.depth_state.get());
  Apply(target, state.point_state.get());
  Apply(target, state.scissor_state.get());
  Apply(target, state.stencil_state.get());
  if (state.viewport) {
    target->viewport.pos.x = static_cast<int>(state.viewport->pos.x);
    target->viewport.pos.y = static_cast<int>(state.viewport->pos.y);
    target->viewport.size.x = static_cast<int>(state.viewport->size.x);
    target->viewport.size.y = static_cast<int>(state.viewport->size.y);
  }
}

}  // namespace lull
