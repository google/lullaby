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

#include "lullaby/systems/render/filament/sceneview.h"

#include "filament/IndirectLight.h"
#include "lullaby/systems/render/filament/filament_utils.h"
#include "lullaby/systems/render/filament/texture.h"
#include "lullaby/systems/render/filament/texture_factory.h"
#include "lullaby/util/math.h"

namespace lull {

SceneView::SceneView(Registry* registry, filament::Engine* engine)
    : registry_(registry), engine_(engine) {
  scene_ = engine_->createScene();
}

SceneView::~SceneView() {
  for (auto& iter : lights_) {
    utils::Entity light = iter.second;
    scene_->remove(light);
    engine_->getLightManager().destroy(light);
    utils::EntityManager::get().destroy(light);
  }
  engine_->destroy(indirect_light_);
  for (auto& camera : cameras_) {
    engine_->destroy(camera);
  }
  for (auto& view : views_) {
    engine_->destroy(view);
  }
  engine_->destroy(scene_);
}

filament::View* SceneView::GetView(size_t index) {
  return index < views_.size() ? views_[index] : nullptr;
}

filament::Scene* SceneView::GetScene() { return scene_; }

void SceneView::Prepare(const mathfu::vec4& clear_color,
                        Span<RenderView> render_views) {
  if (views_.empty()) {
    InitializeViews(clear_color, render_views.size());
  }

  auto& light_manager = engine_->getLightManager();
  auto* transform_system = registry_->Get<TransformSystem>();
  for (auto& iter : lights_) {
    const Entity entity = iter.first;
    const mathfu::mat4* transform =
        transform_system->GetWorldFromEntityMatrix(entity);
    if (transform) {
      const Sqt sqt = CalculateSqtFromMatrix(transform);
      const auto i = light_manager.getInstance(iter.second);
      const auto pos = FilamentFloat3FromMathfuVec3(sqt.translation);
      const auto dir =
          FilamentFloat3FromMathfuVec3(sqt.rotation * -mathfu::kAxisZ3f);

      light_manager.setPosition(i, pos);
      light_manager.setDirection(i, dir);
    }
  }

  DCHECK(views_.size() == render_views.size());
  for (size_t i = 0; i < render_views.size(); ++i) {
    const RenderView& render_view = render_views[i];
    filament::View* filament_view = views_[i];
    filament_view->setScene(scene_);
    filament_view->setViewport(
        {render_view.viewport.x, render_view.viewport.y,
         static_cast<uint32_t>(render_view.dimensions.x),
         static_cast<uint32_t>(render_view.dimensions.y)});
    // TODO:  Use non hardcoded near and far clipping values.
    cameras_[i]->setCustomProjection(
        MathFuMat4ToFilamentMat4(render_view.clip_from_eye_matrix), .1, 1000);
    cameras_[i]->setModelMatrix(
        MathFuMat4ToFilamentMat4f(render_view.world_from_eye_matrix));
  }
}

void SceneView::InitializeViews(const mathfu::vec4& clear_color, size_t count) {
  static const float kCameraAperture = 16.f;
  static const float kCameraShutterSpeed = 1.0f / 125.0f;
  static const float kCameraSensitivity = 100.0f;

  cameras_.resize(count);
  views_.resize(count);
  for (size_t i = 0; i < views_.size(); ++i) {
    cameras_[i] = engine_->createCamera();
    cameras_[i]->setExposure(kCameraAperture, kCameraShutterSpeed,
                             kCameraSensitivity);
    views_[i] = engine_->createView();
    views_[i]->setClearColor(ToLinearColorA(clear_color));
    views_[i]->setName("Filament View");
    views_[i]->setCamera(cameras_[i]);
    views_[i]->setDepthPrepass(filament::View::DepthPrepass::DISABLED);
  }
}

void SceneView::CreateLight(Entity entity,
                            filament::LightManager::Builder& builder) {
  utils::Entity light = utils::EntityManager::get().create();
  builder.build(*engine_, light);
  scene_->addEntity(light);
  lights_.emplace(entity, light);
}

void SceneView::DestroyLight(Entity entity) {
  auto iter = lights_.find(entity);
  if (iter != lights_.end()) {
    utils::Entity light = iter->second;
    scene_->remove(light);
    engine_->getLightManager().destroy(light);
    utils::EntityManager::get().destroy(light);
    lights_.erase(iter);
  }
}

filament::LinearColor ToLinearColor(const Color* color) {
  if (color) {
    mathfu::vec4 tmp(color->r(), color->g(), color->b(), color->a());
    return ToLinearColor(tmp);
  }
  return ToLinearColor(mathfu::kOnes4f);
}

void SceneView::CreateLight(Entity entity, const DirectionalLightDef& light) {
  filament::LightManager::Builder builder(
      filament::LightManager::Type::DIRECTIONAL);
  builder.color(ToLinearColor(light.color()));

  const ShadowMapDef* shadow_def = light.shadow_def_as_ShadowMapDef();
  if (shadow_def) {
    filament::LightManager::ShadowOptions shadow;
    shadow.mapSize = shadow_def->shadow_resolution();
    shadow.shadowNearHint = shadow_def->shadow_min_distance();
    shadow.shadowFar = shadow_def->shadow_max_distance();
    builder.shadowOptions(shadow);
    builder.castShadows(true);
  }
  CreateLight(entity, builder);
}

void SceneView::CreateLight(Entity entity, const PointLightDef& light) {
  filament::LightManager::Builder builder(filament::LightManager::Type::POINT);
  builder.color(ToLinearColor(light.color()));
  builder.intensity(light.intensity());
  CreateLight(entity, builder);
}

void SceneView::CreateLight(Entity entity, const EnvironmentLightDef& light) {
  if (indirect_light_) {
    scene_->setIndirectLight(nullptr);
    engine_->destroy(indirect_light_);
  }

  indirect_light_ = nullptr;
  ibl_reflection_ = nullptr;
  ibl_irradiance_ = nullptr;

  auto* texture_factory = registry_->Get<TextureFactory>();
  if (light.diffuse()) {
    ibl_reflection_ = texture_factory->CreateTexture(light.diffuse());
  }
  if (light.specular()) {
    ibl_irradiance_ = texture_factory->CreateTexture(light.specular());
  }

  auto callback = [=]() {
    // The indirect light has already been set (probably by the first callback)
    // so no need to do anything now.
    if (indirect_light_) {
      return;
    }
    if (ibl_reflection_ && !ibl_reflection_->IsLoaded()) {
      return;
    }
    if (ibl_irradiance_ && !ibl_irradiance_->IsLoaded()) {
      return;
    }

    filament::IndirectLight::Builder builder;
    builder.reflections(ibl_reflection_->GetFilamentTexture());
    if (ibl_irradiance_) {
      builder.irradiance(ibl_irradiance_->GetFilamentTexture());
    } else {
      // TODO: add intensity to EnvironmentLightDef (or read from the ktx
      // texture data if it is available there).
      // clang-format off
      std::array<filament::math::float3, 9> sh_coefficients = {
        filament::math::float3{ 0.592915142902302,  0.580783147865357,  0.564906236122309}, // L00, irradiance, pre-scaled base
        filament::math::float3{ 0.038230073440953,  0.040661612793765,  0.045912497583365}, // L1-1, irradiance, pre-scaled base
        filament::math::float3{-0.306182569332798, -0.298728189882871, -0.292527808646246}, // L10, irradiance, pre-scaled base
        filament::math::float3{-0.268674829827722, -0.258309969107310, -0.244936138194592}, // L11, irradiance, pre-scaled base
        filament::math::float3{ 0.055981897791156,  0.053190319920282,  0.047808414744011}, // L2-2, irradiance, pre-scaled base
        filament::math::float3{ 0.009835221123367,  0.006544190646597,  0.000350193519574}, // L2-1, irradiance, pre-scaled base
        filament::math::float3{ 0.017525154215762,  0.017508716588022,  0.018218263542429}, // L20, irradiance, pre-scaled base
        filament::math::float3{ 0.306912095635860,  0.292384283162994,  0.274657325943371}, // L21, irradiance, pre-scaled base
        filament::math::float3{ 0.055928224084081,  0.051564836176893,  0.044938623517990} // L22, irradiance, pre-scaled base
                                  // clang-format on
      };
      builder.irradiance(3, sh_coefficients.data());
    }
    // TODO: add intensity to EnvironmentLightDef.
    builder.intensity(30000);
    indirect_light_ = builder.build(*engine_);
    scene_->setIndirectLight(indirect_light_);
  };

  // Must have at least a reflection map to use IBL.
  if (ibl_reflection_) {
    ibl_reflection_->AddOrInvokeOnLoadCallback(callback);
    if (ibl_irradiance_) {
      ibl_irradiance_->AddOrInvokeOnLoadCallback(callback);
    }
  }
}
}  // namespace lull
