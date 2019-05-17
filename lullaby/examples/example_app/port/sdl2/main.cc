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

#include <functional>


#ifdef __linux__
#include <unistd.h>
#endif

#include "SDL.h"
#include "SDL_syswm.h"
#include "lullaby/examples/example_app/example_app.h"
#include "lullaby/examples/example_app/port/sdl2/software_controller.h"
#include "lullaby/examples/example_app/port/sdl2/software_hmd.h"
#if defined(__APPLE__)
#include <CoreFoundation/CoreFoundation.h>
#include <TargetConditionals.h>
#include "lullaby/examples/example_app/port/sdl2/get_native_window.h"
#endif

namespace lull {

// Attempts to change the working dir to |target|.  Continuously scans up the
// current working dir one folder at a time until the target is found.
static bool ChangeWorkingDir(const char* const target) {
  static constexpr int kMaxPathLength = 1024;
  static constexpr int kMaxTreeDepth = 64;

  char curr_dir[kMaxPathLength] = ".";

#if defined(__APPLE__)
  // Get the root of the target directory from the Bundle instead of using the
  // directory specified by the client.
  CFBundleRef main_bundle = CFBundleGetMainBundle();
  CFURLRef resources_url = CFBundleCopyResourcesDirectoryURL(main_bundle);
  if (!CFURLGetFileSystemRepresentation(resources_url, true,
                                        reinterpret_cast<UInt8*>(curr_dir),
                                        sizeof(curr_dir))) {
    LOG(ERROR) << "Could not get the bundle directory";
    return false;
  }
  CFRelease(resources_url);
  if (chdir(curr_dir) != 0) {
    return false;
  }
#else
  getcwd(curr_dir, sizeof(curr_dir));
#endif

  for (int i = 0; i < kMaxTreeDepth; ++i) {
    const int retval = chdir(target);
    if (retval == 0) {
      return true;
    }
    // Scan "up" the directory structure until we find the first path that
    // contains our target directory.
    chdir("..");
  }
  return false;
}

// SDL's relative mouse mode doesn't work while chromoting, so disable it for
// now. If we wanted to get fancy, we could detect chromote sessions via
// $DISPLAY or $DESKTOP_SESSION or something.
constexpr bool kEnableRelativeMouseMode = false;

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

  // Simulates HMD movement.
  void SimulateHMDMovement();

  // Handles all SDL events and delegates to the appropriate handler.
  static int HandleEvent(void* userdata, SDL_Event* event);

  // Tracks whether the mouse is controlling the HMD or Controller.
  enum MouseMode {
    kNone,
    kHmd,
    kController,
  };

  SDL_Window* window_ = nullptr;
  void* native_window_ = nullptr;
  SDL_GLContext gl_context_ = nullptr;
  std::unique_ptr<ExampleApp> app_;
  std::unique_ptr<SoftwareHmd> hmd_;
  std::unique_ptr<SoftwareController> controller_;
  MouseMode mouse_mode_ = kNone;
  bool quitting_ = false;
  int exit_code_ = 0;
  bool pressed_keys_[256] = {false};
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

  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
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

  if (!window_) {
    Exit(1, "SDL_CreateWindow failed.");
    return false;
  }

  SDL_SysWMinfo wmi;
  SDL_VERSION(&wmi.version);
  SDL_GetWindowWMInfo(window_, &wmi);
#if defined(__linux__)
  native_window_ = reinterpret_cast<void*>(wmi.info.x11.window);
#elif defined(__APPLE__)
  native_window_ =
      GetNativeWindow(reinterpret_cast<void*>(wmi.info.cocoa.window));
#elif defined(MSC_VER)
  native_window_ = reinterpret_cast<void*>(wmi.info.win.window);
#endif

  gl_context_ = SDL_GL_CreateContext(window_);

  if (!gl_context_) {
    Exit(1, "SDL_GL_CreateContext failed.");
    return false;
  }

  SDL_AddEventWatch(HandleEvent, this);

  if (!ChangeWorkingDir("data/assets")) {
    Exit(1, "Could not change working directory.");
    return false;
  }
  app_->Initialize(native_window_);

  std::shared_ptr<Registry> registry = app_->GetRegistry();
  if (config.enable_hmd) {
    hmd_.reset(new SoftwareHmd(registry.get(), config.width, config.height,
                               config.stereo, config.near_clip_plane,
                               config.far_clip_plane));
  }
  if (config.enable_controller) {
    controller_.reset(new SoftwareController(registry.get()));
  }
  return true;
}

void MainWindow::Update() {
  while (!quitting_) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
    }

    SDL_GL_MakeCurrent(window_, gl_context_);
    SimulateHMDMovement();
    if (hmd_) {
      hmd_->Update();
    }
    if (controller_) {
      controller_->Update();
    }
    if (app_) {
      app_->Update();
    }
#ifndef LULLABY_RENDER_BACKEND_FILAMENT
    SDL_GL_SwapWindow(window_);
#endif
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
      if (kEnableRelativeMouseMode) {
        SDL_SetRelativeMouseMode(SDL_TRUE);
      }
    } else if (button == SDL_BUTTON_RIGHT) {
      mouse_mode_ = kController;
      if (kEnableRelativeMouseMode) {
        SDL_SetRelativeMouseMode(SDL_TRUE);
      }
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
      mode = SoftwareHmd::kRotateRoll;
    } else if (ctrl) {
      mode = SoftwareHmd::kTranslateXY;
    } else if (shift) {
      mode = SoftwareHmd::kTranslateXZ;
    }
    hmd_->OnMouseMotion(delta, mode);
  } else if (controller_ && mouse_mode_ == kController) {
    controller_->OnMouseMotion(delta);
  }
}

void MainWindow::OnKeyDown(const SDL_Keysym& keysym) {
  if (keysym.sym < 256) {
    pressed_keys_[keysym.sym] = true;
  }
  if (keysym.sym == SDLK_SPACE) {
    if (controller_) {
      controller_->OnButtonDown();
    }
  } else if (keysym.sym == SDLK_TAB) {
    if (controller_) {
      controller_->OnSecondaryButtonDown();
    }
  }
}

void MainWindow::OnKeyUp(const SDL_Keysym& keysym) {
  if (keysym.sym < 256) {
    pressed_keys_[keysym.sym] = false;
  }

  if (keysym.sym == SDLK_ESCAPE) {
    OnQuit();
  } else if (keysym.sym == SDLK_SPACE) {
    if (controller_) {
      controller_->OnButtonUp();
    }
  } else if (keysym.sym == SDLK_TAB) {
    if (controller_) {
      controller_->OnSecondaryButtonUp();
    }
  }
}

void MainWindow::OnQuit() { quitting_ = true; }

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

void MainWindow::SimulateHMDMovement() {
  mathfu::vec2i xy(0);
  if (pressed_keys_[SDLK_z]) {
    xy.y += 1;
  }
  if (pressed_keys_[SDLK_x]) {
    xy.y -= 1;
  }
  if (pressed_keys_[SDLK_a]) {
    xy.x += 1;
  } else if (pressed_keys_[SDLK_d]) {
    xy.x -= 1;
  }
  hmd_->OnMouseMotion(xy, SoftwareHmd::kTranslateXY);

  mathfu::vec2i xz(0);
  if (pressed_keys_[SDLK_w]) {
    xz.y += 1;
  } else if (pressed_keys_[SDLK_s]) {
    xz.y -= 1;
  }
  hmd_->OnMouseMotion(xz, SoftwareHmd::kTranslateXZ);
}

}  // namespace lull

// Main entry point for application that simply creates and runs the MainWindow.
int main(int argc, char** argv) {
  lull::MainWindow window;
  return window.Run();
}
