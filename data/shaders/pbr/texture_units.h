#ifndef LULLABY_DATA_SHADERS_PBR_TEXTURE_UNITS_H_
#define LULLABY_DATA_SHADERS_PBR_TEXTURE_UNITS_H_

namespace lull {
// List of texture units meant to be assigned in C++. There are other texture
// units that come ahead of these, but they are implicitly populated by
// RenderDefs, so we have no control over them.



////////////////////////////////////////////////////////////////////////////////
// The following units are available to shaders derived from
// lullaby/data/shaders/pbr.
////////////////////////////////////////////////////////////////////////////////
enum PbrTextureUnits : int {
  // Environment cube map convolved for specular reflection in RGB.
  kSpecEnvMapUnit = 4,
  // Shadow map (hard-coded by Lullaby, reserved for future use).
  kShadowMapUnit = 5,
  // Environment cube map convolved for diffuse reflection in RGB.
  kDiffEnvMapUnit = 6,
  // Filtered camera feed in RGB.
  kCameraFeedMapUnit = 7,
  // Source for camera feed. On Android, this is an external texture; on desktop
  // with AR Anywhere, it is a loaded image simulating the camera feed.
  kInputCameraFeedMapUnit = 8,
  // MotionStereo depth map in RGBA (loaded as luminance + alpha).
  kCameraDepthMapUnit = 9,
};


}  // namespace lull

#endif  // LULLABY_DATA_SHADERS_PBR_TEXTURE_UNITS_H_
