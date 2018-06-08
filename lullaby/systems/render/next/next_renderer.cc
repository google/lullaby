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

#include "lullaby/systems/render/next/next_renderer.h"

#include <atomic>
#ifdef __APPLE__
#include "fplbase/internal/renderer_ios.h"
#endif
#include "lullaby/systems/render/next/detail/glplatform.h"
#include "lullaby/systems/render/next/gl_helpers.h"
#include "lullaby/systems/render/next/texture.h"
#include "lullaby/util/hash.h"
#include "lullaby/util/make_unique.h"

#include <array>

#define LULLABY_GL_FUNCTION(type, name, required, lookup_fn)      \
  union {                                                         \
    void* data;                                                   \
    type function;                                                \
  } data_function_union_##name;                                   \
  data_function_union_##name.data = (void*)lookup_fn(#name);      \
  if (required && !data_function_union_##name.data) {             \
    LOG(ERROR) << "Could not find GL function pointer " << #name; \
  }                                                               \
  name = data_function_union_##name.function;

#ifdef _WIN32
WINGDIAPI PROC WINAPI wglGetProcAddress(LPCSTR);
#endif

namespace lull {

constexpr HashValue kEnvironmentHashMultiview = ConstHash("MULTIVIEW");

struct ContextCapabilities {
  ContextCapabilities()
      : feature_level_3(false),
        supports_samplers(false),
        supports_multiview(false),
        supports_vertex_arrays(false),
        supports_npot_textures(false),
        supports_astc_textures(false),
        supports_etc2_textures(false) {}

  std::atomic<bool> feature_level_3;
  std::atomic<bool> supports_samplers;
  std::atomic<bool> supports_multiview;
  std::atomic<bool> supports_vertex_arrays;
  std::atomic<bool> supports_npot_textures;
  std::atomic<bool> supports_astc_textures;
  std::atomic<bool> supports_etc2_textures;

#ifdef FPLBASE_GLES
  static const bool is_gles = true;
#else
  static const bool is_gles = false;
#endif
};

ContextCapabilities gContextCapabilities;

static std::set<std::string> GetExtensions() {
  std::set<std::string> extensions;

  auto res = glGetString(GL_EXTENSIONS);
  if (glGetError() == GL_NO_ERROR && res != nullptr) {
    std::stringstream ss(reinterpret_cast<const char*>(res));
    std::string ext;
    while (std::getline(ss, ext, ' ')) {
      extensions.insert(ext);
    }
    return extensions;
  }

#if GL_ES_VERSION_3_0 || defined(GL_VERSION_3_0)
  int num_extensions = 0;
  glGetIntegerv(GL_NUM_EXTENSIONS, &num_extensions);
  if (glGetError() == GL_NO_ERROR) {
    for (int i = 0; i < num_extensions; ++i) {
      res = glGetStringi(GL_EXTENSIONS, i);
      if (res != nullptr) {
        extensions.insert(reinterpret_cast<const char*>(res));
      }
    }
  }
#endif  // defined(GL_NUM_EXTENSIONS)
  return extensions;
}

template<class T, size_t N>
static constexpr size_t ArraySize(T (&)[N]) { return N; }

NextRenderer::NextRenderer() {
#if defined(_WIN32) && !defined(FPLBASE_GLES)
#define GLEXT(type, name, required) \
  LULLABY_GL_FUNCTION(type, name, required, wglGetProcAddress)
  GLBASEEXTS GLEXTS
#undef GLEXT
#endif

#ifndef PLATFORM_MOBILE
#ifdef GL_MAJOR_VERSION
      GLint version = 0;
  glGetIntegerv(GL_MAJOR_VERSION, &version);
  if (glGetError() == 0) {
    if (version >= 3) {
      gContextCapabilities.feature_level_3 = true;
      gContextCapabilities.supports_samplers = true;
      gContextCapabilities.supports_vertex_arrays = true;
    }
  }
#endif  // defined(GL_MAJOR_VERSION)
#endif  // !defined(PLATFORM_MOBILE)

#ifdef __ANDROID__
  EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
  CHECK_NE(display, EGL_NO_DISPLAY) << "Display is not available.";
  EGLContext context = eglGetCurrentContext();
  CHECK_NE(context, EGL_NO_CONTEXT) << "Context is not available.";
  EGLint version = 0;
  eglQueryContext(display, context, EGL_CONTEXT_CLIENT_VERSION, &version);
  if (version >= 3) {
    gContextCapabilities.feature_level_3 = true;
    gContextCapabilities.supports_samplers = true;
    gContextCapabilities.supports_vertex_arrays = true;
#if __ANDROID_API__ < 18
    gl3stubInit();
#endif
  }
#endif  // defined(__ANDROID__)

#if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
  const int version = fplbase::IosGetContextClientVersion();
  assert(version >= 2);
  if (version >= 3) {
    gContextCapabilities.feature_level_3 = true;
    gContextCapabilities.supports_samplers = true;
    gContextCapabilities.supports_vertex_arrays = true;
  }
#endif  // TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR

  const std::set<std::string> extensions = GetExtensions();
  for (const std::string& ext : extensions) {
    environment_flags_.insert(Hash(ext));
  }

#ifndef FPLBASE_GLES
  if (!extensions.count("GL_ARB_vertex_buffer_object") ||
      !extensions.count("GL_ARB_multitexture") ||
      !extensions.count("GL_ARB_vertex_program") ||
      !extensions.count("GL_ARB_fragment_program")) {
    LOG(ERROR) << "Missing required extensions";
  }
#endif

  // Check for multiview extension support.
  if (extensions.count("GL_OVR_multiview") ||
      extensions.count("GL_OVR_multiview2")) {
    gContextCapabilities.supports_multiview = true;
  }

  // Check for ASTC.
#if defined(GL_COMPRESSED_RGBA_ASTC_4x4_KHR) and \
    defined(GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR)
  // If we have the ASTC type enums defined, check using them.
  gContextCapabilities.supports_astc_textures = true;

  int num_formats = 0;
  GL_CALL(glGetIntegerv(GL_NUM_COMPRESSED_TEXTURE_FORMATS, &num_formats));
  CHECK_LE(num_formats, 256);
  std::array<int, 256> supported_formats;
  GL_CALL(
      glGetIntegerv(GL_COMPRESSED_TEXTURE_FORMATS, supported_formats.data()));

  const int astc_formats[] = {
      GL_COMPRESSED_RGBA_ASTC_4x4_KHR,
      GL_COMPRESSED_RGBA_ASTC_5x4_KHR,
      GL_COMPRESSED_RGBA_ASTC_5x5_KHR,
      GL_COMPRESSED_RGBA_ASTC_6x5_KHR,
      GL_COMPRESSED_RGBA_ASTC_6x6_KHR,
      GL_COMPRESSED_RGBA_ASTC_8x5_KHR,
      GL_COMPRESSED_RGBA_ASTC_8x6_KHR,
      GL_COMPRESSED_RGBA_ASTC_8x8_KHR,
      GL_COMPRESSED_RGBA_ASTC_10x5_KHR,
      GL_COMPRESSED_RGBA_ASTC_10x6_KHR,
      GL_COMPRESSED_RGBA_ASTC_10x8_KHR,
      GL_COMPRESSED_RGBA_ASTC_10x10_KHR,
      GL_COMPRESSED_RGBA_ASTC_12x10_KHR,
      GL_COMPRESSED_RGBA_ASTC_12x12_KHR,
      GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR,
      GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4_KHR,
      GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5_KHR,
      GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5_KHR,
      GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6_KHR,
      GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x5_KHR,
      GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x6_KHR,
      GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8_KHR,
      GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x5_KHR,
      GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x6_KHR,
      GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x8_KHR,
      GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x10_KHR,
      GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x10_KHR,
      GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x12_KHR};
  for (int ii = 0; ii < ArraySize(astc_formats); ++ii) {
    int astc_format = astc_formats[ii];
    auto itr = std::find(supported_formats.begin(), supported_formats.end(),
                         astc_format);
    if (itr == supported_formats.end()) {
      gContextCapabilities.supports_astc_textures = false;
      break;
    }
  }
#else
  // Without the ASTC type enums, check for the GL extension.
  if (extensions.count("GL_KHR_texture_compression_astc_ldr")) {
    gContextCapabilities.supports_astc_textures = true;
  }
#endif

#ifdef __ANDROID__
  // Check for Non Power of 2 (NPOT) extension.
  if (extensions.count("GL_ARB_texture_non_power_of_two") ||
      extensions.count("GL_OES_texture_npot")) {
    gContextCapabilities.supports_npot_textures = true;
  }
#else
  // All desktop platforms support NPOT.
  // iOS ES 2 is supposed to only have limited support, but in practice always
  // supports it.
  gContextCapabilities.supports_npot_textures = true;
#endif

  // Check for ETC2:
  // TODO(b/80516463): GLES3/GL_ARB_ES3_compatibility implies ETC2 support, but
  // is not required for it.  The ETC2 formats may also be individually queried
  // via the OES_compressed_ETC2_* extension strings.
#ifdef FPLBASE_GLES
  if (gContextCapabilities.feature_level_3) {
#else
  if (extensions.count("GL_ARB_ES3_compatibility")) {
#endif
    gContextCapabilities.supports_etc2_textures = true;
  }

  glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &max_texture_units_);
}

NextRenderer::~NextRenderer() {}

RenderStateManager& NextRenderer::GetRenderStateManager() {
  return render_state_manager_;
}

const RenderStateManager& NextRenderer::GetRenderStateManager() const {
  return render_state_manager_;
}

void NextRenderer::ResetGpuState() {
  // Clear VAOs.
  if (SupportsVertexArrays()) {
#if GL_ES_VERSION_3_0 || defined(GL_VERSION_3_0)
    GL_CALL(glBindVertexArray(0));
#endif
  } else {
// VAOs were available prior to GLES3 using an extension.
#if GL_OES_vertex_array_object
#ifndef GL_GLEXT_PROTOTYPES
    static PFNGLBINDVERTEXARRAYOESPROC glBindVertexArrayOES = []() {
      return (PFNGLBINDVERTEXARRAYOESPROC)eglGetProcAddress(
          "glBindVertexArrayOES");
    };
    if (glBindVertexArrayOES) {
      GL_CALL(glBindVertexArrayOES(0));
    }
#else   // GL_GLEXT_PROTOTYPES
    GL_CALL(glBindVertexArrayOES(0));
#endif  // !GL_GLEXT_PROTOTYPES
#endif  // GL_OES_vertex_array_object
  }

  // Clear samplers.
  if (SupportsSamplers()) {
    // HACKHACK
    int max_texture_unit_ = 8;

// Samplers are part of GLES3 & GL3.3 specs.
#if GL_ES_VERSION_3_0 || defined(GL_VERSION_3_3)
    for (int i = 0; i <= max_texture_unit_; ++i) {
      // Confusingly, glBindSampler takes an index, not the raw texture unit
      // (GL_TEXTURE0 + index).
      GL_CALL(glBindSampler(i, 0));
    }
#endif  // GL_ES_VERSION_3_0 || GL_VERSION_3_3
  }
}

bool NextRenderer::IsGles() { return gContextCapabilities.is_gles; }

bool NextRenderer::SupportsVertexArrays() {
  return gContextCapabilities.supports_vertex_arrays;
}

bool NextRenderer::SupportsTextureNpot() {
  return gContextCapabilities.supports_npot_textures;
}

bool NextRenderer::SupportsSamplers() {
  return gContextCapabilities.supports_samplers;
}

bool NextRenderer::SupportsAstc() {
  return gContextCapabilities.supports_astc_textures;
}

bool NextRenderer::SupportsEtc2() {
  return gContextCapabilities.supports_etc2_textures;
}

void NextRenderer::EnableMultiview() {
  multiview_enabled_ = true;
  environment_flags_.insert(kEnvironmentHashMultiview);
}

void NextRenderer::DisableMultiview() {
  multiview_enabled_ = false;
  environment_flags_.erase(kEnvironmentHashMultiview);
}

bool NextRenderer::IsMultiviewEnabled() const { return multiview_enabled_; }

const std::set<HashValue>& NextRenderer::GetEnvironmentFlags() const {
  return environment_flags_;
}

void NextRenderer::Begin(RenderTarget* render_target) {
#ifdef LULLABY_VERIFY_GPU_STATE
  render_state_manager_.Validate();
#endif

  if (render_target) {
    render_target_ = render_target;
    render_target_->Bind();
  }
}

void NextRenderer::End() {
  if (render_target_) {
    render_target_->Unbind();
    render_target_ = nullptr;
  }
  GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, 0));
  GL_CALL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
}

void NextRenderer::Clear(const RenderClearParams& clear_params) {
  GLbitfield gl_clear_mask = 0;
  if (CheckBit(clear_params.clear_options, RenderClearParams::kColor)) {
    gl_clear_mask |= GL_COLOR_BUFFER_BIT;
    // Ensure all colors are writable before clearing the color buffer.
    ColorStateT color;
    color.write_red = true;
    color.write_green = true;
    color.write_blue = true;
    color.write_alpha = true;
    render_state_manager_.SetColorState(color);
    GL_CALL(glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE));
    GL_CALL(glClearColor(clear_params.color_value.x, clear_params.color_value.y,
                         clear_params.color_value.z,
                         clear_params.color_value.w));
  }

  if (CheckBit(clear_params.clear_options, RenderClearParams::kDepth)) {
    gl_clear_mask |= GL_DEPTH_BUFFER_BIT;
    // Ensure depth buffer is writable before clearing it.
    DepthStateT depth;
    depth.write_enabled = true;
    render_state_manager_.SetDepthState(depth);
#ifdef FPLBASE_GLES
    GL_CALL(glClearDepthf(clear_params.depth_value));
#else
    GL_CALL(glClearDepth(static_cast<double>(clear_params.depth_value)));
#endif
  }

  if (CheckBit(clear_params.clear_options, RenderClearParams::kStencil)) {
    gl_clear_mask |= GL_STENCIL_BUFFER_BIT;
    // Ensure all stencils are writable before clearing the stencil buffer.
    StencilStateT stencil;
    stencil.front_function.mask = ~0u;
    stencil.back_function.mask = ~0u;
    render_state_manager_.SetStencilState(stencil);
    GL_CALL(glClearStencil(clear_params.stencil_value));
  }
  GL_CALL(glClear(gl_clear_mask));
}

void NextRenderer::ApplyMaterial(const std::shared_ptr<Material>& material) {
  if (!material) {
    return;
  }

  material->Bind(max_texture_units_);

  if (material->GetBlendState()) {
    render_state_manager_.SetBlendState(*material->GetBlendState());
  }
  if (material->GetCullState()) {
    render_state_manager_.SetCullState(*material->GetCullState());
  }
  if (material->GetDepthState()) {
    render_state_manager_.SetDepthState(*material->GetDepthState());
  }
  if (material->GetPointState()) {
    render_state_manager_.SetPointState(*material->GetPointState());
  }
  if (material->GetStencilState()) {
    render_state_manager_.SetStencilState(*material->GetStencilState());
  }
}

void NextRenderer::Draw(const MeshPtr& mesh,
                        const mathfu::mat4& world_from_object,
                        int submesh_index) {
  if (submesh_index >= 0) {
    mesh->RenderSubmesh(submesh_index);
  } else {
    mesh->Render();
  }
}

}  // namespace lull
