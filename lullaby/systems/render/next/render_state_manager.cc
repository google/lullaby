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

#include "lullaby/systems/render/next/render_state_manager.h"

#include "lullaby/systems/render/next/detail/glplatform.h"

// The Mac requires a Core OpenGL profile in order to access GLES 3+.
#if !defined(GL_CORE_PROFILE) && defined(PLATFORM_OSX)
#define GL_CORE_PROFILE
#endif

namespace lull {

// Checks if the specified GL |parameter| matches the provided |test| value.
// Assumes the |test| value is an integral type and queries the |parameter|
// using glGetInteger.
template <typename T>
static bool CheckGlInt(GLenum parameter, T test) {
  GLint value = 0;
  GL_CALL(glGetIntegerv(parameter, &value));
  if (value == static_cast<GLint>(test)) {
    return true;
  } else {
    LOG(ERROR) << "Unexpected GL state for parameter " << parameter;
    LOG(ERROR) << "Expected: " << test << ", Actual: " << value;
    return false;
  }
}

// Checks if the specified GL |parameter| matches the provided |test| value.
// Assumes the |test| value is an floating-point type and queries the
// |parameter| using glGetFloat.
template <typename T>
static bool CheckGlFloat(GLenum parameter, T test) {
  float value = 0;
  GL_CALL(glGetFloatv(parameter, &value));
  if (value == static_cast<float>(test)) {
    return true;
  } else {
    LOG(ERROR) << "Unexpected GL state for parameter " << parameter;
    LOG(ERROR) << "Expected: " << test << ", Actual: " << value;
    return false;
  }
}

// Checks if the specified GL |parameter| matches the provided boolean |test|
// value. Queries the |parameter| using glGetBoolean.
static bool CheckGlBool(GLenum parameter, bool test) {
  GLboolean value = GL_FALSE;
  GL_CALL(glGetBooleanv(parameter, &value));
  const bool bool_value = (value == GL_TRUE);
  if (bool_value == test) {
    return true;
  } else {
    LOG(ERROR) << "Unexpected GL state for parameter " << parameter;
    LOG(ERROR) << "Expected: " << test << ", Actual: " << bool_value;
    return false;
  }
}

static GLenum GetGlRenderFunction(RenderFunction func) {
  switch (func) {
    case RenderFunction_Always:
      return GL_ALWAYS;
    case RenderFunction_Equal:
      return GL_EQUAL;
    case RenderFunction_Greater:
      return GL_GREATER;
    case RenderFunction_GreaterEqual:
      return GL_GEQUAL;
    case RenderFunction_Less:
      return GL_LESS;
    case RenderFunction_LessEqual:
      return GL_LEQUAL;
    case RenderFunction_Never:
      return GL_NEVER;
    case RenderFunction_NotEqual:
      return GL_NOTEQUAL;
    default:
      LOG(DFATAL) << "Unknown function type: " << func;
      return GL_LESS;
  }
}

static GLenum GetGlBlendFactor(BlendFactor factor) {
  switch (factor) {
    case BlendFactor_Zero:
      return GL_ZERO;
    case BlendFactor_One:
      return GL_ONE;
    case BlendFactor_SrcColor:
      return GL_SRC_COLOR;
    case BlendFactor_OneMinusSrcColor:
      return GL_ONE_MINUS_SRC_COLOR;
    case BlendFactor_DstColor:
      return GL_DST_COLOR;
    case BlendFactor_OneMinusDstColor:
      return GL_ONE_MINUS_DST_COLOR;
    case BlendFactor_SrcAlpha:
      return GL_SRC_ALPHA;
    case BlendFactor_OneMinusSrcAlpha:
      return GL_ONE_MINUS_SRC_ALPHA;
    case BlendFactor_DstAlpha:
      return GL_DST_ALPHA;
    case BlendFactor_OneMinusDstAlpha:
      return GL_ONE_MINUS_DST_ALPHA;
    case BlendFactor_ConstantColor:
      return GL_CONSTANT_COLOR;
    case BlendFactor_OneMinusConstantColor:
      return GL_ONE_MINUS_CONSTANT_COLOR;
    case BlendFactor_ConstantAlpha:
      return GL_CONSTANT_ALPHA;
    case BlendFactor_OneMinusConstantAlpha:
      return GL_ONE_MINUS_CONSTANT_ALPHA;
    case BlendFactor_SrcAlphaSaturate:
      return GL_SRC_ALPHA_SATURATE;
    default:
      LOG(DFATAL) << "Unknown factor: " << factor;
      return GL_ZERO;
  }
}

static GLenum GetGlCullFace(CullFace face) {
  switch (face) {
    case CullFace_Front:
      return GL_FRONT;
    case CullFace_Back:
      return GL_BACK;
    case CullFace_FrontAndBack:
      return GL_FRONT_AND_BACK;
    default:
      LOG(DFATAL) << "Unknown cull face: " << face;
      return GL_FRONT;
  }
}

static GLenum GetGlFrontFace(FrontFace face) {
  switch (face) {
    case FrontFace_Clockwise:
      return GL_CW;
    case FrontFace_CounterClockwise:
      return GL_CCW;
    default:
      LOG(DFATAL) << "Unknown cull face: " << face;
      return GL_CCW;
  }
}

static GLenum GetGlStencilAction(StencilAction action) {
  switch (action) {
    case StencilAction_Keep:
      return GL_KEEP;
    case StencilAction_Zero:
      return GL_ZERO;
    case StencilAction_Replace:
      return GL_REPLACE;
    case StencilAction_Increment:
      return GL_INCR;
    case StencilAction_IncrementAndWrap:
      return GL_INCR_WRAP;
    case StencilAction_Decrement:
      return GL_DECR;
    case StencilAction_DecrementAndWrap:
      return GL_DECR_WRAP;
    case StencilAction_Invert:
      return GL_INVERT;
    default:
      LOG(DFATAL) << "Unknown stencil action: " << action;
      return GL_KEEP;
  }
}

void RenderStateManager::Reset() { state_ = RenderStateT(); }

static bool ValidateAlphaTestState(const AlphaTestStateT& state) {
  bool ok = true;
#if !defined(GL_CORE_PROFILE) && !defined(FPLBASE_GLES)
  ok &= CheckGlBool(GL_ALPHA_TEST, state.enabled);
  ok &= CheckGlInt(GL_ALPHA_TEST_FUNC, GetGlRenderFunction(state.function));
  ok &= CheckGlFloat(GL_ALPHA_TEST_REF, state.ref);
  if (!ok) {
    LOG(ERROR) << "ValidateAlphaTestState failed";
  }
#endif
  return ok;
}

static bool ValidateBlendState(const BlendStateT& state) {
  bool ok = true;
  ok &= CheckGlBool(GL_BLEND, state.enabled);
  ok &= CheckGlInt(GL_BLEND_SRC_RGB, GetGlBlendFactor(state.src_color));
  ok &= CheckGlInt(GL_BLEND_SRC_ALPHA, GetGlBlendFactor(state.src_alpha));
  ok &= CheckGlInt(GL_BLEND_DST_RGB, GetGlBlendFactor(state.dst_color));
  ok &= CheckGlInt(GL_BLEND_DST_ALPHA, GetGlBlendFactor(state.dst_alpha));
  if (!ok) {
    LOG(ERROR) << "ValidateBlendState failed";
  }
  return ok;
}

static bool ValidateCullState(const CullStateT& state) {
  bool ok = true;
  ok &= CheckGlBool(GL_CULL_FACE, state.enabled);
  ok &= CheckGlInt(GL_CULL_FACE_MODE, GetGlCullFace(state.face));
  ok &= CheckGlInt(GL_FRONT_FACE, GetGlFrontFace(state.front));
  if (!ok) {
    LOG(ERROR) << "ValidateCullState failed";
  }
  return ok;
}

static bool ValidateDepthState(const DepthStateT& state) {
  bool ok = true;
  ok &= CheckGlBool(GL_DEPTH_TEST, state.test_enabled);
  ok &= CheckGlBool(GL_DEPTH_WRITEMASK, state.write_enabled);
  ok &= CheckGlInt(GL_DEPTH_FUNC, GetGlRenderFunction(state.function));
  if (!ok) {
    LOG(ERROR) << "ValidateDepthState failed";
  }
  return ok;
}

static bool ValidatePointState(const PointStateT& state) {
  bool ok = true;
#if !defined(FPLBASE_GLES) && defined(GL_POINT_SPRITE)
  ok &= CheckGlBool(GL_POINT_SPRITE, state.point_sprite_enabled);
#endif
#if !defined(FPLBASE_GLES) && defined(GL_PROGRAM_POINT_SIZE)
  ok &= CheckGlBool(GL_PROGRAM_POINT_SIZE, state.program_point_size_enabled);
#elif !defined(FPLBASE_GLES) && defined(GL_VERTEX_PROGRAM_POINT_SIZE)
  ok &= CheckGlBool(GL_VERTEX_PROGRAM_POINT_SIZE,
                    state.program_point_size_enabled);
#endif
#if !defined(FPLBASE_GLES)
  ok &= CheckGlFloat(GL_POINT_SIZE, state.point_size);
#endif
  if (!ok) {
    LOG(ERROR) << "ValidatePointState failed";
  }
  return ok;
}

static bool ValidateScissorState(const ScissorStateT& state) {
  bool ok = true;
  ok &= CheckGlBool(GL_SCISSOR_TEST, state.enabled);
  GLint values[4];
  GL_CALL(glGetIntegerv(GL_SCISSOR_BOX, values));
  ok &= (values[0] != state.rect->pos.x || values[1] != state.rect->pos.y ||
         values[2] != state.rect->size.x || values[3] != state.rect->size.y);
  if (!ok) {
    LOG(ERROR) << "ValidateScissorState failed";
  }
  return ok;
}

static bool ValidateStencilState(const StencilStateT& state) {
  bool ok = true;
  ok &= CheckGlBool(GL_STENCIL_TEST, state.enabled);
  ok &= CheckGlInt(GL_STENCIL_FUNC,
                   GetGlRenderFunction(state.front_function.function));
  ok &= CheckGlInt(GL_STENCIL_REF, state.front_function.ref);
  ok &= CheckGlInt(GL_STENCIL_VALUE_MASK, state.front_function.mask);
  ok &= CheckGlInt(GL_STENCIL_FAIL,
                   GetGlStencilAction(state.front_op.stencil_fail));
  ok &= CheckGlInt(GL_STENCIL_PASS_DEPTH_FAIL,
                   GetGlStencilAction(state.front_op.depth_fail));
  ok &= CheckGlInt(GL_STENCIL_PASS_DEPTH_PASS,
                   GetGlStencilAction(state.front_op.pass));
  ok &= CheckGlInt(GL_STENCIL_BACK_FUNC,
                   GetGlRenderFunction(state.back_function.function));
  ok &= CheckGlInt(GL_STENCIL_BACK_REF, state.back_function.ref);
  ok &= CheckGlInt(GL_STENCIL_BACK_VALUE_MASK, state.back_function.mask);
  ok &= CheckGlInt(GL_STENCIL_BACK_FAIL,
                   GetGlStencilAction(state.back_op.stencil_fail));
  ok &= CheckGlInt(GL_STENCIL_BACK_PASS_DEPTH_FAIL,
                   GetGlStencilAction(state.back_op.depth_fail));
  ok &= CheckGlInt(GL_STENCIL_BACK_PASS_DEPTH_PASS,
                   GetGlStencilAction(state.back_op.pass));
  if (!ok) {
    LOG(ERROR) << "ValidateStencilState failed";
  }
  return ok;
}

static bool ValidateViewport(const mathfu::recti& rect) {
  GLint values[4];
  GL_CALL(glGetIntegerv(GL_VIEWPORT, values));
  if (values[0] != rect.pos.x || values[1] != rect.pos.y ||
      values[2] != rect.size.x || values[3] != rect.size.y) {
    LOG(ERROR) << "ValidateViewport failed";
    return false;
  }
  return true;
}

bool RenderStateManager::Validate() const {
  bool ok = true;
  if (state_.alpha_test_state) {
    ok &= ValidateAlphaTestState(*state_.alpha_test_state);
  }
  if (state_.blend_state) {
    ok &= ValidateBlendState(*state_.blend_state);
  }
  if (state_.cull_state) {
    ok &= ValidateCullState(*state_.cull_state);
  }
  if (state_.depth_state) {
    ok &= ValidateDepthState(*state_.depth_state);
  }
  if (state_.point_state) {
    ok &= ValidatePointState(*state_.point_state);
  }
  if (state_.scissor_state) {
    ok &= ValidateScissorState(*state_.scissor_state);
  }
  if (state_.stencil_state) {
    ok &= ValidateStencilState(*state_.stencil_state);
  }
  if (state_.viewport) {
    ok &= ValidateViewport(*state_.viewport);
  }
  return ok;
}

const RenderStateT& RenderStateManager::GetRenderState() const {
  return state_;
}

void RenderStateManager::SetRenderState(const RenderStateT& state) {
  if (state.alpha_test_state) {
    SetAlphaTestState(*state.alpha_test_state);
  }
  if (state.blend_state) {
    SetBlendState(*state.blend_state);
  }
  if (state.color_state) {
    SetColorState(*state.color_state);
  }
  if (state.cull_state) {
    SetCullState(*state.cull_state);
  }
  if (state.depth_state) {
    SetDepthState(*state.depth_state);
  }
  if (state.point_state) {
    SetPointState(*state.point_state);
  }
  if (state.scissor_state) {
    SetScissorState(*state.scissor_state);
  }
  if (state.stencil_state) {
    SetStencilState(*state.stencil_state);
  }
  if (state.viewport) {
    SetViewport(*state.viewport);
  }
}

static void SetGlAlphaTestEnabled(const AlphaTestStateT& state) {
  // Alpha test not supported in ES 2.
#if !defined(GL_CORE_PROFILE) && !defined(FPLBASE_GLES)
  if (state.enabled) {
    GL_CALL(glEnable(GL_ALPHA_TEST));
  } else {
    GL_CALL(glDisable(GL_ALPHA_TEST));
  }
#endif
}

static void SetGlAlphaFunc(const AlphaTestStateT& state) {
  // Alpha test not supported in ES 2.
#if !defined(GL_CORE_PROFILE) && !defined(FPLBASE_GLES)
  const GLenum func = GetGlRenderFunction(state.function);
  GL_CALL(glAlphaFunc(func, state.ref));
#endif
}

void RenderStateManager::SetAlphaTestState(const AlphaTestStateT& state) {
  bool update = false;

  if (!state_.alpha_test_state ||
      state.enabled != state_.alpha_test_state->enabled) {
    SetGlAlphaTestEnabled(state);
    update = true;
  }

  if (!state_.alpha_test_state || state.ref != state_.alpha_test_state->ref ||
      state.function != state_.alpha_test_state->function) {
    SetGlAlphaFunc(state);
    update = true;
  }

  if (update) {
    state_.alpha_test_state = state;
  }
}

static void SetGlBlendEnabled(const BlendStateT& state) {
  if (state.enabled) {
    GL_CALL(glEnable(GL_BLEND));
  } else {
    GL_CALL(glDisable(GL_BLEND));
  }
}

static void SetGlBlendFunc(const BlendStateT& state) {
  const GLenum src_factor = GetGlBlendFactor(state.src_alpha);
  const GLenum dst_factor = GetGlBlendFactor(state.dst_alpha);
  GL_CALL(glBlendFunc(src_factor, dst_factor));
}

void RenderStateManager::SetBlendState(const BlendStateT& state) {
  bool update = false;

  if (!state_.blend_state || state.enabled != state_.blend_state->enabled) {
    SetGlBlendEnabled(state);
    update = true;
  }

  if (!state_.blend_state || state.src_alpha != state_.blend_state->src_alpha ||
      state.src_color != state_.blend_state->src_color ||
      state.dst_alpha != state_.blend_state->dst_alpha ||
      state.dst_color != state_.blend_state->dst_color) {
    SetGlBlendFunc(state);
    update = true;
  }

  if (update) {
    state_.blend_state = state;
  }
}

static void SetGlColorMask(const ColorStateT& state) {
  const GLboolean r = state.write_red ? GL_TRUE : GL_FALSE;
  const GLboolean g = state.write_green ? GL_TRUE : GL_FALSE;
  const GLboolean b = state.write_blue ? GL_TRUE : GL_FALSE;
  const GLboolean a = state.write_alpha ? GL_TRUE : GL_FALSE;
  GL_CALL(glColorMask(r, g, b, a));
}

void RenderStateManager::SetColorState(const ColorStateT& state) {
  bool update = false;

  if (!state_.color_state || state.write_red != state_.color_state->write_red ||
      state.write_green != state_.color_state->write_green ||
      state.write_blue != state_.color_state->write_blue ||
      state.write_alpha != state_.color_state->write_alpha) {
    SetGlColorMask(state);
    update = true;
  }

  if (update) {
    state_.color_state = state;
  }
}

static void SetGlCullEnabled(const CullStateT& state) {
  if (state.enabled) {
    GL_CALL(glEnable(GL_CULL_FACE));
  } else {
    GL_CALL(glDisable(GL_CULL_FACE));
  }
}

static void SetGlCullFace(const CullStateT& state) {
  const GLenum cull_face = GetGlCullFace(state.face);
  GL_CALL(glCullFace(cull_face));
}

static void SetGlFrontFace(const CullStateT& state) {
  GL_CALL(glFrontFace(GetGlFrontFace(state.front)));
}

void RenderStateManager::SetCullState(const CullStateT& state) {
  bool update = false;

  if (!state_.cull_state || state.enabled != state_.cull_state->enabled) {
    SetGlCullEnabled(state);
    update = true;
  }

  if (!state_.cull_state || state.face != state_.cull_state->face) {
    SetGlCullFace(state);
    update = true;
  }

  if (!state_.cull_state || state.front != state_.cull_state->front) {
    SetGlFrontFace(state);
    update = true;
  }

  if (update) {
    state_.cull_state = state;
  }
}

static void SetGlDepthTestEnabled(const DepthStateT& state) {
#if !defined(NDEBUG) && !defined(__ANDROID__)
  static bool check_once = false;
  if (!check_once) {
    // GL_DEPTH_BITS was deprecated in desktop GL 3.3, so make sure this get
    // succeeds before checking depth_bits.
    GLint depth_bits = 0;
    glGetIntegerv(GL_DEPTH_BITS, &depth_bits);
    if (glGetError() == 0 && depth_bits == 0) {
      LOG(WARNING) << "Enabling depth test without a depth buffer; this has "
                      "known issues on some platforms.";
    }
    check_once = true;
  }
#endif  // !NDEBUG

  if (state.test_enabled) {
    GL_CALL(glEnable(GL_DEPTH_TEST));
  } else {
    GL_CALL(glDisable(GL_DEPTH_TEST));
  }
}

static void SetGlDepthWriteEnabled(const DepthStateT& state) {
  GL_CALL(glDepthMask(state.write_enabled ? GL_TRUE : GL_FALSE));
}

static void SetGlDepthFunction(const DepthStateT& state) {
  const GLenum func = GetGlRenderFunction(state.function);
  GL_CALL(glDepthFunc(func));
}

void RenderStateManager::SetDepthState(const DepthStateT& state) {
  bool update = false;

  if (!state_.depth_state ||
      state.test_enabled != state_.depth_state->test_enabled) {
    SetGlDepthTestEnabled(state);
    update = true;
  }

  if (!state_.depth_state ||
      state.write_enabled != state_.depth_state->write_enabled) {
    SetGlDepthWriteEnabled(state);
    update = true;
  }

  if (!state_.depth_state || state.function != state_.depth_state->function) {
    SetGlDepthFunction(state);
    update = true;
  }

  if (update) {
    state_.depth_state = state;
  }
}

static void SetGlPointSpriteEnabled(const PointStateT& state) {
#if !defined(GL_CORE_PROFILE) && \
    !defined(FPLBASE_GLES) && \
     defined(GL_POINT_SPRITE)
  if (state.point_sprite_enabled) {
    GL_CALL(glEnable(GL_POINT_SPRITE));
  } else {
    GL_CALL(glDisable(GL_POINT_SPRITE));
  }
#endif
}

static void SetGlPointSizeEnabled(const PointStateT& state) {
#if !defined(FPLBASE_GLES) && defined(GL_PROGRAM_POINT_SIZE)
  if (state.program_point_size_enabled) {
    GL_CALL(glEnable(GL_PROGRAM_POINT_SIZE));
  } else {
    GL_CALL(glDisable(GL_PROGRAM_POINT_SIZE));
  }
#elif !defined(FPLBASE_GLES) && defined(GL_VERTEX_PROGRAM_POINT_SIZE)
  if (state.program_point_size_enabled) {
    GL_CALL(glEnable(GL_VERTEX_PROGRAM_POINT_SIZE));
  } else {
    GL_CALL(glDisable(GL_VERTEX_PROGRAM_POINT_SIZE));
  }
#endif
}

void SetGlPointSize(const PointStateT& state) {
#if !defined(FPLBASE_GLES)
  if (state.point_size > 0) {
    GL_CALL(glPointSize(state.point_size));
  }
#endif
}

void RenderStateManager::SetPointState(const PointStateT& state) {
  bool update = false;

  if (!state_.point_state ||
      state.point_sprite_enabled != state_.point_state->point_sprite_enabled) {
    SetGlPointSpriteEnabled(state);
    update = true;
  }

  if (!state_.point_state ||
      state.program_point_size_enabled !=
          state_.point_state->program_point_size_enabled) {
    SetGlPointSizeEnabled(state);
    update = true;
  }

  if (!state_.point_state ||
      state.point_size != state_.point_state->point_size) {
    SetGlPointSize(state);
    update = true;
  }

  if (update) {
    state_.point_state = state;
  }
}

static void SetGlScissorEnabled(const ScissorStateT& state) {
  if (state.enabled) {
    GL_CALL(glEnable(GL_SCISSOR_TEST));
  } else {
    GL_CALL(glDisable(GL_SCISSOR_TEST));
  }
}

void RenderStateManager::SetScissorState(const ScissorStateT& state) {
  bool update = false;

  if (!state_.scissor_state || state.enabled != state_.scissor_state->enabled) {
    SetGlScissorEnabled(state);
    update = true;
  }

  if (!state_.scissor_state ||
      (state.enabled && state.rect != state_.scissor_state->rect)) {
    GL_CALL(glScissor(state.rect->pos.x, state.rect->pos.y, state.rect->size.x,
                      state.rect->size.y));
    update = true;
  }

  if (update) {
    state_.scissor_state = state;
  }
}

static void SetGlStencilTestEnabled(const StencilStateT& state) {
  if (state.enabled) {
    GL_CALL(glEnable(GL_STENCIL_TEST));
  } else {
    GL_CALL(glDisable(GL_STENCIL_TEST));
  }
}

bool operator!=(const StencilFunctionT& lhs, const StencilFunctionT& rhs) {
  return lhs.function != rhs.function || lhs.mask != rhs.mask ||
         lhs.ref != rhs.ref;
}

bool operator!=(const StencilOperationT& lhs, const StencilOperationT& rhs) {
  return lhs.depth_fail != rhs.depth_fail ||
         lhs.stencil_fail != rhs.stencil_fail || lhs.pass != rhs.pass;
}

static void SetGlStencilFunction(GLenum face, const StencilFunctionT& func) {
  const GLenum gl_func = GetGlRenderFunction(func.function);
  GL_CALL(glStencilFuncSeparate(face, gl_func, func.ref, func.mask));
}

static void SetGlStencilOperation(GLenum face, const StencilOperationT& op) {
  const GLenum stencil_fail = GetGlStencilAction(op.stencil_fail);
  const GLenum depth_fail = GetGlStencilAction(op.depth_fail);
  const GLenum pass = GetGlStencilAction(op.pass);
  GL_CALL(glStencilOpSeparate(face, stencil_fail, depth_fail, pass));
}

void RenderStateManager::SetStencilState(const StencilStateT& state) {
  bool update = false;

  if (!state_.stencil_state || state.enabled != state_.stencil_state->enabled) {
    SetGlStencilTestEnabled(state);
    update = true;
  }

  if (!state_.stencil_state ||
      state.back_function != state_.stencil_state->back_function) {
    SetGlStencilFunction(GL_BACK, state.back_function);
    update = true;
  }

  if (!state_.stencil_state ||
      state.front_function != state_.stencil_state->front_function) {
    SetGlStencilFunction(GL_FRONT, state.front_function);
    update = true;
  }

  if (!state_.stencil_state ||
      state.front_op != state_.stencil_state->front_op) {
    SetGlStencilOperation(GL_FRONT, state.front_op);
    update = true;
  }

  if (!state_.stencil_state || state.back_op != state_.stencil_state->back_op) {
    SetGlStencilOperation(GL_FRONT, state.back_op);
    update = true;
  }

  if (update) {
    state_.stencil_state = state;
  }
}

static void SetGlViewport(const mathfu::recti& rect) {
  GL_CALL(glViewport(rect.pos.x, rect.pos.y, rect.size.x, rect.size.y));
}

void RenderStateManager::SetViewport(const mathfu::recti& rect) {
  bool update = false;

  if (!state_.viewport || rect != state_.viewport.value()) {
    if (rect.size.x <= 0 || rect.size.y <= 0) {
      return;
    }
    SetGlViewport(rect);
    update = true;
  }

  if (update) {
    state_.viewport = rect;
  }
}

}  // namespace lull
