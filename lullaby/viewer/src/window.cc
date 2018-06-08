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

#include "lullaby/viewer/src/window.h"

#include <SDL.h>
#include "dear_imgui/imgui.h"
#include "fplbase/glplatform.h"
#include "fplbase/utilities.h"
#include "lullaby/util/logging.h"

namespace lull {
namespace tool {

static const char* g_vertex_shader_source =
    "#version 300 es\n"
    "precision highp float;\n"
    "uniform mat4 u_projection_matrix;\n"
    "in vec2 a_position;\n"
    "in vec2 a_texcoord;\n"
    "in vec4 a_color;\n"
    "out vec2 v_texcoord;\n"
    "out vec4 v_color;\n"
    "void main()\n"
    "{\n"
    "  v_texcoord = a_texcoord;\n"
    "  v_color = a_color;\n"
    "  gl_Position = u_projection_matrix * vec4(a_position.xy, 0, 1);\n"
    "}\n";

static const char* g_fragment_shader_source =
    "#version 300 es\n"
    "precision highp float;\n"
    "uniform sampler2D u_texture;\n"
    "in vec2 v_texcoord;\n"
    "in vec4 v_color;\n"
    "out vec4 out_color;\n"
    "void main()\n"
    "{\n"
    "  out_color = v_color * texture(u_texture, v_texcoord.st);\n"
    "}\n";

static const char* GetClipboardText(void*) {
  return SDL_GetClipboardText();
}

static void SetClipboardText(void*, const char* text) {
  SDL_SetClipboardText(text);
}

namespace {
class ScopedRenderState {
 public:
  ScopedRenderState();
  ~ScopedRenderState();
 private:
  GLint active_texture_;
  GLint program_;
  GLint texture_;
  GLint array_buffer_;
  GLint element_array_buffer_;
  GLint blend_src_rgb_;
  GLint blend_dst_rgb_;
  GLint blend_src_alpha_;
  GLint blend_dst_alpha_;
  GLint blend_equation_rgb_;
  GLint blend_equation_alpha_;
  GLint viewport_[4];
  GLint scissor_box_[4];
  GLboolean enable_blend_;
  GLboolean enable_cull_face_;
  GLboolean enable_depth_test_;
  GLboolean enable_scissor_test_;
};

ScopedRenderState::ScopedRenderState() {
  glGetIntegerv(GL_ACTIVE_TEXTURE, &active_texture_);
  glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &array_buffer_);
  glGetIntegerv(GL_BLEND_DST_ALPHA, &blend_dst_alpha_);
  glGetIntegerv(GL_BLEND_DST_RGB, &blend_dst_rgb_);
  glGetIntegerv(GL_BLEND_EQUATION_ALPHA, &blend_equation_alpha_);
  glGetIntegerv(GL_BLEND_EQUATION_RGB, &blend_equation_rgb_);
  glGetIntegerv(GL_BLEND_SRC_ALPHA, &blend_src_alpha_);
  glGetIntegerv(GL_BLEND_SRC_RGB, &blend_src_rgb_);
  glGetIntegerv(GL_CURRENT_PROGRAM, &program_);
  glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &element_array_buffer_);
  glGetIntegerv(GL_SCISSOR_BOX, scissor_box_);
  glGetIntegerv(GL_TEXTURE_BINDING_2D, &texture_);
  glGetIntegerv(GL_VIEWPORT, viewport_);
  enable_blend_ = glIsEnabled(GL_BLEND);
  enable_cull_face_ = glIsEnabled(GL_CULL_FACE);
  enable_depth_test_ = glIsEnabled(GL_DEPTH_TEST);
  enable_scissor_test_ = glIsEnabled(GL_SCISSOR_TEST);
}

ScopedRenderState::~ScopedRenderState() {
  glUseProgram(program_);
  glBindTexture(GL_TEXTURE_2D, texture_);
  glActiveTexture(active_texture_);
  glBindBuffer(GL_ARRAY_BUFFER, array_buffer_);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, element_array_buffer_);
  glBlendEquationSeparate(blend_equation_rgb_, blend_equation_alpha_);
  glBlendFuncSeparate(blend_src_rgb_, blend_dst_rgb_, blend_src_alpha_,
                      blend_dst_alpha_);
  glViewport(viewport_[0], viewport_[1], viewport_[2], viewport_[3]);
  glScissor(scissor_box_[0], scissor_box_[1], scissor_box_[2], scissor_box_[3]);
  if (enable_blend_) {
    glEnable(GL_BLEND);
  } else {
    glDisable(GL_BLEND);
  }
  if (enable_cull_face_) {
    glEnable(GL_CULL_FACE);
  } else {
    glDisable(GL_CULL_FACE);
  }
  if (enable_depth_test_) {
    glEnable(GL_DEPTH_TEST);
  } else {
    glDisable(GL_DEPTH_TEST);
  }
  if (enable_scissor_test_) {
    glEnable(GL_SCISSOR_TEST);
  } else{
    glDisable(GL_SCISSOR_TEST);
  }
}
}

void Window::Update() {
  const double current_time = SDL_GetTicks() / 1000.0;
  double delta_time = 1.0 / 60.0;
  if (time_ > 0.0) {
    delta_time = current_time - time_;
  }
  time_ = current_time;

  input_state_ = InputState();
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    ProcessSdlEvent(&event);
  }
  UpdateInputState(delta_time);

  PrepareImgui(delta_time);

  int width = 0;
  int height = 0;
  SDL_GetWindowSize(sdl_window_, &width, &height);

  SDL_GL_MakeCurrent(sdl_window_, sdl_context_);

  AdvanceFrame(delta_time, width, height);
  RenderImgui();

  SDL_GL_SwapWindow(sdl_window_);
}

void Window::RenderImgui() {
  ImGui::Render();
  ImDrawData* draw_data = ImGui::GetDrawData();

  ImGuiIO& io = ImGui::GetIO();

  static const GLenum kIndexSize =
      sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT;

  const auto& size = io.DisplaySize;
  const auto& scale = io.DisplayFramebufferScale;
  const int width = static_cast<int>(size.x * scale.x);
  const int height = static_cast<int>(size.y * scale.y);
  if (width == 0 || height == 0) {
    return;
  }

  draw_data->ScaleClipRects(scale);
  const float orthographic_projection[4][4] = {
      {2.0f / io.DisplaySize.x, 0.0f, 0.0f, 0.0f},
      {0.0f, 2.0f / -io.DisplaySize.y, 0.0f, 0.0f},
      {0.0f, 0.0f, -1.0f, 0.0f},
      {-1.0f, 1.0f, 0.0f, 1.0f},
  };

  ScopedRenderState render_state;
  glEnable(GL_BLEND);
  glBlendEquation(GL_FUNC_ADD);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glDisable(GL_CULL_FACE);
  glDisable(GL_DEPTH_TEST);
  glEnable(GL_SCISSOR_TEST);
  glActiveTexture(GL_TEXTURE0);
  glViewport(0, 0, width, height);

  glUseProgram(shader_);

  const GLuint matrix_location =
      glGetUniformLocation(shader_, "u_projection_matrix");
  glUniformMatrix4fv(matrix_location, 1, GL_FALSE,
                     &orthographic_projection[0][0]);
  const GLuint texture_location = glGetUniformLocation(shader_, "u_texture");
  glUniform1i(texture_location, 0);

  const size_t stride = sizeof(ImDrawVert);
  const GLuint position_location = glGetAttribLocation(shader_, "a_position");
  const GLuint texcoord_location = glGetAttribLocation(shader_, "a_texcoord");
  const GLuint color_location = glGetAttribLocation(shader_, "a_color");

  glEnableVertexAttribArray(position_location);
  glEnableVertexAttribArray(texcoord_location);
  glEnableVertexAttribArray(color_location);

  for (int n = 0; n < draw_data->CmdListsCount; ++n) {
    const ImDrawList* cmd_list = draw_data->CmdLists[n];
    const ImDrawIdx* idx_buffer_offset = 0;

    const size_t num_vertices = cmd_list->VtxBuffer.Size;
    const size_t num_indices = cmd_list->IdxBuffer.Size;
    const void* vertices = cmd_list->VtxBuffer.Data;
    const void* indices = cmd_list->IdxBuffer.Data;

    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elements_);
    glBufferData(GL_ARRAY_BUFFER, num_vertices * sizeof(ImDrawVert), vertices,
                 GL_STREAM_DRAW);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, num_indices * sizeof(ImDrawIdx),
                 indices, GL_STREAM_DRAW);
    glVertexAttribPointer(position_location, 2, GL_FLOAT, GL_FALSE, stride,
                          (GLvoid*)offsetof(ImDrawVert, pos));
    glVertexAttribPointer(texcoord_location, 2, GL_FLOAT, GL_FALSE, stride,
                          (GLvoid*)offsetof(ImDrawVert, uv));
    glVertexAttribPointer(color_location, 4, GL_UNSIGNED_BYTE, GL_TRUE, stride,
                          (GLvoid*)offsetof(ImDrawVert, col));

    for (int i = 0; i < cmd_list->CmdBuffer.Size; ++i) {
      const ImDrawCmd& command = cmd_list->CmdBuffer[i];
      const GLint num_elements = command.ElemCount;

      if (command.UserCallback) {
        command.UserCallback(cmd_list, &command);
      } else {
        const auto& rect = command.ClipRect;
        const GLuint texture =
            static_cast<GLuint>(reinterpret_cast<uintptr_t>(command.TextureId));

        glBindTexture(GL_TEXTURE_2D, texture);
        glScissor(static_cast<int>(rect.x),
                  static_cast<int>(height - rect.w),
                  static_cast<int>(rect.z - rect.x),
                  static_cast<int>(rect.w - rect.y));
        glDrawElements(GL_TRIANGLES, num_elements, kIndexSize,
                       idx_buffer_offset);
      }
      idx_buffer_offset += num_elements;
    }
  }
}

void Window::Initialize(const InitParams& params) {
  InitializeSdl(params.label, params.width, params.height);
  InitializeImgui();
  InitializeGl();
  InitializeFontTexture();

  OnInitialize();
}

void Window::Shutdown() {
  OnShutdown();
  ShutdownFontTexture();
  ShutdownGl();
  ShutdownImgui();
  ShutdownSdl();
}

void Window::InitializeSdl(const std::string& name, size_t width,
                           size_t height) {
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    LOG(ERROR) << "Could not initialize SDL.";
    return;
  }

  SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

  sdl_window_ = SDL_CreateWindow(
      name.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width,
      height, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
  if (sdl_window_ == nullptr) {
    LOG(ERROR) << "Could not create window.";
    return;
  }

  sdl_context_ = SDL_GL_CreateContext(sdl_window_);
  SDL_GL_SetSwapInterval(1);

  fplbase::ChangeToUpstreamDir("./", "assets");
}

void Window::ShutdownSdl() {
  SDL_GL_DeleteContext(sdl_context_);
  SDL_DestroyWindow(sdl_window_);
  SDL_Quit();
}

void Window::InitializeImgui() {
  ImGuiIO& io = ImGui::GetIO();
  io.KeyMap[ImGuiKey_Tab] = SDLK_TAB;
  io.KeyMap[ImGuiKey_LeftArrow] = SDL_SCANCODE_LEFT;
  io.KeyMap[ImGuiKey_RightArrow] = SDL_SCANCODE_RIGHT;
  io.KeyMap[ImGuiKey_UpArrow] = SDL_SCANCODE_UP;
  io.KeyMap[ImGuiKey_DownArrow] = SDL_SCANCODE_DOWN;
  io.KeyMap[ImGuiKey_PageUp] = SDL_SCANCODE_PAGEUP;
  io.KeyMap[ImGuiKey_PageDown] = SDL_SCANCODE_PAGEDOWN;
  io.KeyMap[ImGuiKey_Home] = SDL_SCANCODE_HOME;
  io.KeyMap[ImGuiKey_End] = SDL_SCANCODE_END;
  io.KeyMap[ImGuiKey_Delete] = SDLK_DELETE;
  io.KeyMap[ImGuiKey_Backspace] = SDLK_BACKSPACE;
  io.KeyMap[ImGuiKey_Enter] = SDLK_RETURN;
  io.KeyMap[ImGuiKey_Escape] = SDLK_ESCAPE;
  io.KeyMap[ImGuiKey_A] = SDLK_a;
  io.KeyMap[ImGuiKey_C] = SDLK_c;
  io.KeyMap[ImGuiKey_V] = SDLK_v;
  io.KeyMap[ImGuiKey_X] = SDLK_x;
  io.KeyMap[ImGuiKey_Y] = SDLK_y;
  io.KeyMap[ImGuiKey_Z] = SDLK_z;
  io.SetClipboardTextFn = SetClipboardText;
  io.GetClipboardTextFn = GetClipboardText;
  io.ClipboardUserData = NULL;
#ifdef _WIN32
  SDL_SysWMinfo wm_info;
  SDL_VERSION(&wm_info.version);
  SDL_GetWindowWMInfo(window, &wm_info);
  ImGui::GetIO().ImeWindowHandle = wm_info.info.win.window;
#endif
}

void Window::ShutdownImgui() { ImGui::Shutdown(); }

void Window::InitializeGl() {
  ScopedRenderState render_state;
  auto vertex_shader = glCreateShader(GL_VERTEX_SHADER);
  auto fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(vertex_shader, 1, &g_vertex_shader_source, 0);
  glShaderSource(fragment_shader, 1, &g_fragment_shader_source, 0);
  glCompileShader(vertex_shader);
  glCompileShader(fragment_shader);

  GLint success = 0;
  glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
  if (success == GL_FALSE) {
    GLint len = 0;
    glGetShaderiv(vertex_shader, GL_INFO_LOG_LENGTH, &len);
    std::vector<GLchar> log(len);
    glGetShaderInfoLog(vertex_shader, len, &len, &log[0]);
    LOG(ERROR) << log.data();
  }

  glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
  if (success == GL_FALSE) {
    GLint len = 0;
    glGetShaderiv(fragment_shader, GL_INFO_LOG_LENGTH, &len);
    std::vector<GLchar> log(len);
    glGetShaderInfoLog(fragment_shader, len, &len, &log[0]);
    LOG(ERROR) << log.data();
  }

  shader_ = glCreateProgram();
  glAttachShader(shader_, vertex_shader);
  glAttachShader(shader_, fragment_shader);
  glLinkProgram(shader_);

  glGetProgramiv(shader_, GL_LINK_STATUS, &success);
  if (success == GL_FALSE) {
    GLint len = 0;
    glGetProgramiv(shader_, GL_INFO_LOG_LENGTH, &len);
    std::vector<GLchar> log(len);
    glGetProgramInfoLog(shader_, len, &len, &log[0]);
    LOG(ERROR) << log.data();
  }

  glGenBuffers(1, &vbo_);
  glGenBuffers(1, &elements_);
}

void Window::ShutdownGl() {
  if (vbo_) {
    glDeleteBuffers(1, &vbo_);
    vbo_ = 0;
  }
  if (elements_) {
    glDeleteBuffers(1, &elements_);
    elements_ = 0;
  }
  if (shader_) {
    glDeleteProgram(shader_);
    shader_ = 0;
  }
}

void Window::InitializeFontTexture() {
  ScopedRenderState render_state;
  ImGuiIO& io = ImGui::GetIO();

  io.Fonts->AddFontDefault();

  ImFontConfig config;
  config.MergeMode = true;
  static const ImWchar ranges[] = { 0xf000, 0xf3ff, 0 };
  io.Fonts->AddFontFromFileTTF("fontawesome-webfont.ttf", 13.0f, &config, ranges);

  int width = 0;
  int height = 0;
  unsigned char* pixels = nullptr;
  io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

  glGenTextures(1, &font_texture_);
  glBindTexture(GL_TEXTURE_2D, font_texture_);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA,
               GL_UNSIGNED_BYTE, pixels);
  io.Fonts->TexID = (void*)(intptr_t)font_texture_;
}

void Window::ShutdownFontTexture() {
  if (font_texture_ != 0) {
    ImGui::GetIO().Fonts->TexID = 0;
    glDeleteTextures(1, &font_texture_);
    font_texture_ = 0;
  }
}

void Window::ProcessSdlEvent(const SDL_Event* event) {
  ImGuiIO& io = ImGui::GetIO();
  switch (event->type) {
    case SDL_MOUSEWHEEL: {
      if (event->wheel.y > 0) {
        input_state_.mouse_wheel = 1;
      }
      if (event->wheel.y < 0) {
        input_state_.mouse_wheel = -1;
      }
      break;
    }
    case SDL_MOUSEBUTTONDOWN: {
      if (event->button.button == SDL_BUTTON_LEFT) {
        input_state_.mouse_buttons[0] = true;
      }
      if (event->button.button == SDL_BUTTON_RIGHT) {
        input_state_.mouse_buttons[1] = true;
      }
      if (event->button.button == SDL_BUTTON_MIDDLE) {
        input_state_.mouse_buttons[2] = true;
      }
      break;
    }
    case SDL_TEXTINPUT: {
      io.AddInputCharactersUTF8(event->text.text);
      break;
    }
    case SDL_KEYDOWN:
    case SDL_KEYUP: {
      int key = event->key.keysym.sym & ~SDLK_SCANCODE_MASK;
      io.KeysDown[key] = (event->type == SDL_KEYDOWN);
      io.KeyShift = ((SDL_GetModState() & KMOD_SHIFT) != 0);
      io.KeyCtrl = ((SDL_GetModState() & KMOD_CTRL) != 0);
      io.KeyAlt = ((SDL_GetModState() & KMOD_ALT) != 0);
      io.KeySuper = ((SDL_GetModState() & KMOD_GUI) != 0);
      break;
    }
    case SDL_QUIT: {
      quit_ = true;
      break;
    }
  }
}

void Window::UpdateInputState(double dt) {
  int mx = -1;
  int my = -1;
  const uint32_t mask = SDL_GetMouseState(&mx, &my);

  if (SDL_GetWindowFlags(sdl_window_) & SDL_WINDOW_MOUSE_FOCUS) {
    input_state_.mouse_x = static_cast<float>(mx);
    input_state_.mouse_y = static_cast<float>(my);
  } else {
    input_state_.mouse_x = -1.f;
    input_state_.mouse_y = -1.f;
  }
  input_state_.mouse_buttons[0] |= (mask & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;
  input_state_.mouse_buttons[1] |= (mask & SDL_BUTTON(SDL_BUTTON_RIGHT)) != 0;
  input_state_.mouse_buttons[2] |=
      (mask & SDL_BUTTON(SDL_BUTTON_MIDDLE)) != 0;
}

void Window::PrepareImgui(double dt) {
  ImGuiIO& io = ImGui::GetIO();

  int window_width = 0;
  int window_height = 0;
  SDL_GetWindowSize(sdl_window_, &window_width, &window_height);
  io.DisplaySize.x = static_cast<float>(window_width);
  io.DisplaySize.y = static_cast<float>(window_height);

  int display_width = 0;
  int display_height = 0;
  SDL_GL_GetDrawableSize(sdl_window_, &display_width, &display_height);
  float scale_width = 0;
  if (window_width > 0) {
    scale_width = static_cast<float>(display_width) / io.DisplaySize.x;
  }
  float scale_height = 0;
  if (window_height > 0) {
    scale_height = static_cast<float>(display_height) / io.DisplaySize.y;
  }
  io.DisplayFramebufferScale.x = scale_width;
  io.DisplayFramebufferScale.y = scale_height;

  io.MousePos.x = input_state_.mouse_x;
  io.MousePos.y = input_state_.mouse_y;
  io.MouseDown[0] = input_state_.mouse_buttons[0];
  io.MouseDown[1] = input_state_.mouse_buttons[1];
  io.MouseDown[2] = input_state_.mouse_buttons[2];
  io.MouseWheel = input_state_.mouse_wheel;

  io.DeltaTime = dt;

  SDL_ShowCursor(io.MouseDrawCursor ? 0 : 1);
  ImGui::NewFrame();
}

void Window::Exit(const std::string& message, int exit_code) {
  if (exit_code != 0) {
    LOG(ERROR) << message;
    exit_code_ = exit_code;
  }
  quit_ = true;
}

}  // namespace tool
}  // namespace lull
