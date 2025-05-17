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

#ifndef REDUX_ENGINES_RENDER_RENDER_LAYER_OPTIONS_H_
#define REDUX_ENGINES_RENDER_RENDER_LAYER_OPTIONS_H_

#include <stdint.h>

#include <limits>

#include "redux/modules/graphics/color.h"
#include "redux/modules/math/vector.h"

namespace redux {

enum class RenderQualityLevel : uint8_t {
  kLow,
  kMedium,
  kHigh,
  kUltra,
};

struct MultiSampleAntiAliasingOptions {
  // Number of samples to use for multi-sampled anti-aliasing.
  uint8_t sample_count = 4;

  // Custom resolve improves quality for HDR scenes, but may impact performance.
  bool hdr_custom_resolve = false;
};

struct DepthOfFieldOptions {
  enum class Filter : uint8_t {
    kNone,
    kMedian,
  };

  // Circle-of-confusion scale factor (amount of blur). Can be used to set the
  // depth of field blur independently from the camera aperture. This can be
  // achieved by setting this value to:
  //     camera_aperturre / desired_depth_of_field_aperture
  float circle_of_confusion_scale = 1.0f;

  // Width/height aspect ratio of the circle-of-confusion (to simulate
  // anamorphic lenses).
  float circle_of_confusion_aspect_ratio = 1.0f;

  // Maximum circle-of-confusion in pixels for the foreground, must be in [0,
  // 32] range. A value of 0 means default, which is 32 on desktop and 24 on
  // mobile.
  uint16_t max_foreground_circle_of_confusion_pixels = 0;

  // Maximum circle-of-confusion in pixels for the background, must be in [0,
  // 32] range. A value of 0 means default, which is 32 on desktop and 24 on
  // mobile.
  uint16_t max_background_circle_of_confusion_pixels = 0;

  // Maximum aperture diameter in meters (zero to disable rotation).
  float max_aperture_diameter = 0.01f;

  // Filter to use for filling gaps in the kernel.
  Filter filter = Filter::kMedian;

  // Perform DoF processing at native resolution
  bool use_native_resolution = false;

  // Number of of rings used by the gather kernels. The number of rings affects
  // quality and performance. The actual number of sample per pixel is defined
  // as (ringCount * 2 - 1)^2. Here are a few commonly used values:
  //       3 rings :   25 ( 5x 5 grid)
  //       4 rings :   49 ( 7x 7 grid)
  //       5 rings :   81 ( 9x 9 grid)
  //      17 rings : 1089 (33x33 grid)
  //
  // With a maximum circle-of-confusion of 32, it is never necessary to use more
  // than 17 rings.
  //
  // Usually all three settings below are set to the same value, however, it is
  // often acceptable to use a lower ring count for the "fast tiles", which
  // improves performance. Fast tiles are regions of the screen where every
  // pixels have a similar circle-of-confusion radius.
  //
  // A value of 0 means default, which is 5 on desktop and 3 on mobile.

  // Number of kernel rings for foreground tiles.
  uint8_t foreground_ring_count = 0;

  // Number of kernel rings for background tiles.
  uint8_t background_ring_count = 0;

  // Number of kernel rings for fast tiles.
  uint8_t fast_gather_ring_count = 0;
};

struct VignetteOptions {
  // Color of the vignette effect, alpha is currently ignored
  Color4f color = {0.0f, 0.0f, 0.0f, 1.0f};

  // High values restrict the vignette closer to the corners, between 0 and 1.
  float mid_point = 0.5f;

  // Controls the shape of the vignette, from a rounded rectangle (0.0), to
  // an oval (0.5), to a circle (1.0).
  float roundness = 0.5f;

  // Softening amount of the vignette effect, between 0 and 1.
  float feather = 0.5f;
};

struct BloomOptions {
  enum class BlendMode : uint8_t {
    // Modulate bloom by the strength parameter and add to the scene.
    kAdd,
    // Interpolate bloom with the scene using the strength parameter.
    kInterpolate,
  };

  // How much of the bloom is added to the original image. Between 0 and 1.
  float strength = 0.10f;

  // Resolution of bloom's minor axis. The minimum value is 2^levels and the
  // the maximum is lower of the original resolution and 4096. This parameter is
  // silently clamped to the minimum and maximum.
  // It is highly recommended that this value be smaller than the target
  // resolution after dynamic resolution is applied.
  uint32_t resolution = 384;

  // Number of successive blurs to achieve the blur effect, the minimum is 3 and
  // the maximum is 12. This value together with resolution influences the
  // spread of the blur effect. This value can be silently reduced to
  // accommodate the original image size.
  uint8_t levels = 6;

  // Whether the bloom effect is purely additive or mixed with the original
  // image.
  BlendMode blend_mode = BlendMode::kAdd;  // how the bloom effect is applied

  // Limits highlights to this value before bloom [10, +inf]
  float highlight = 1000.0f;

  // Bloom quality level.
  //   kLow: use a more optimized down-sampling filter, however there can be
  //     artifacts with dynamic resolution, this can be alleviated by using the
  //     homogenous mode.
  //  kMedium: Good balance between quality and performance.
  //  kHigh: In this mode the bloom resolution is automatically increased to
  //    avoid artifacts. This mode can be significantly slower on mobile,
  //    especially at high resolution. This mode greatly improves the anamorphic
  //    bloom.
  RenderQualityLevel quality = RenderQualityLevel::kLow;

  // Enable screen-space lens flare.
  bool lens_flare = false;

  // Enable starburst effect on lens flare.
  bool starburst = true;

  // Amount of chromatic aberration.
  float chromatic_aberration = 0.005f;

  // Number of flare "ghosts".
  uint8_t ghost_count = 4;

  // Spacing of the ghost in screen units [0, 1].
  float ghost_spacing = 0.6f;

  // HRD threshold for the ghosts.
  float ghost_threshold = 10.0f;

  // Thickness of halo in vertical screen units, 0 to disable.
  float halo_thickness = 0.1f;

  // Radius of halo in vertical screen units [0, 0.5].
  float halo_radius = 0.4f;

  // HDR threshold for the halo.
  float halo_threshold = 10.0f;
};

// Options to control large-scale fog in the scene
struct FogOptions {
  // Distance in world units from the camera to where the fog starts.
  float distance = 0.0f;

  // Distance in world units [m] after which the fog calculation is disabled.
  // This can be used to exclude the skybox, which is desirable if it already
  // contains clouds or fog. The default value is +infinity which applies the
  // fog to everything.
  //
  // Note: The SkyBox is typically at a distance of 1e19 in world space
  // (depending on the near plane distance and projection used though).
  float cut_off_distance = std::numeric_limits<float>::infinity();

  // fog's maximum opacity between 0 and 1
  float maximum_opacity = 1.0f;

  // Fog's floor in world units [m]. This sets the "sea level".
  float height = 0.0f;

  // How fast the fog dissipates with altitude. heightFalloff has a unit of
  // [1/m]. It can be expressed as 1/H, where H is the altitude change in world
  // units [m] that causes a factor 2.78 (e) change in fog density.
  //
  // A falloff of 0 means the fog density is constant everywhere and may result
  // is slightly faster computations.
  float height_falloff = 1.0f;

  // Fog's color is used for ambient light in-scattering, a good value is
  // to use the average of the ambient light, possibly tinted towards blue
  // for outdoors environments. Color component's values should be between 0 and
  // 1, values above one are allowed but could create a non energy-conservative
  // fog (this is dependant on the IBL's intensity as well).
  //
  // We assume that our fog has no absorption and therefore all the light it
  // scatters out becomes ambient light in-scattering and has lost all
  // directionality, i.e.: scattering is isotropic. This somewhat simulates
  // Rayleigh scattering.
  //
  // This value is used as a tint instead, when use_ibl_for_fog_color is
  // enabled.
  Color4f color = {1.0f, 1.0f, 1.0f, 1.0f};

  // The fog color will be sampled from the IBL in the view direction and tinted
  // by `color`. Depending on the scene this can produce very convincing
  // results.
  //
  // This simulates a more anisotropic phase-function.
  bool use_ibl_for_fog_color = false;

  // Extinction factor in [1/m] at altitude 'height'. The extinction factor
  // controls how much light is absorbed and out-scattered per unit of distance.
  // Each unit of extinction reduces the incoming light to 37% of its original
  // value.
  //
  // Note: The extinction factor is related to the fog density, it's usually
  // some constant K times the density at sea level (more specifically at fog
  // height). The constant K depends on the composition of the fog/atmosphere.
  float extinction_factor = 0.1f;

  // Distance in world units [m] from the camera where the Sun in-scattering
  // starts.
  float in_scattering_start = 0.0f;

  // Very inaccurately simulates the Sun's in-scattering. That is, the light
  // from the sun that is scattered (by the fog) towards the camera. Size of the
  // Sun in-scattering (>0 to activate). Good values are >> 1 (e.g. ~10 - 100).
  // Smaller values result is a larger scattering size.
  float in_scattering_size = -1.0f;
};

struct AmbientOcclusionOptions {
  // Occlusion radius in meters, between 0 and ~10.
  float radius = 0.3f;

  // Controls ambient occlusion's contrast. Must be positive.
  float power = 1.0f;

  // Self-occlusion bias in meters. Use to avoid self-occlusion. Between 0 and
  // a few mm.
  float bias = 0.0005f;

  // How each dimension of the ambient occlusion buffer is scaled. Must be
  // either 0.5 or 1.0.
  float resolution = 0.5f;

  // Strength of the ambient occlusion effect.
  float intensity = 1.0f;

  // Depth distance that constitute an edge for filtering.
  float bilateral_threshold = 0.05f;

  // Minimum angle (in radians) to consider.
  float min_horizon_radians = 0.0f;

  // Enables bent normals computation from ambient occlusion, and specular
  // ambient occlusion.
  bool bent_normals = false;

  // Affects the number of samples used for ambient occlusion.
  RenderQualityLevel quality = RenderQualityLevel::kLow;

  // Affects ambient occlusion smoothness.
  RenderQualityLevel low_pass_filter = RenderQualityLevel::kMedium;

  // Affects ambient occlusion buffer upsampling quality.
  RenderQualityLevel upsampling = RenderQualityLevel::kLow;
};

// Ambient shadows from dominant light. Requires Ambient Occlusion to be
// enabled.
struct ScreenSpaceConeTracingOptions {
  // Full cone angle (in radians), between 0 and pi/2
  float light_cone_angle = 1.0f;

  // How far shadows can be cast.
  float shadow_distance = 0.3f;

  // Max distance for contact.
  float max_contact_distance = 1.0f;

  // Intensity.
  float intensity = 0.8f;

  // Light direction.
  vec3 light_direction = {0, -1, 0};

  // Depth bias in world units (to mitigate self shadowing).
  float depth_bias = 0.01f;

  // Depth slope bias (to mitigate self shadowing).
  float depth_slope_bias = 0.01f;

  // Tracing sample count, between 1 and 255.s
  uint8_t sample_count = 4;

  // Number of rays to trace, between 1 and 255.
  uint8_t ray_count = 1;
};

struct ScreenSpaceReflectionsOptions {
  // Ray thickness, in world units.
  float thickness = 0.1f;

  // Bias, in world units, to prevent self-intersections.
  float bias = 0.01f;

  // Maximum distance, in world units, to raycast.
  float max_distance = 3.0f;

  // Stride, in texels, for samples along the ray.
  float stride = 2.0f;
};

}  // namespace redux

#endif  // REDUX_ENGINES_RENDER_RENDER_LAYER_OPTIONS_H_
