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

#include "redux/engines/render/filament/filament_render_layer.h"

#include <stdint.h>

#include <memory>

#include "filament/Color.h"
#include "filament/Options.h"
#include "filament/View.h"
#include "filament/Viewport.h"
#include "math/mat4.h"
#include "utils/EntityManager.h"
#include "redux/engines/render/filament/filament_render_engine.h"
#include "redux/engines/render/filament/filament_render_scene.h"
#include "redux/engines/render/filament/filament_render_target.h"
#include "redux/engines/render/filament/filament_utils.h"
#include "redux/engines/render/render_layer_options.h"
#include "redux/engines/render/render_scene.h"
#include "redux/engines/render/render_target.h"
#include "redux/modules/base/registry.h"
#include "redux/modules/math/bounds.h"
#include "redux/modules/math/matrix.h"
#include "redux/modules/math/vector.h"

namespace redux {

static filament::QualityLevel ToFilament(const RenderQualityLevel& level) {
  switch (level) {
    case RenderQualityLevel::kUltra:
      return filament::QualityLevel::ULTRA;
    case RenderQualityLevel::kHigh:
      return filament::QualityLevel::HIGH;
    case RenderQualityLevel::kMedium:
      return filament::QualityLevel::MEDIUM;
    case RenderQualityLevel::kLow:
      return filament::QualityLevel::LOW;
    }
}

static filament::DepthOfFieldOptions::Filter ToFilament(
    const DepthOfFieldOptions::Filter& filter) {
  switch (filter) {
    case DepthOfFieldOptions::Filter::kNone:
      return filament::DepthOfFieldOptions::Filter::NONE;
    case DepthOfFieldOptions::Filter::kMedian:
      return filament::DepthOfFieldOptions::Filter::MEDIAN;
  }
}

static filament::BloomOptions::BlendMode ToFilament(
    const BloomOptions::BlendMode& mode) {
  switch (mode) {
    case BloomOptions::BlendMode::kAdd:
      return filament::BloomOptions::BlendMode::ADD;
    case BloomOptions::BlendMode::kInterpolate:
      return filament::BloomOptions::BlendMode::INTERPOLATE;
  }
}


FilamentRenderLayer::FilamentRenderLayer(Registry* registry,
                                         RenderTargetPtr target)
    : render_target_(target) {
  auto fengine = redux::GetFilamentEngine(registry);
  fview_ = MakeFilamentResource(fengine->createView(), fengine);
  fview_->setShadowingEnabled(true);

  static const float kCameraAperture = 16.f;
  static const float kCameraShutterSpeed = 1.0f / 125.0f;
  static const float kCameraSensitivity = 100.0f;
  auto camera_entity = utils::EntityManager::get().create();
  fcamera_ =
      MakeFilamentResource(fengine->createCamera(camera_entity), fengine);
  fcamera_->setExposure(kCameraAperture, kCameraShutterSpeed,
                        kCameraSensitivity);
  fview_->setCamera(fcamera_.get());

  SetViewport({vec2::Zero(), vec2::One()});
}

void FilamentRenderLayer::Enable() { enabled_ = true; }

void FilamentRenderLayer::Disable() { enabled_ = false; }

bool FilamentRenderLayer::IsEnabled() const { return enabled_; }

void FilamentRenderLayer::SetPriority(int priority) { priority_ = priority; }

int FilamentRenderLayer::GetPriority() const { return priority_; }

void FilamentRenderLayer::SetClipPlaneDistances(float near, float far) {
  near_plane_ = near;
  far_plane_ = far;
}

void FilamentRenderLayer::SetViewport(const Bounds2f& viewport) {
  viewport_ = viewport;

  const vec2i target_size = render_target_->GetDimensions();
  filament::Viewport vp;
  vp.left = static_cast<int32_t>(viewport.min.x * target_size.x);
  vp.bottom = static_cast<int32_t>(viewport.min.y * target_size.y);
  vp.width = static_cast<uint32_t>(viewport.Size().x * target_size.x);
  vp.height = static_cast<uint32_t>(viewport.Size().y * target_size.y);
  fview_->setViewport(vp);
}

Bounds2i FilamentRenderLayer::GetAbsoluteViewport() const {
  const filament::Viewport& vp = fview_->getViewport();
  Bounds2i res;
  res.min = vec2i{vp.left, vp.bottom};
  res.max = vec2i{static_cast<int>(vp.width), static_cast<int>(vp.height)};
  return res;
}

void FilamentRenderLayer::SetRenderTarget(RenderTargetPtr target) {
  auto impl = static_cast<FilamentRenderTarget*>(target.get());
  fview_->setRenderTarget(impl ? impl->GetFilamentRenderTarget() : nullptr);
  render_target_ = target;
  SetViewport(viewport_);
}

void FilamentRenderLayer::SetScene(const RenderScenePtr& scene) {
  auto impl = static_cast<FilamentRenderScene*>(scene.get());
  fview_->setScene(impl ? impl->GetFilamentScene() : nullptr);
}

void FilamentRenderLayer::SetViewMatrix(const mat4& view_matrix) {
  fcamera_->setModelMatrix(ToFilament(view_matrix));
}

void FilamentRenderLayer::SetProjectionMatrix(const mat4& projection_matrix) {
  fcamera_->setCustomProjection(
      filament::math::mat4(ToFilament(projection_matrix)), near_plane_,
      far_plane_);
}

void FilamentRenderLayer::SetCameraExposure(float aperture, float shutter_speed,
                                    float iso_sensitivity) {
  fcamera_->setExposure(aperture, shutter_speed, iso_sensitivity);
}

void FilamentRenderLayer::SetCameraFocalDistance(float focus_distance) {
  fcamera_->setFocusDistance(focus_distance);
}

void FilamentRenderLayer::EnableAntiAliasing(
    const MultiSampleAntiAliasingOptions& opts) {
  fview_->setMultiSampleAntiAliasingOptions({
      .enabled = true,
      .sampleCount = opts.sample_count,
      .customResolve = opts.hdr_custom_resolve,
  });
}

void FilamentRenderLayer::DisableAntiAliasing() {
  fview_->setMultiSampleAntiAliasingOptions({
      .enabled = false,
  });
}

void FilamentRenderLayer::EnableDepthOfField(const DepthOfFieldOptions& opts) {
  fview_->setDepthOfFieldOptions({
      .cocScale = opts.circle_of_confusion_scale,
      .cocAspectRatio = opts.circle_of_confusion_aspect_ratio,
      .maxApertureDiameter = opts.max_aperture_diameter,
      .enabled = true,
      .filter = ToFilament(opts.filter),
      .nativeResolution = opts.use_native_resolution,
      .foregroundRingCount = opts.foreground_ring_count,
      .backgroundRingCount = opts.background_ring_count,
      .fastGatherRingCount = opts.fast_gather_ring_count,
      .maxForegroundCOC = opts.max_foreground_circle_of_confusion_pixels,
      .maxBackgroundCOC = opts.max_background_circle_of_confusion_pixels,
  });
}

void FilamentRenderLayer::DisableDepthOfField() {
  fview_->setDepthOfFieldOptions({
      .enabled = false,
  });
}

void FilamentRenderLayer::EnableVignette(const VignetteOptions& opts) {
  fview_->setVignetteOptions({
      .midPoint = opts.mid_point,
      .roundness = opts.roundness,
      .feather = opts.feather,
      .color = ToFilament(opts.color),
      .enabled = true,
  });
}

void FilamentRenderLayer::DisableVignette() {
  fview_->setVignetteOptions({
      .enabled = false,
  });
}

void FilamentRenderLayer::EnableBloom(const BloomOptions& opts) {
  fview_->setBloomOptions({
      .strength = opts.strength,
      .resolution = opts.resolution,
      .levels = opts.levels,
      .blendMode = ToFilament(opts.blend_mode),
      .enabled = true,
      .highlight = opts.highlight,
      .quality = ToFilament(opts.quality),
      .lensFlare = opts.lens_flare,
      .starburst = opts.starburst,
      .chromaticAberration = opts.chromatic_aberration,
      .ghostCount = opts.ghost_count,
      .ghostSpacing = opts.ghost_spacing,
      .ghostThreshold = opts.ghost_threshold,
      .haloThickness = opts.halo_thickness,
      .haloRadius = opts.halo_radius,
      .haloThreshold = opts.halo_threshold,
  });
}

void FilamentRenderLayer::DisableBloom() {
  fview_->setBloomOptions({
      .enabled = false,
  });
}

void FilamentRenderLayer::EnableFog(const FogOptions& opts) {
  fview_->setFogOptions({
      .distance = opts.distance,
      .cutOffDistance = opts.cut_off_distance,
      .maximumOpacity = opts.maximum_opacity,
      .height = opts.height,
      .heightFalloff = opts.height_falloff,
      .color = filament::LinearColor{opts.color.r, opts.color.g, opts.color.b},
      .density = opts.extinction_factor,
      .inScatteringStart = opts.in_scattering_start,
      .inScatteringSize = opts.in_scattering_size,
      .fogColorFromIbl = opts.use_ibl_for_fog_color,
      .enabled = true,
  });
}

void FilamentRenderLayer::DisableFog() {
  fview_->setFogOptions({
      .enabled = false,
  });
}

void FilamentRenderLayer::EnableAmbientOcclusion(
    const AmbientOcclusionOptions& opts) {
  fview_->setAmbientOcclusionOptions({
      .radius = opts.radius,
      .power = opts.power,
      .bias = opts.bias,
      .resolution = opts.resolution,
      .intensity = opts.intensity,
      .bilateralThreshold = opts.bilateral_threshold,
      .quality = ToFilament(opts.quality),
      .lowPassFilter = ToFilament(opts.low_pass_filter),
      .upsampling = ToFilament(opts.upsampling),
      .enabled = true,
      .bentNormals = opts.bent_normals,
      .minHorizonAngleRad = opts.min_horizon_radians,
  });
}

void FilamentRenderLayer::DisableAmbientOcclusion() {
  fview_->setAmbientOcclusionOptions({
      .enabled = false,
  });
}

void FilamentRenderLayer::EnableScreenSpaceConeTracing(
    const ScreenSpaceConeTracingOptions& opts) {
  auto ao_options = fview_->getAmbientOcclusionOptions();
  ao_options.ssct = filament::AmbientOcclusionOptions::Ssct{
      .lightConeRad = opts.light_cone_angle,
      .shadowDistance = opts.shadow_distance,
      .contactDistanceMax = opts.max_contact_distance,
      .intensity = opts.intensity,
      .lightDirection = ToFilament(opts.light_direction),
      .depthBias = opts.depth_bias,
      .depthSlopeBias = opts.depth_slope_bias,
      .sampleCount = opts.sample_count,
      .rayCount = opts.ray_count,
      .enabled = true,
  };
  fview_->setAmbientOcclusionOptions(ao_options);
}

void FilamentRenderLayer::DisableScreenSpaceConeTracing() {
  auto ao_options = fview_->getAmbientOcclusionOptions();
  ao_options.ssct = filament::AmbientOcclusionOptions::Ssct{
      .enabled = false,
  };
  fview_->setAmbientOcclusionOptions(ao_options);
}

void FilamentRenderLayer::EnableScreenSpaceReflections(
    const ScreenSpaceReflectionsOptions& opts) {
  fview_->setScreenSpaceReflectionsOptions({
      .thickness = opts.thickness,
      .bias = opts.bias,
      .maxDistance = opts.max_distance,
      .stride = opts.stride,
      .enabled = true,
  });
}

void FilamentRenderLayer::DisableScreenSpaceReflections() {
  fview_->setScreenSpaceReflectionsOptions({
      .enabled = false,
  });
}

void FilamentRenderLayer::EnablePostProcessing() {
  fview_->setPostProcessingEnabled(true);
}

void FilamentRenderLayer::DisablePostProcessing() {
  fview_->setPostProcessingEnabled(false);
}

}  // namespace redux
