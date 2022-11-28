/*
Copyright 2017-2022 Google Inc. All Rights Reserved.

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

#include "redux/engines/render/filament/filament_render_engine.h"

#include "filament/Fence.h"
#include "redux/engines/platform/device_manager.h"
#include "redux/engines/render/filament/filament_indirect_light.h"
#include "redux/engines/render/filament/filament_light.h"
#include "redux/engines/render/filament/filament_render_target.h"
#include "redux/engines/render/filament/filament_renderable.h"
#include "redux/engines/render/renderable.h"
#include "redux/modules/base/choreographer.h"
#include "redux/modules/base/static_registry.h"
#include "redux/modules/graphics/image_utils.h"

namespace redux {

#ifdef __APPLE__
constexpr auto kFilamentBackend = filament::Engine::Backend::OPENGL;
#else
constexpr auto kFilamentBackend = filament::Engine::Backend::VULKAN;
#endif

constexpr HashValue kDefaultName = ConstHash("default");

static ImageFormat ToImageFormat(RenderTargetFormat format) {
  switch (format) {
    case RenderTargetFormat::Red8:
      return ImageFormat::Alpha8;
    case RenderTargetFormat::Rgb8:
      return ImageFormat::Rgb888;
    case RenderTargetFormat::Rgba8:
      return ImageFormat::Rgba8888;
    default:
      LOG(FATAL) << "Unsupported format: " << static_cast<int>(format);
  }
}

static filament::Texture::PixelBufferDescriptor::PixelDataFormat
ToFilamentPixelDataFormat(RenderTargetFormat format) {
  switch (format) {
    case RenderTargetFormat::Red8:
      return filament::Texture::PixelBufferDescriptor::PixelDataFormat::R;
    case RenderTargetFormat::Rgb8:
      return filament::Texture::PixelBufferDescriptor::PixelDataFormat::RGB;
    case RenderTargetFormat::Rgba8:
      return filament::Texture::PixelBufferDescriptor::PixelDataFormat::RGBA;
    default:
      LOG(FATAL) << "Unsupported format: " << static_cast<int>(format);
  }
}

FilamentRenderEngine::FilamentRenderEngine(Registry* registry)
    : RenderEngine(registry) {
  registry->RegisterDependency<DeviceManager>(static_cast<RenderEngine*>(this),
                                              true);
}

void FilamentRenderEngine::CreateFactories() {
  mesh_factory_ = registry_->Create<MeshFactory>(registry_);
  shader_factory_ = registry_->Create<ShaderFactory>(registry_);
  texture_factory_ = registry_->Create<TextureFactory>(registry_);
  render_target_factory_ = registry_->Create<RenderTargetFactory>(registry_);
}

FilamentRenderEngine::~FilamentRenderEngine() {
  SyncWait();
  scenes_.clear();
  layers_.clear();
  default_render_target_.reset();
  if (fengine_ && frenderer_) {
    fengine_->destroy(frenderer_);
  }
  if (fengine_ && fswapchain_) {
    fengine_->destroy(fswapchain_);
  }
  filament::Engine::destroy(fengine_);
}

void FilamentRenderEngine::OnRegistryInitialize() {
  auto* choreographer = registry_->Get<Choreographer>();
  if (choreographer) {
    choreographer->Add<&RenderEngine::Render>(Choreographer::Stage::kRender);
  }

  auto display = registry_->Get<DeviceManager>()->Display();
  void* native_window = display.GetProfile()->native_window;

  fengine_ = filament::Engine::create(kFilamentBackend);
  frenderer_ = fengine_->createRenderer();
  fswapchain_ = fengine_->createSwapChain(native_window);

  filament::Renderer::ClearOptions clear_options;
  clear_options.clearColor = {0.f, 0.f, 0.f, 1.f};
  clear_options.clear = true;
  clear_options.discard = true;
  frenderer_->setClearOptions(clear_options);

  auto target = std::make_shared<FilamentRenderTarget>(registry_);
  default_render_target_ = std::static_pointer_cast<RenderTarget>(target);

  auto layer = CreateRenderLayer(kDefaultName);
  auto scene = CreateRenderScene(kDefaultName);
  layer->SetScene(scene);
}

RenderLayerPtr FilamentRenderEngine::CreateRenderLayer(HashValue name) {
  auto layer =
      std::make_shared<FilamentRenderLayer>(registry_, default_render_target_);

  auto& ptr = layers_[name];
  CHECK(ptr == nullptr) << "Layer already exists: " << name.get();
  ptr = layer;
  return ptr;
}

RenderLayerPtr FilamentRenderEngine::GetRenderLayer(HashValue name) {
  auto iter = layers_.find(name);
  return iter != layers_.end() ? iter->second : nullptr;
}

RenderLayerPtr FilamentRenderEngine::GetDefaultRenderLayer() {
  return GetRenderLayer(kDefaultName);
}

RenderScenePtr FilamentRenderEngine::CreateRenderScene(HashValue name) {
  auto scene = std::make_shared<FilamentRenderScene>(registry_);

  auto& ptr = scenes_[name];
  CHECK(ptr == nullptr);
  ptr = std::static_pointer_cast<RenderScene>(scene);
  return ptr;
}

RenderScenePtr FilamentRenderEngine::GetRenderScene(HashValue name) {
  return scenes_[name];
}

RenderScenePtr FilamentRenderEngine::GetDefaultRenderScene() {
  return GetRenderScene(kDefaultName);
}

RenderablePtr FilamentRenderEngine::CreateRenderable() {
  auto renderable = std::make_shared<FilamentRenderable>(registry_);
  return std::static_pointer_cast<Renderable>(renderable);
}

LightPtr FilamentRenderEngine::CreateLight(Light::Type type) {
  auto light = std::make_shared<FilamentLight>(registry_, type);
  return std::static_pointer_cast<Light>(light);
}

IndirectLightPtr FilamentRenderEngine::CreateIndirectLight(
    const TexturePtr& reflection, const TexturePtr& irradiance) {
  auto light = std::make_shared<FilamentIndirectLight>(registry_, reflection,
                                                       irradiance);
  return std::static_pointer_cast<IndirectLight>(light);
}

bool FilamentRenderEngine::Render() {
  std::vector<FilamentRenderLayer*> layers;
  layers.reserve(layers_.size());
  for (auto& iter : layers_) {
    if (iter.second->IsEnabled()) {
      auto impl = static_cast<FilamentRenderLayer*>(iter.second.get());
      layers.emplace_back(impl);
    }
  }
  if (layers.empty()) {
    return false;
  }

  absl::c_sort(layers, [](auto* lhs, auto* rhs) {
    return lhs->GetPriority() < rhs->GetPriority();
  });

  if (!frenderer_->beginFrame(fswapchain_)) {
    return false;
  }

  for (FilamentRenderLayer* layer : layers) {
    frenderer_->render(layer->GetFilamentView());
  }

  frenderer_->endFrame();
  return true;
}

bool FilamentRenderEngine::RenderLayer(HashValue name) {
  auto iter = layers_.find(name);
  if (iter == layers_.end()) {
    return false;
  }
  if (!frenderer_->beginFrame(fswapchain_)) {
    return false;
  }

  auto* impl = static_cast<FilamentRenderLayer*>(iter->second.get());
  frenderer_->render(impl->GetFilamentView());

  frenderer_->endFrame();
  return true;
}

ImageData FilamentRenderEngine::ReadPixels(FilamentRenderTarget* target) {
  const RenderTargetFormat target_format = target->GetRenderTargetFormat();
  const int width = target->GetDimensions().x;
  const int height = target->GetDimensions().y;

  const ImageFormat output_format = ToImageFormat(target_format);
  const std::size_t bytes_per_pixel = GetBytesPerPixel(output_format);
  ImageData image(output_format, {width, height},
                  DataContainer::Allocate(width * height * bytes_per_pixel));

  const auto format = ToFilamentPixelDataFormat(target_format);
  const auto type =
      filament::Texture::PixelBufferDescriptor::PixelDataType::UBYTE;
  filament::Texture::PixelBufferDescriptor desc(
      image.GetData(), image.GetNumBytes(), format, type);

  SyncWait();
  if (!frenderer_->beginFrame(fswapchain_)) {
    LOG(FATAL) << "Unable to prepare renderer for reading pixels.";
  }

  frenderer_->readPixels(target->GetFilamentRenderTarget(), 0, 0, width,
                                            height, std::move(desc));
  frenderer_->endFrame();
  SyncWait();
  return image;
}

void FilamentRenderEngine::SyncWait() {
  filament::Fence::waitAndDestroy(fengine_->createFence());
}

void RenderEngine::Create(Registry* registry) {
  auto ptr = new FilamentRenderEngine(registry);
  registry->Register(std::unique_ptr<RenderEngine>(ptr));
  // We need to delay the creation of factories until after the engine is in
  // the registry. Otherwise, the engine will get destroyed before the factories
  // during shutdown.
  ptr->CreateFactories();
}

static StaticRegistry Static_Register(RenderEngine::Create);

}  // namespace redux
