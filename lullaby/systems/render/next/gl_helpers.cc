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

#include "lullaby/systems/render/next/gl_helpers.h"

#include "lullaby/systems/render/next/next_renderer.h"

namespace lull {
// Predefined attributes supported in shaders.
enum {
  kAttribPosition,
  kAttribNormal,
  kAttribTangent,
  kAttribOrientation,
  kAttribTexCoord,
  kAttribTexCoord1,
  kAttribTexCoord2,
  kAttribTexCoord3,
  kAttribTexCoord4,
  kAttribTexCoord5,
  kAttribTexCoord6,
  kAttribTexCoord7,
  kAttribColor,
  kAttribBoneIndices,
  kAttribBoneWeights,
  kAttribInvalid,

  kAttribTexCoordMax = kAttribTexCoord7,
};

GLenum GetGlInternalFormat(DepthStencilFormat format) {
  switch (format) {
    case DepthStencilFormat_None:
      return GL_NONE;
    case DepthStencilFormat_Depth16:
      return GL_DEPTH_COMPONENT16;
    case DepthStencilFormat_Depth24:
      return GL_DEPTH_COMPONENT24;
    case DepthStencilFormat_Depth32F:
      return GL_DEPTH_COMPONENT32F;
    case DepthStencilFormat_Depth24Stencil8:
      return GL_DEPTH24_STENCIL8;
    case DepthStencilFormat_Depth32FStencil8:
      return GL_DEPTH32F_STENCIL8;
    case DepthStencilFormat_Stencil8:
      return GL_STENCIL_INDEX8;
    default:
      LOG(DFATAL) << "Unknown depth stencil format: " << format;
      return GL_NONE;
  }
}

GLenum GetGlInternalFormat(TextureFormat format) {
  switch (format) {
    case TextureFormat_None:
      return GL_NONE;
    case TextureFormat_A8:
      return GL_ALPHA;
    case TextureFormat_R8:
      return GL_RGB;
    case TextureFormat_RGB8:
      return GL_RGB;
    case TextureFormat_RGBA8:
      return GL_RGBA;
    case TextureFormat_Depth16:
      return GL_DEPTH_COMPONENT16;
    case TextureFormat_Depth32F:
      return GL_DEPTH_COMPONENT32F;
    default:
      LOG(DFATAL) << "Unknown texture format: " << format;
      return GL_NONE;
  }
}

GLenum GetGlFormat(TextureFormat format) {
  switch (format) {
    case TextureFormat_None:
      return GL_NONE;
    case TextureFormat_A8:
      return GL_ALPHA;
    case TextureFormat_R8:
      // For GLES2, the format must match internalformat.
      return NextRenderer::IsGles() ? GL_RGB : GL_RED;
    case TextureFormat_RGB8:
      return GL_RGB;
    case TextureFormat_RGBA8:
      return GL_RGBA;
    case TextureFormat_Depth16:
      return GL_DEPTH_COMPONENT;
    case TextureFormat_Depth32F:
      return GL_DEPTH_COMPONENT;
    default:
      LOG(DFATAL) << "Unknown format: " << format;
      return GL_NONE;
  }
}

GLenum GetGlType(TextureFormat format) {
  switch (format) {
    case TextureFormat_None:
      return GL_NONE;
    case TextureFormat_A8:
      return GL_UNSIGNED_BYTE;
    case TextureFormat_R8:
      return GL_UNSIGNED_BYTE;
    case TextureFormat_RGB8:
      return GL_UNSIGNED_BYTE;
    case TextureFormat_RGBA8:
      return GL_UNSIGNED_BYTE;
    case TextureFormat_Depth16:
      return GL_UNSIGNED_SHORT;
    case TextureFormat_Depth32F:
      return GL_FLOAT;
    default:
      LOG(DFATAL) << "Unknown texture type: " << format;
      return GL_NONE;
  }
}

GLenum GetGlTextureFiltering(TextureFiltering filtering) {
  switch (filtering) {
    case TextureFiltering_Nearest:
      return GL_NEAREST;
    case TextureFiltering_Linear:
      return GL_LINEAR;
    case TextureFiltering_NearestMipmapNearest:
      return GL_NEAREST_MIPMAP_NEAREST;
    case TextureFiltering_LinearMipmapNearest:
      return GL_LINEAR_MIPMAP_NEAREST;
    case TextureFiltering_NearestMipmapLinear:
      return GL_NEAREST_MIPMAP_LINEAR;
    case TextureFiltering_LinearMipmapLinear:
      return GL_LINEAR_MIPMAP_LINEAR;
    default:
      LOG(DFATAL) << "Unknown texture filtering: " << filtering;
      return GL_NEAREST;
  }
}

GLenum GetGlTextureWrap(TextureWrap wrap) {
  switch (wrap) {
    case TextureWrap_ClampToBorder:
#ifdef GL_CLAMP_TO_BORDER
      return GL_CLAMP_TO_BORDER;
#else
      LOG(ERROR) << "TextureWrap_ClampToBorder is not supported.";
      return GL_CLAMP_TO_EDGE;
#endif
    case TextureWrap_ClampToEdge:
      return GL_CLAMP_TO_EDGE;
    case TextureWrap_MirroredRepeat:
      return GL_MIRRORED_REPEAT;
    case TextureWrap_MirrorClampToEdge:
      LOG(ERROR) << "TextureWrap_MirrorClampToEdge is not supported.";
      return GL_CLAMP_TO_EDGE;
    case TextureWrap_Repeat:
      return GL_REPEAT;
    default:
      LOG(DFATAL) << "Unknown texture wrap mode: " << wrap;
      return GL_REPEAT;
  }
}

GLenum GetGlPrimitiveType(MeshData::PrimitiveType type) {
  switch (type) {
    case MeshData::kPoints:
      return GL_POINTS;
    case MeshData::kLines:
      return GL_LINES;
    case MeshData::kTriangles:
      return GL_TRIANGLES;
    case MeshData::kTriangleFan:
      return GL_TRIANGLE_FAN;
    case MeshData::kTriangleStrip:
      return GL_TRIANGLE_STRIP;
    default:
      LOG(DFATAL) << "Unknown index type: " << type;
      return GL_UNSIGNED_BYTE;
  }
}

GLenum GetGlVertexType(VertexAttributeType type) {
  switch (type) {
    case VertexAttributeType_Scalar1f:
    case VertexAttributeType_Vec2f:
    case VertexAttributeType_Vec3f:
    case VertexAttributeType_Vec4f:
      return GL_FLOAT;
    case VertexAttributeType_Vec2us:
    case VertexAttributeType_Vec4us:
      return GL_UNSIGNED_SHORT;
    case VertexAttributeType_Vec4ub:
      return GL_UNSIGNED_BYTE;
    default:
      LOG(DFATAL) << "Unknown vertex attribute type.";
      return GL_UNSIGNED_BYTE;
  }
}

GLint GetNumElementsInVertexType(VertexAttributeType type) {
  switch (type) {
    case VertexAttributeType_Scalar1f:
      return 1;
    case VertexAttributeType_Vec2f:
      return 2;
    case VertexAttributeType_Vec3f:
      return 3;
    case VertexAttributeType_Vec4f:
      return 4;
    case VertexAttributeType_Vec2us:
      return 2;
    case VertexAttributeType_Vec4us:
      return 4;
    case VertexAttributeType_Vec4ub:
      return 4;
    default:
      LOG(DFATAL) << "Unknown vertex attribute type.";
      return 0;
  }
}

GLenum GetGlIndexType(MeshData::IndexType type) {
  switch (type) {
    case MeshData::kIndexU16:
      return GL_UNSIGNED_SHORT;
    case MeshData::kIndexU32:
      return GL_UNSIGNED_INT;
    default:
      LOG(DFATAL) << "Unknown index type: " << type;
      return GL_UNSIGNED_BYTE;
  }
}

GLuint GetGlRenderFunction(fplbase::RenderFunction func) {
  switch (func) {
    case fplbase::kRenderAlways:
      return GL_ALWAYS;
    case fplbase::kRenderEqual:
      return GL_EQUAL;
    case fplbase::kRenderGreater:
      return GL_GREATER;
    case fplbase::kRenderGreaterEqual:
      return GL_GEQUAL;
    case fplbase::kRenderLess:
      return GL_LESS;
    case fplbase::kRenderLessEqual:
      return GL_LEQUAL;
    case fplbase::kRenderNever:
      return GL_NEVER;
    case fplbase::kRenderNotEqual:
      return GL_NOTEQUAL;
    default:
      LOG(DFATAL) << "Unknown function type: " << func;
      return GL_ALWAYS;
  }
}

GLuint GetGlBlendStateFactor(fplbase::BlendState::BlendFactor factor) {
  switch (factor) {
    case fplbase::BlendState::kZero:
      return GL_ZERO;
    case fplbase::BlendState::kOne:
      return GL_ONE;
    case fplbase::BlendState::kSrcColor:
      return GL_SRC_COLOR;
    case fplbase::BlendState::kOneMinusSrcColor:
      return GL_ONE_MINUS_SRC_COLOR;
    case fplbase::BlendState::kDstColor:
      return GL_DST_COLOR;
    case fplbase::BlendState::kOneMinusDstColor:
      return GL_ONE_MINUS_DST_COLOR;
    case fplbase::BlendState::kSrcAlpha:
      return GL_SRC_ALPHA;
    case fplbase::BlendState::kOneMinusSrcAlpha:
      return GL_ONE_MINUS_SRC_ALPHA;
    case fplbase::BlendState::kDstAlpha:
      return GL_DST_ALPHA;
    case fplbase::BlendState::kOneMinusDstAlpha:
      return GL_ONE_MINUS_DST_ALPHA;
    case fplbase::BlendState::kConstantColor:
      return GL_CONSTANT_COLOR;
    case fplbase::BlendState::kOneMinusConstantColor:
      return GL_ONE_MINUS_CONSTANT_COLOR;
    case fplbase::BlendState::kConstantAlpha:
      return GL_CONSTANT_ALPHA;
    case fplbase::BlendState::kOneMinusConstantAlpha:
      return GL_ONE_MINUS_CONSTANT_ALPHA;
    case fplbase::BlendState::kSrcAlphaSaturate:
      return GL_SRC_ALPHA_SATURATE;
    default:
      LOG(DFATAL) << "Unknown factor: " << factor;
      return GL_ZERO;
  }
}

GLuint GetGlCullFace(fplbase::CullState::CullFace face) {
  switch (face) {
    case fplbase::CullState::kFront:
      return GL_FRONT;
    case fplbase::CullState::kBack:
      return GL_BACK;
    case fplbase::CullState::kFrontAndBack:
      return GL_FRONT_AND_BACK;
    default:
      LOG(DFATAL) << "Unknown cull face: " << face;
      return GL_FRONT;
  }
}

GLuint GetGlFrontFace(fplbase::CullState::FrontFace front_face) {
  switch (front_face) {
    case fplbase::CullState::kClockWise:
      return GL_CW;
    case fplbase::CullState::kCounterClockWise:
      return GL_CCW;
    default:
      LOG(DFATAL) << "Unknown front face: " << front_face;
      return GL_CW;
  }
}

GLuint GetGlStencilOp(fplbase::StencilOperation::StencilOperations op) {
  switch (op) {
    case fplbase::StencilOperation::kKeep:
      return GL_KEEP;
    case fplbase::StencilOperation::kZero:
      return GL_ZERO;
    case fplbase::StencilOperation::kReplace:
      return GL_REPLACE;
    case fplbase::StencilOperation::kIncrement:
      return GL_INCR;
    case fplbase::StencilOperation::kIncrementAndWrap:
      return GL_INCR_WRAP;
    case fplbase::StencilOperation::kDecrement:
      return GL_DECR;
    case fplbase::StencilOperation::kDecrementAndWrap:
      return GL_DECR_WRAP;
    case fplbase::StencilOperation::kInvert:
      return GL_INVERT;
    default:
      LOG(DFATAL) << "Unknown stencil op: " << op;
      return GL_KEEP;
  }
}

ShaderLanguage GetShaderLanguage() {
  return NextRenderer::IsGles() ? ShaderLanguage_GLSL_ES : ShaderLanguage_GLSL;
}

bool GlSupportsVertexArrays() { return NextRenderer::SupportsVertexArrays(); }

bool GlSupportsTextureNpot() { return NextRenderer::SupportsTextureNpot(); }

bool GlSupportsAstc() { return NextRenderer::SupportsAstc(); }

bool GlSupportsEtc2() { return NextRenderer::SupportsEtc2(); }

void SetVertexAttributes(const VertexFormat& vertex_format,
                         const uint8_t* buffer) {
  int tex_coord_count = 0;
  const int stride = static_cast<int>(vertex_format.GetVertexSize());
  for (size_t i = 0; i < vertex_format.GetNumAttributes(); ++i) {
    const VertexAttribute* attrib = vertex_format.GetAttributeAt(i);

    const GLenum gl_type = GetGlVertexType(attrib->type());
    const GLint count = GetNumElementsInVertexType(attrib->type());
    if (count == 0) {
      continue;
    }

    int location = kAttribInvalid;
    bool normalized = false;
    switch (attrib->usage()) {
      case VertexAttributeUsage_Position:
        location = kAttribPosition;
        break;
      case VertexAttributeUsage_Normal:
        location = kAttribNormal;
        break;
      case VertexAttributeUsage_Tangent:
        location = kAttribTangent;
        break;
      case VertexAttributeUsage_Orientation:
        location = kAttribOrientation;
        break;
      case VertexAttributeUsage_Color:
        location = kAttribColor;
        normalized = true;
        break;
      case VertexAttributeUsage_BoneIndices:
        location = kAttribBoneIndices;
        break;
      case VertexAttributeUsage_BoneWeights:
        location = kAttribBoneWeights;
        normalized = true;
        break;
      case VertexAttributeUsage_TexCoord:
        DCHECK(kAttribTexCoord + tex_coord_count < kAttribTexCoordMax);
        location = kAttribTexCoord + tex_coord_count;
        normalized = (gl_type != GL_FLOAT);
        ++tex_coord_count;
        break;
      default:
        break;
    }

    if (location != kAttribInvalid) {
      GL_CALL(glEnableVertexAttribArray(location));
      GL_CALL(glVertexAttribPointer(location, count, gl_type,
                                    normalized ? GL_TRUE : GL_FALSE, stride,
                                    buffer));
    }
    buffer += VertexFormat::GetAttributeSize(*attrib);
  }
}

void UnsetVertexAttributes(const VertexFormat& vertex_format) {
  int tex_coord_count = 0;
  for (size_t i = 0; i < vertex_format.GetNumAttributes(); ++i) {
    const VertexAttribute* attrib = vertex_format.GetAttributeAt(i);
    switch (attrib->usage()) {
      case VertexAttributeUsage_Position:
        GL_CALL(glDisableVertexAttribArray(kAttribPosition));
        break;
      case VertexAttributeUsage_Normal:
        GL_CALL(glDisableVertexAttribArray(kAttribNormal));
        break;
      case VertexAttributeUsage_Tangent:
        GL_CALL(glDisableVertexAttribArray(kAttribTangent));
        break;
      case VertexAttributeUsage_Orientation:
        GL_CALL(glDisableVertexAttribArray(kAttribOrientation));
        break;
      case VertexAttributeUsage_Color:
        GL_CALL(glDisableVertexAttribArray(kAttribColor));
        break;
      case VertexAttributeUsage_BoneIndices:
        GL_CALL(glDisableVertexAttribArray(kAttribBoneIndices));
        break;
      case VertexAttributeUsage_BoneWeights:
        GL_CALL(glDisableVertexAttribArray(kAttribBoneWeights));
        break;
      case VertexAttributeUsage_TexCoord:
        DCHECK(kAttribTexCoord + tex_coord_count < kAttribTexCoordMax);
        GL_CALL(glDisableVertexAttribArray(kAttribTexCoord + tex_coord_count));
        ++tex_coord_count;
        break;
      default:
        break;
    }
  }
}

void UnsetDefaultAttributes() {
  // Calling glDisableVertexAttribArray() with an unbound vbo results in a
  // GL_INVALID_OPERATION per the spec. Most drivers ignore this but it
  // typically causes problems on PLATFORM_OSX.
  //
  // We may want to remove the #ifdef or (better yet, to avoid the speed hit)
  // restructure the code to eliminate the situation where this is called with
  // unbound vbo's.
#ifdef PLATFORM_OSX
  GLint current_vbo;
  glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &current_vbo);
  if (current_vbo == 0) {
    return;
  }
#endif  // PLATFORM_OSX
  // Can leave position set.
  GL_CALL(glDisableVertexAttribArray(kAttribNormal));
  GL_CALL(glDisableVertexAttribArray(kAttribTangent));
  GL_CALL(glDisableVertexAttribArray(kAttribTexCoord));
  GL_CALL(glDisableVertexAttribArray(kAttribTexCoord1));
  GL_CALL(glDisableVertexAttribArray(kAttribColor));
  GL_CALL(glDisableVertexAttribArray(kAttribBoneIndices));
  GL_CALL(glDisableVertexAttribArray(kAttribBoneWeights));
}

MeshHelper::MeshHelper() {
#ifdef PLATFORM_OSX
  if (GlSupportsVertexArrays()) {
    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glGenBuffers(1, &ibo_);
  }
#endif
}

MeshHelper::~MeshHelper() {
  if (ibo_ != 0) {
    glDeleteBuffers(1, &ibo_);
    ibo_ = 0;
  }
  if (vbo_ != 0) {
    glDeleteBuffers(1, &vbo_);
    vbo_ = 0;
  }
  if (vao_ != 0) {
    glDeleteVertexArrays(1, &vao_);
    vao_ = 0;
  }
}

void MeshHelper::DrawMeshData(const MeshData& mesh_data) {
  const VertexFormat& vertex_format = mesh_data.GetVertexFormat();
  const int num_vertices = static_cast<int>(mesh_data.GetNumVertices());
  const int num_indices = static_cast<int>(mesh_data.GetNumIndices());
  const uint8_t* vertices = num_vertices ? mesh_data.GetVertexBytes() : nullptr;
  const uint8_t* indices = num_indices ? mesh_data.GetIndexBytes() : nullptr;

  if (num_vertices == 0) {
    return;
  }
  if (vertices == nullptr) {
    LOG(DFATAL) << "Can't draw mesh without vertex read access.";
    return;
  }

  if (vao_ > 0) {
    GL_CALL(glBindVertexArray(vao_));
  }
  GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, vbo_));
  GL_CALL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_));
  const GLenum gl_mode = GetGlPrimitiveType(mesh_data.GetPrimitiveType());

  SetVertexAttributes(vertex_format, vertices);
  if (num_indices > 0 && indices != nullptr) {
    const GLenum gl_type = GetGlIndexType(mesh_data.GetIndexType());
    GL_CALL(glDrawElements(gl_mode, num_indices, gl_type, indices));
  } else {
    const GLenum gl_mode = GetGlPrimitiveType(mesh_data.GetPrimitiveType());
    GL_CALL(glDrawArrays(gl_mode, 0, num_vertices));
  }
  UnsetVertexAttributes(vertex_format);
}

void MeshHelper::DrawQuad(const mathfu::vec3& bottom_left,
                          const mathfu::vec3& top_right,
                          const mathfu::vec2& tex_bottom_left,
                          const mathfu::vec2& tex_top_right) {
  const VertexPT vertices[] = {
      {bottom_left.x, bottom_left.y, bottom_left.z, tex_bottom_left.x,
       tex_bottom_left.y},
      {bottom_left.x, top_right.y, top_right.z, tex_bottom_left.x,
       tex_top_right.y},
      {top_right.x, bottom_left.y, bottom_left.z, tex_top_right.x,
       tex_bottom_left.y},
      {top_right.x, top_right.y, top_right.z, tex_top_right.x, tex_top_right.y},
  };
  static const uint16_t indices[] = {0, 1, 2, 1, 3, 2};
  static const int kNumIndices = sizeof(indices) / sizeof(indices[0]);

  if (vao_ > 0) {
    GL_CALL(glBindVertexArray(vao_));
  }
  GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, vbo_));
  GL_CALL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_));
  SetVertexAttributes(VertexPT::kFormat,
                      reinterpret_cast<const uint8_t*>(vertices));
  GL_CALL(
      glDrawElements(GL_TRIANGLES, kNumIndices, GL_UNSIGNED_SHORT, indices));
  UnsetVertexAttributes(VertexPT::kFormat);
}

Span<std::pair<const char*, int>> GetDefaultVertexAttributes() {
  static const std::pair<const char*, int> kDefaultAttributes[] = {
      {"aPosition", kAttribPosition},
      {"aNormal", kAttribNormal},
      {"aTangent", kAttribTangent},
      {"aOrientation", kAttribOrientation},
      {"aTexCoord", kAttribTexCoord},
      {"aTexCoordAlt", kAttribTexCoord1},
      {"aTexCoord2", kAttribTexCoord2},
      {"aTexCoord3", kAttribTexCoord3},
      {"aTexCoord4", kAttribTexCoord4},
      {"aTexCoord5", kAttribTexCoord5},
      {"aTexCoord6", kAttribTexCoord6},
      {"aTexCoord7", kAttribTexCoord7},
      {"aColor", kAttribColor},
      {"aBoneIndices", kAttribBoneIndices},
      {"aBoneWeights", kAttribBoneWeights}};

  return Span<std::pair<const char*, int>>(
      kDefaultAttributes,
      sizeof(kDefaultAttributes) / sizeof(kDefaultAttributes[0]));
}
}  // namespace lull
