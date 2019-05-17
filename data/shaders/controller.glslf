#define MAX_TOUCHES 1
#include "lullaby/data/shaders/uber_fragment_common.glslh"

STAGE_INPUT lowp vec4 vColor;

uniform sampler2D texture_unit_0;

uniform vec4 touchpad_rect;
uniform vec2 touch_position;
uniform float touch_radius_squared;
uniform vec4 touch_color;

uniform vec4 battery_uv_rect;
uniform vec2 battery_offset;

// Returns true if point is inside box.
// Expects rect as xmin, ymin, xmax, ymax.
bool inRect(vec2 point, vec4 rect) {
  vec2 result = step(rect.xy, point) - (vec2(1.0, 1.0) - step(point, rect.zw));
  // Output of step should be 0 or 1 for each value.  Check against 0.5 to
  // avoid any possibility of floating point error.
  return result.x * result.y > 0.5;
}

void main() {
  // Do a dither based transparency, since the controller should be in an
  // opaque pass.
  lowp vec4 set_color = GetColor();
  if (set_color.a < Dither4x4()) {
    discard;
  }
  set_color.a = 1.0;

  vec2 uv = GetVTexCoord();

  // Look up texture color.  Using the offset from the battery causes a
  // discontinuity in the mipmapping, so we have to bias.
  lowp vec4 texture_color = inRect(uv, battery_uv_rect) ?
      texture2DWithBias(texture_unit_0, uv + battery_offset, -20.0):
      texture2D(texture_unit_0, uv);


  texture_color = PremultiplyAlpha(set_color) * texture_color;

  // Apply the touchpad touch.
  lowp vec4 color = vColor;
  vec2 diff = uv - touch_position;
  float len_squared = dot(diff, diff);
  if (len_squared <= touch_radius_squared && inRect(uv, touchpad_rect)) {
    color = mix(
        color, touch_color,
        touch_color.a *
            (1.0 - smoothstep(0.8, 1.0, len_squared / touch_radius_squared)));
  }

  SetFragColor(mix(texture_color, color, color.a));
}
