// This file defines several default values for use in widget templates.
// Any time they are used, the arguments to the function should be able to
// override them.
// All distance values are in dmm (aside from 'depth').
{
  depth: 2.5,
  epsilon: 0.0001,

  // Note: cursor will be scaled differently if another value is used.
  cursor_no_hit_distance: 2.7,

  quad: {
    small_corner_radius: 0.004,
    small_corner_verts: 2,

    medium_corner_radius: 0.006,
    medium_corner_verts: 3,
  },

  depths: {
    flat_z: 0.0,
    raised_z: 0.016,
    hover_z: 0.032,
  },

  // A shadow for rects with 4 dmm corner rounding and 0.5 to 8 dmm blur.
  // Texture is intended for 2 pixels per dmm. (original_size 0.32 x 0.32)
  shadow_4_8: {
    sprite: "COMPOSITE_SHADOW_4_8",
    file: "textures/composite_shadow_4_8.webp",
    padding: 0.008,
    nine_patch_original_size: {x: 0.032, y: 0.032},
  },

  fonts: {
    title_font: ["/system/fonts/Roboto-Medium.ttf"],
    title_font_size: 0.032,

    body_font: ["/system/fonts/Roboto-Regular.ttf"],
    body_font_size: 0.024,
    body_font_line_height: 0.028 / 0.024,

    button_font: ["/system/fonts/Roboto-Medium.ttf"],
    button_font_size: 0.024,
  },
}
