local colors = import "colors.jsonnet";
local utils = import "lullaby/data/jsonnet/utils.jsonnet";

{
  cursor(args)::
   local cursor_trail_def = if args.enable_trail == true then [{
    "def_type": "CursorTrailDef",
    "def": {
      "average_trail_length": utils.select(14, args, "average_trail_length"),
      "average_speed": utils.select(0.2, args, "average_trail_speed"),
      "quad_size": utils.select(0.05, args, "trail_size"),
      "curve_samples": utils.select(4, args, "trail_curve_samples"),
      "default_color": utils.select(colors.kWhite, args, "trail_color"),
      "max_trail_length": utils.select(32, args, "max_trail_length")
    }
  }] else [];

  [{
    "def_type": "TransformDef",
    "def": {
      "position": {
        "x": 0.0,
        "y": 0.0,
        "z": 0.0
      },
      "rotation": {
        "x": 0.0,
        "y": 0.0,
        "z": 0.0
      },
      "scale": {
        "x": 1.0,
        "y": 1.0,
        "z": 1.0
      }
    }
    }, {
    "def_type": "RenderDef",
    "def": {
      "quad": {
        "size_x": utils.select(0.05, args, "size"),
        "size_y": self.size_x,
        "verts_x": 2,
        "verts_y": 2,
        "has_uv": true,
        "corner_radius": 0,
        "corner_verts": 0
      },
    "shader": "shaders/reticle.fplshader",
    "pass": utils.select("OverDraw", args, "render_pass"),
    }
    }, {
    "def_type": "CursorDef",
    "def": {
      "ring_active_diameter": utils.select(1.0, args, "ring_active_diameter"),
      "ring_inactive_diameter":
        utils.select(1.0, args, "ring_inactive_diameter"),
      "hit_color": utils.select(colors.kWhite, args, "hit_color"),
      "no_hit_color": utils.select(colors.kWhite, args, "no_hit_color"),
      "no_hit_distance": utils.select(2.0, args, "no_hit_distance"),
      "inner_hole": utils.select(0.0, args, "inner_hole"),
      "inner_ring_end": utils.select(0.177, args, "inner_ring_end"),
      "inner_ring_thickness": utils.select(0.14, args, "inner_ring_thickness"),
      "mid_ring_end": utils.select(0.177, args, "mid_ring_end"),
      "mid_ring_opacity": utils.select(0.22, args, "mid_ring_opacity"),
      "device": utils.select("Controller", args, "device")
    }
    }] + cursor_trail_def
}
