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

#include "lullaby/systems/render/next/render_target.h"

#include "lullaby/systems/render/next/gl_helpers.h"

namespace {
const int kRgbaStride = 4;
}  // namespace

namespace lull {

RenderTarget::RenderTarget(const RenderTargetCreateParams& create_params)
    : dimensions_(create_params.dimensions),
      num_mip_levels_(create_params.num_mip_levels) {
  GLint original_frame_buffer = 0;
  GLint original_render_buffer = 0;
  GL_CALL(glGetIntegerv(GL_FRAMEBUFFER_BINDING, &original_frame_buffer));
  GL_CALL(glGetIntegerv(GL_RENDERBUFFER_BINDING, &original_render_buffer));

  GLuint gl_framebuffer_id = 0;
  GL_CALL(glGenFramebuffers(1, &gl_framebuffer_id));
  GL_CALL(glBindFramebuffer(GL_FRAMEBUFFER, gl_framebuffer_id));
  frame_buffer_ = gl_framebuffer_id;

  const bool is_depth_texture =
      (create_params.texture_format == TextureFormat_Depth16 ||
       create_params.texture_format == TextureFormat_Depth32F);

  if (create_params.texture_format != TextureFormat_None) {
    const GLenum target =
        is_depth_texture ? GL_DEPTH_ATTACHMENT : GL_COLOR_ATTACHMENT0;

    GLuint gl_texture_id = 0;
    GL_CALL(glGenTextures(1, &gl_texture_id));
    GL_CALL(glBindTexture(GL_TEXTURE_2D, gl_texture_id));
    GL_CALL(glTexImage2D(GL_TEXTURE_2D, 0,
                         GetGlInternalFormat(create_params.texture_format),
                         create_params.dimensions.x, create_params.dimensions.y,
                         0, GetGlFormat(create_params.texture_format),
                         GetGlType(create_params.texture_format), nullptr));
    GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                            GetGlTextureFiltering(create_params.mag_filter)));
    GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                            GetGlTextureFiltering(create_params.min_filter)));
    GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
                            GetGlTextureWrap(create_params.wrap_s)));
    GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
                            GetGlTextureWrap(create_params.wrap_t)));
    GL_CALL(glFramebufferTexture2D(GL_FRAMEBUFFER, target, GL_TEXTURE_2D,
                                   gl_texture_id, 0));
    texture_ = gl_texture_id;

    if (num_mip_levels_ == 0) {
      glGenerateMipmap(GL_TEXTURE_2D);
    } else if (num_mip_levels_ > 1) {
      LOG(ERROR)
          << "Manually specified number of mipmaps is currently not supported.";
    }

    if (is_depth_texture) {
      const GLenum draw_buffers = GL_NONE;
      GL_CALL(glDrawBuffers(1, &draw_buffers));
      GL_CALL(glReadBuffer(GL_NONE));
    }
  }

  if (create_params.depth_stencil_format != DepthStencilFormat_None &&
      !is_depth_texture) {
    GLuint gl_depthbuffer_id = 0;
    GL_CALL(glGenRenderbuffers(1, &gl_depthbuffer_id));
    GL_CALL(glBindRenderbuffer(GL_RENDERBUFFER, gl_depthbuffer_id));
    GL_CALL(glRenderbufferStorage(
        GL_RENDERBUFFER,
        GetGlInternalFormat(create_params.depth_stencil_format),
        create_params.dimensions.x, create_params.dimensions.y));
    GL_CALL(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                      GL_RENDERBUFFER, gl_depthbuffer_id));
    depth_buffer_ = gl_depthbuffer_id;
  }

  DCHECK(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

  GL_CALL(glBindTexture(GL_TEXTURE_2D, 0));
  GL_CALL(glBindFramebuffer(GL_FRAMEBUFFER, original_frame_buffer));
  GL_CALL(glBindRenderbuffer(GL_RENDERBUFFER, original_render_buffer));
}

RenderTarget::~RenderTarget() {
  if (frame_buffer_) {
    GLuint handle = *frame_buffer_;
    GL_CALL(glDeleteFramebuffers(1, &handle));
  }
  if (depth_buffer_) {
    GLuint handle = *depth_buffer_;
    GL_CALL(glDeleteRenderbuffers(1, &handle));
  }
  if (texture_) {
    GLuint handle = *texture_;
    GL_CALL(glDeleteTextures(1, &handle));
  }
}

void RenderTarget::Bind() const {
  if (frame_buffer_) {
    GL_CALL(glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &prev_frame_buffer_));
    GL_CALL(glBindFramebuffer(GL_FRAMEBUFFER, *frame_buffer_));
    GL_CALL(glViewport(0, 0, dimensions_.x, dimensions_.y));
  }
}

void RenderTarget::Unbind() const {
  if (texture_ && num_mip_levels_ == 0) {
    GLuint current_texture_id;
    GL_CALL(glGetIntegerv(GL_TEXTURE_BINDING_2D,
                          reinterpret_cast<GLint*>(&current_texture_id)));

    GL_CALL(glBindTexture(GL_TEXTURE_2D, *texture_));
    GL_CALL(glGenerateMipmap(GL_TEXTURE_2D));
    GL_CALL(glBindTexture(GL_TEXTURE_2D, current_texture_id));
  }
  GL_CALL(glBindFramebuffer(GL_FRAMEBUFFER, prev_frame_buffer_));
  prev_frame_buffer_ = 0;
}

ImageData RenderTarget::GetFrameBufferData() const {
  if (!frame_buffer_) {
    LOG(WARNING) << "No Framebuffer!";
    return ImageData();
  }
  // Save previous OpenGL state.
  GLint viewport[4];
  GL_CALL(glGetIntegerv(GL_VIEWPORT, viewport));
  GLint prev_frame_buffer;
  GL_CALL(glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prev_frame_buffer));
  GLint prev_read_buffer;
  GL_CALL(glGetIntegerv(GL_READ_BUFFER, &prev_read_buffer));

  Bind();
  const int size = dimensions_.x * dimensions_.y * kRgbaStride;
  DataContainer container = DataContainer::CreateHeapDataContainer(size);
  uint8_t* pixels = container.GetAppendPtr(size);

  GL_CALL(glReadBuffer(GL_COLOR_ATTACHMENT0));
  GL_CALL(glPixelStorei(GL_PACK_ALIGNMENT, 1));
  GL_CALL(glReadPixels(0, 0, dimensions_.x, dimensions_.y, GL_RGBA,
                       GL_UNSIGNED_BYTE, pixels));

  // Restore OpenGL state.
  GL_CALL(glViewport(viewport[0], viewport[1], viewport[2], viewport[3]));
  GL_CALL(glBindFramebuffer(GL_FRAMEBUFFER, prev_frame_buffer));
  GL_CALL(glReadBuffer(prev_read_buffer));

  return ImageData(ImageData::kRgba8888, dimensions_, std::move(container));
}
}  // namespace lull
