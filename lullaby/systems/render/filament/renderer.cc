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

#include "lullaby/systems/render/filament/renderer.h"

#include <memory>

#ifdef __ANDROID__
#include <EGL/egl.h>
#endif

#include "lullaby/systems/render/filament/filament_utils.h"
#include "lullaby/systems/render/filament/sceneview.h"
#include "lullaby/util/hash.h"
#include "lullaby/util/logging.h"

namespace lull {
namespace {

class EnginePtr {
 public:
  EnginePtr() {
    filament::Engine::Backend backend = filament::Engine::Backend::DEFAULT;
    filament::Engine::Platform* platform = nullptr;
    void* context = CreateEglContext();
    engine_ = filament::Engine::create(backend, platform, context);
  }

  ~EnginePtr() {
    filament::Engine::destroy(&engine_);
  }

  void MakeCurrent() {
#ifdef __ANDROID__
    if (!eglMakeCurrent(display_, surface_, surface_, context_)) {
      LOG(DFATAL) << "Unable to set egl context.";
    }
#endif  // __ANDROID__
  }

  filament::Engine* GetEngine() { return engine_; }

 private:
  void* CreateEglContext() {
#ifdef __ANDROID__
    display_ = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglInitialize(display_, nullptr, nullptr);

    const int config_attribs[] = {EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
                                  EGL_NONE};
    int num_config = 0;
    EGLConfig egl_config;
    eglChooseConfig(display_, config_attribs, &egl_config, 1, &num_config);
    DCHECK(num_config > 0) << "Could not choose egl config.";

    const int context_attribs[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
    context_ = eglCreateContext(display_, egl_config, nullptr, context_attribs);
    DCHECK(context_ != 0) << "Could not create egl context.";

    int surface_attribs[] = {EGL_WIDTH, 1, EGL_HEIGHT, 1, EGL_NONE};
    surface_ = eglCreatePbufferSurface(display_, egl_config, surface_attribs);
    DCHECK(surface_ != 0) << "Could not create egl surface.";
    return context_;
#else  // __ANDROID__
    return nullptr;
#endif  // __ANDROID__
  }

#ifdef __ANDROID__
  EGLDisplay display_;
  EGLContext context_;
  EGLSurface surface_;
#endif  // __ANDROID__
  filament::Engine* engine_;
};

// Static crashes if we try to recreate in a different thread.
// Instanced crashes if we try to recreate in the same thread.
// This is because Filament uses a thread_local job system.  While Android
// apps share the same 1 process and ui thread over multiple Activity restarts,
// we'd also like to support the use case of being able to spin up a new runtime
// thread with filament per restart if the app wants.
thread_local std::unique_ptr<EnginePtr> thread_local_ptr(new EnginePtr());

}  // namespace

Renderer::Renderer() : thread_local_ptr_(thread_local_ptr.get()) {
  // Reacquire the context in case we're reusing an old instance.
  thread_local_ptr->MakeCurrent();
  renderer_ = GetEngine()->createRenderer();
}

Renderer::~Renderer() {
  auto* engine = GetEngine();
  if (renderer_) {
    engine->destroy(renderer_);
  }
  if (swap_chain_) {
    engine->destroy(swap_chain_);
  }
}

filament::Engine* Renderer::GetEngine() {
  DCHECK_EQ(thread_local_ptr_, thread_local_ptr.get())
      << "Calling Filament Renderer on different thread.";
  return thread_local_ptr->GetEngine();
}

void Renderer::SetNativeWindow(void* native_window) {
  if (swap_chain_) {
    GetEngine()->destroy(swap_chain_);
    swap_chain_ = nullptr;
  }

  if (native_window) {
    swap_chain_ = GetEngine()->createSwapChain(native_window);
  }
}

void Renderer::SetClearColor(const mathfu::vec4 color) {
  clear_color_ = color;
}

mathfu::vec4 Renderer::GetClearColor() const {
  return clear_color_;
}

void Renderer::Render(SceneView* sceneview, Span<RenderView> views) {
  if (!swap_chain_) {
    LOG(WARNING) << "Rendering without swap chain!";
    return;
  }

  sceneview->Prepare(clear_color_, views);
  if (!renderer_->beginFrame(swap_chain_)) {
    return;
  }
  for (size_t i = 0; i < views.size(); ++i) {
    renderer_->render(sceneview->GetView(i));
  }
  renderer_->endFrame();

#if __EMSCRIPTEN__
  // Needed to run filament on a single thread.
  GetEngine()->execute();
#endif
}

}  // namespace lull
