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

#include "lullaby/systems/render/next/render_state.h"

namespace lull {

static RenderFunction Convert(fplbase::RenderFunction value) {
  switch (value) {
    case fplbase::kRenderAlways:
      return RenderFunction_Always;
    case fplbase::kRenderEqual:
      return RenderFunction_Equal;
    case fplbase::kRenderGreater:
      return RenderFunction_Greater;
    case fplbase::kRenderGreaterEqual:
      return RenderFunction_GreaterEqual;
    case fplbase::kRenderLess:
      return RenderFunction_Less;
    case fplbase::kRenderLessEqual:
      return RenderFunction_LessEqual;
    case fplbase::kRenderNever:
      return RenderFunction_Never;
    case fplbase::kRenderNotEqual:
      return RenderFunction_NotEqual;
    default:
      LOG(DFATAL) << "Unknown render function.";
      return RenderFunction_Always;
  }
}

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

static BlendFactor Convert(fplbase::BlendState::BlendFactor value) {
  switch (value) {
    case fplbase::BlendState::kZero:
      return BlendFactor_Zero;
    case fplbase::BlendState::kOne:
      return BlendFactor_One;
    case fplbase::BlendState::kSrcColor:
      return BlendFactor_SrcColor;
    case fplbase::BlendState::kOneMinusSrcColor:
      return BlendFactor_OneMinusSrcColor;
    case fplbase::BlendState::kDstColor:
      return BlendFactor_DstColor;
    case fplbase::BlendState::kOneMinusDstColor:
      return BlendFactor_OneMinusDstColor;
    case fplbase::BlendState::kSrcAlpha:
      return BlendFactor_SrcAlpha;
    case fplbase::BlendState::kOneMinusSrcAlpha:
      return BlendFactor_OneMinusSrcAlpha;
    case fplbase::BlendState::kDstAlpha:
      return BlendFactor_DstAlpha;
    case fplbase::BlendState::kOneMinusDstAlpha:
      return BlendFactor_OneMinusDstAlpha;
    case fplbase::BlendState::kConstantColor:
      return BlendFactor_ConstantColor;
    case fplbase::BlendState::kOneMinusConstantColor:
      return BlendFactor_OneMinusConstantColor;
    case fplbase::BlendState::kConstantAlpha:
      return BlendFactor_ConstantAlpha;
    case fplbase::BlendState::kOneMinusConstantAlpha:
      return BlendFactor_OneMinusConstantAlpha;
    case fplbase::BlendState::kSrcAlphaSaturate:
      return BlendFactor_SrcAlphaSaturate;
    default:
      LOG(DFATAL) << "Unknown blend factor.";
      return BlendFactor_Zero;
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

static FrontFace Convert(fplbase::CullState::FrontFace value) {
  switch (value) {
    case fplbase::CullState::kClockWise:
      return FrontFace_Clockwise;
    case fplbase::CullState::kCounterClockWise:
      return FrontFace_CounterClockwise;
    default:
      LOG(DFATAL) << "Unknown cull front value.";
      return FrontFace_Clockwise;
  }
}

static fplbase::CullState::FrontFace Convert(FrontFace value) {
  switch (value) {
    case FrontFace_Clockwise:
      return fplbase::CullState::kClockWise;
    case FrontFace_CounterClockwise:
      return fplbase::CullState::kCounterClockWise;
    default:
      LOG(DFATAL) << "Unknown cull front value.";
      return fplbase::CullState::kClockWise;
  }
}

static CullFace Convert(fplbase::CullState::CullFace value) {
  switch (value) {
    case fplbase::CullState::kFront:
      return CullFace_Front;
    case fplbase::CullState::kBack:
      return CullFace_Back;
    case fplbase::CullState::kFrontAndBack:
      return CullFace_FrontAndBack;
    default:
      LOG(DFATAL) << "Unknown cull face value.";
      return CullFace_Front;
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

static StencilAction Convert(
    fplbase::StencilOperation::StencilOperations value) {
  switch (value) {
    case fplbase::StencilOperation::kKeep:
      return StencilAction_Keep;
    case fplbase::StencilOperation::kZero:
      return StencilAction_Zero;
    case fplbase::StencilOperation::kReplace:
      return StencilAction_Replace;
    case fplbase::StencilOperation::kIncrement:
      return StencilAction_Increment;
    case fplbase::StencilOperation::kIncrementAndWrap:
      return StencilAction_IncrementAndWrap;
    case fplbase::StencilOperation::kDecrement:
      return StencilAction_Decrement;
    case fplbase::StencilOperation::kDecrementAndWrap:
      return StencilAction_DecrementAndWrap;
    case fplbase::StencilOperation::kInvert:
      return StencilAction_Invert;
    default:
      LOG(DFATAL) << "Unknown stencil action.";
      return StencilAction_Keep;
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
  target->cull_state.front = Convert(state->front);
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
    target->viewport.pos.x = state.viewport->pos.x;
    target->viewport.pos.y = state.viewport->pos.y;
    target->viewport.size.x = state.viewport->size.x;
    target->viewport.size.y = state.viewport->size.y;
  }
}

AlphaTestStateT Convert(const fplbase::AlphaTestState& state) {
  AlphaTestStateT res;
  res.enabled = state.enabled;
  res.function = Convert(state.function);
  res.ref = state.ref;
  return res;
}

BlendStateT Convert(const fplbase::BlendState& state) {
  BlendStateT res;
  res.enabled = state.enabled;
  res.src_alpha = Convert(state.src_alpha);
  res.src_color = Convert(state.src_color);
  res.dst_alpha = Convert(state.dst_alpha);
  res.dst_color = Convert(state.dst_color);
  return res;
}

CullStateT Convert(const fplbase::CullState& state) {
  CullStateT res;
  res.face = Convert(state.face);
  res.enabled = state.enabled;
  res.front = Convert(state.front);
  return res;
}

DepthStateT Convert(const fplbase::DepthState& state) {
  DepthStateT res;
  res.function = Convert(state.function);
  res.test_enabled = state.test_enabled;
  res.write_enabled = state.write_enabled;
  return res;
}

PointStateT Convert(const fplbase::PointState& state) {
  PointStateT res;
  res.point_sprite_enabled = state.point_sprite_enabled;
  res.program_point_size_enabled = state.program_point_size_enabled;
  res.point_size = state.point_size;
  return res;
}

ScissorStateT Convert(const fplbase::ScissorState& state) {
  ScissorStateT res;
  res.enabled = state.enabled;
  res.rect = state.rect;
  return res;
}

StencilStateT Convert(const fplbase::StencilState& state) {
  StencilStateT res;
  res.enabled = state.enabled;
  res.back_function.function = Convert(state.back_function.function);
  res.back_function.ref = state.back_function.ref;
  res.back_function.mask = state.back_function.mask;
  res.back_op.stencil_fail = Convert(state.back_op.stencil_fail);
  res.back_op.depth_fail = Convert(state.back_op.depth_fail);
  res.back_op.pass = Convert(state.back_op.pass);
  res.front_function.function = Convert(state.front_function.function);
  res.front_function.ref = state.front_function.ref;
  res.front_function.mask = state.front_function.mask;
  res.front_op.stencil_fail = Convert(state.front_op.stencil_fail);
  res.front_op.depth_fail = Convert(state.front_op.depth_fail);
  res.front_op.pass = Convert(state.front_op.pass);
  return res;
}

RenderStateT Convert(const fplbase::RenderState& state) {
  RenderStateT res;
  res.alpha_test_state = Convert(state.alpha_test_state);
  res.blend_state = Convert(state.blend_state);
  res.cull_state = Convert(state.cull_state);
  res.depth_state = Convert(state.depth_state);
  res.point_state = Convert(state.point_state);
  res.scissor_state = Convert(state.scissor_state);
  res.stencil_state = Convert(state.stencil_state);
  return res;
}

}  // namespace lull
