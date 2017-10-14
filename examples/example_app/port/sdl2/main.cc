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


extern "C"
{
    #include "SDL.h"
}

#include <functional>

#include "fplbase/utilities.h"
#include "examples/example_app/example_app.h"
#include "examples/example_app/port/sdl2/software_controller.h"
#include "examples/example_app/port/sdl2/software_hmd.h"

namespace lull {

// Manages the SDL platform objects and updates the ExampleApp instance.
class MainWindow {
 public:
  MainWindow() {}

  // Creates the SDL objects and ExampleApp and runs the main event/render loop.
  // Returns the error code for any errors encountered, or 0 if no errors.
  int Run();

 private:
  // Initialize the SDL and ExampleApp objects.
  bool Init();

  // Updates the SDL event loop and ExampleApp instance.
  void Update();

  // Triggers cleanup of all objects and stores the associated exit code.
  void Exit(int code, const char* message = nullptr);

  // Handles quit events.
  void OnQuit();

  // Handles keyboard key down event.
  void OnKeyDown(const SDL_Keysym& keysym);

  // Handles keyboard key up event.
  void OnKeyUp(const SDL_Keysym& keysym);

  // Handles mouse button down event.
  void OnMouseDown(const mathfu::vec2i& position, int button);

  // Handles mouse button up event.
  void OnMouseUp(const mathfu::vec2i& position, int button);

  // Handles mouse movement event.
  void OnMouseMotion(const mathfu::vec2i& position, const mathfu::vec2i& delta);

  // Handles all SDL events and delegates to the appropriate handler.
  static int HandleEvent(void* userdata, SDL_Event* event);

  // Tracks whether the mouse is controlling the HMD or Controller.
  enum MouseMode {
    kNone,
    kHmd,
    kController,
  };

  SDL_Window* window_ = nullptr;
  SDL_GLContext gl_context_ = nullptr;
  std::unique_ptr<ExampleApp> app_;
  std::unique_ptr<SoftwareHmd> hmd_;
  std::unique_ptr<SoftwareController> controller_;
  MouseMode mouse_mode_ = kNone;
  bool quitting_ = false;
  int exit_code_ = 0;
};

int MainWindow::Run() {
  if (Init()) {
    Update();
  }
  return exit_code_;
}

bool MainWindow::Init() {
  app_ = CreateExampleAppInstance();
  if (!app_) {
    Exit(1, "Could not create example app.");
    return false;
  }

  if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_EVENTS) != 0) {
    Exit(1, SDL_GetError());
    return false;
  }

  SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

  const ExampleApp::Config& config = app_->GetConfig();
  window_ = SDL_CreateWindow(config.title.c_str(), SDL_WINDOWPOS_UNDEFINED,
                             SDL_WINDOWPOS_UNDEFINED, config.width,
                             config.height, SDL_WINDOW_OPENGL);
  gl_context_ = SDL_GL_CreateContext(window_);
  SDL_AddEventWatch(HandleEvent, this);

  fplbase::ChangeToUpstreamDir("./", "data/assets");
  app_->Initialize();

  std::shared_ptr<Registry> registry = app_->GetRegistry();
  if (config.enable_hmd) {
    hmd_.reset(new SoftwareHmd(registry.get(), config.width, config.height,
                               config.stereo));
  }
  if (config.enable_controller) {
    controller_.reset(new SoftwareController(registry.get()));
  }
  return true;
}

void MainWindow::Update() {
  while (!quitting_) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {}

    SDL_GL_MakeCurrent(window_, gl_context_);
    if (hmd_) {
      hmd_->Update();
    }
    if (controller_) {
      controller_->Update();
    }
    if (app_) {
      app_->Update();
    }

    SDL_GL_SwapWindow(window_);
  }
  Exit(0);
}

void MainWindow::Exit(int code, const char* message) {
  if (code != 0 && message != nullptr) {
    LOG(ERROR) << message;
  }

  app_->Shutdown();
  controller_.reset();
  hmd_.reset();
  app_.reset();

  SDL_DelEventWatch(HandleEvent, this);
  SDL_GL_DeleteContext(gl_context_);
  SDL_DestroyWindow(window_);
  SDL_Quit();
  exit_code_ = code;
}

void MainWindow::OnMouseDown(const mathfu::vec2i& position, int button) {
  if (mouse_mode_ == kNone) {
    if (button == SDL_BUTTON_LEFT) {
      mouse_mode_ = kHmd;
      SDL_SetRelativeMouseMode(SDL_TRUE);
    } else if (button == SDL_BUTTON_RIGHT) {
      mouse_mode_ = kController;
      SDL_SetRelativeMouseMode(SDL_TRUE);
    }
  }
}

void MainWindow::OnMouseUp(const mathfu::vec2i& position, int button) {
  if (button == SDL_BUTTON_LEFT && mouse_mode_ == kHmd) {
    mouse_mode_ = kNone;
    SDL_SetRelativeMouseMode(SDL_FALSE);
  } else if (button == SDL_BUTTON_RIGHT && mouse_mode_ == kController) {
    mouse_mode_ = kNone;
    SDL_SetRelativeMouseMode(SDL_FALSE);
  }
}

void MainWindow::OnMouseMotion(const mathfu::vec2i& position,
                               const mathfu::vec2i& delta) {
  if (hmd_ && mouse_mode_ == kHmd) {
    const SDL_Keymod mod = SDL_GetModState();
    const bool ctrl = (mod & KMOD_CTRL);
    const bool shift = (mod & KMOD_SHIFT);

    SoftwareHmd::ControlMode mode = SoftwareHmd::kRotatePitchYaw;
    if (ctrl && shift) {
      mode = SoftwareHmd::kTranslateXY;
    } else if (ctrl) {
      mode = SoftwareHmd::kRotateRoll;
    } else if (shift) {
      mode = SoftwareHmd::kTranslateXZ;
    }
    hmd_->OnMouseMotion(delta, mode);
  } else if (controller_ && mouse_mode_ == kController) {
    controller_->OnMouseMotion(delta);
  }
}

void MainWindow::OnKeyDown(const SDL_Keysym& keysym) {
  if (keysym.sym == SDLK_SPACE) {
    if (controller_) {
      controller_->OnButtonDown();
    }
  }
}

void MainWindow::OnKeyUp(const SDL_Keysym& keysym) {
  if (keysym.sym == SDLK_ESCAPE) {
    OnQuit();
  } else if (keysym.sym == SDLK_SPACE) {
    if (controller_) {
      controller_->OnButtonUp();
    }
  }
}

void MainWindow::OnQuit() {
  quitting_ = true;
}

int MainWindow::HandleEvent(void* userdata, SDL_Event* event) {
  MainWindow* self = reinterpret_cast<MainWindow*>(userdata);
  switch (event->type) {
    case SDL_MOUSEMOTION: {
      const mathfu::vec2i position(event->motion.x, event->motion.y);
      const mathfu::vec2i delta(event->motion.xrel, event->motion.yrel);
      self->OnMouseMotion(position, delta);
      break;
    }
    case SDL_MOUSEBUTTONDOWN: {
      const mathfu::vec2i position(event->button.x, event->button.y);
      self->OnMouseDown(position, event->button.button);
      break;
    }
    case SDL_MOUSEBUTTONUP: {
      const mathfu::vec2i position(event->button.x, event->button.y);
      self->OnMouseUp(position, event->button.button);
      break;
    }
    case SDL_KEYDOWN: {
      self->OnKeyDown(event->key.keysym);
      break;
    }
    case SDL_KEYUP: {
      self->OnKeyUp(event->key.keysym);
      break;
    }
    case SDL_APP_WILLENTERBACKGROUND: {
      self->OnQuit();
      break;
    }
    case SDL_QUIT: {
      self->OnQuit();
      break;
    }
  }
  return 1;
}

}  // namespace lull

// Main entry point for application that simply creates and runs the MainWindow.
int main(int argc, char** argv) {
  lull::MainWindow window;
  return window.Run();
}
