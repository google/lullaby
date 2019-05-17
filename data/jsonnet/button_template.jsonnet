local colors = import "colors.jsonnet";
local utils = import "lullaby/data/jsonnet/utils.jsonnet";
local button_response = import "button_response.jsonnet";
local responses = import "lullaby/data/jsonnet/responses.jsonnet";

{
  button_root(position, rotation, scale)::
  [{
    "def_type": "TransformDef",
    "def": {
      "position": {
        "x": utils.select(0.0, position, "x"),
        "y": utils.select(0.0, position, "y"),
        "z": utils.select(0.0, position, "z")
      },
      "rotation": {
        "x": utils.select(0.0, rotation, "x"),
        "y": utils.select(0.0, rotation, "y"),
        "z": utils.select(0.0, rotation, "z")
      },
      "scale": {
        "x": utils.select(1.0, scale, "x"),
        "y": utils.select(1.0, scale, "y"),
        "z": utils.select(1.0, scale, "z")
      }
    }
  }],

  button_shadow(shadow, quad)::
  [{
   "def_type": "TransformDef",
   "def": {
     "position": utils.select(utils.vec3(0.0, 0.0, 0.0), shadow, "position"),
     "enabled": utils.select(false, shadow, "enabled_on_init")
   }
  }, {
   "def_type": "RenderDef",
   "def": {
      local default_shader = if std.objectHas(shadow, "texture") then
        "shaders/texture.fplshader" else "shaders/color.fplshader",
      local margin = utils.select(utils.vec2(0.0, 0.0), shadow,
        "margin"),
      "quad": {
        "size_x": quad.size_x + margin.x,
        "size_y": quad.size_y + margin.y,
        "verts_x": utils.select(quad.verts_x, shadow, "verts_x"),
        "verts_y": utils.select(quad.verts_y, shadow, "verts_y"),
        "has_uv": utils.select(true, shadow, "has_uv"),
        "corner_radius": utils.select(quad.corner_radius, shadow,
          "corner_radius"),
        "corner_verts": utils.select(quad.corner_verts, shadow,
          "corner_verts")
      },
      "shader": utils.select(default_shader, shadow, "shader"),
      "texture": utils.select("", shadow, "texture"),
      "color": utils.select(colors.kWhite, shadow, "color"),
      "color_hex": utils.select(colors.kWhiteHex, shadow, "color_hex"),
      "sort_order_offset": -1,
   }
  }, {
    "def_type": "NameDef",
    "def": {
        "name": "shadow"
    }
  }] + if std.objectHas(shadow, "extra_defs") then shadow.extra_defs else [],

  button_bg(bg)::
  [{
    "def_type": "TransformDef",
    "def": {}
  }, {
    "def_type": "RenderDef",
    "def": {
      local default_shader = if std.objectHas(bg, "texture") then
        "shaders/texture.fplshader" else "shaders/color.fplshader",
      "pass": utils.select("Main", bg, "pass"),
      "quad": {
        "size_x": utils.select(0.0, bg, "width"),
        "size_y": utils.select(0.0, bg, "height"),
        "verts_x": utils.select(2, bg, "verts_x"),
        "verts_y": utils.select(2, bg, "verts_y"),
        "has_uv": utils.select(false, bg, "has_uv"),
        "corner_radius": utils.select(0.0, bg, "corner_radius"),
        "corner_verts": utils.select(0.0, bg, "corner_verts")
      },
      "shader": utils.select(default_shader, bg, "shader"),
      "texture": utils.select("", bg, "texture"),
      "color": utils.select(colors.kWhite, bg, "color"),
      "color_hex": utils.select(colors.kWhiteHex, bg, "color_hex")
    }
  }, {
    "def_type": "CollisionDef",
    "def": {
      "interactive": utils.select(true, bg, "interactive")
    }
  }, {
    "def_type": "NameDef",
    "def": {
        "name": "background"
    }
  }] + if std.objectHas(bg, "extra_defs") then bg.extra_defs else [],

  button_fg()::
  [{
    "def_type": "TransformDef",
    "def": {}
  }, {
    "def_type": "NameDef",
    "def": {
        "name": "foreground"
    }
  }],

  button_text(text)::
  [{
    "def_type": "TransformDef",
    "def": {}
  }, {
    "def_type": "RenderDef",
    "def": {
      "shader": utils.select("shaders/sdf_glyph_aa.fplshader", text, "shader"),
      "color": utils.select(colors.kWhite, text, "color"),
      "color_hex": utils.select(colors.kWhiteHex, text, "color_hex")
    }
  }, {
    "def_type": "TextDef",
    "def": {
      "text": utils.select("", text, "string"),
      "fonts": text.fonts,
      "font_size": text.font_size,
      "line_height_scale": utils.select(1.2, text, "line_height_scale"),
      "vertical_alignment":
        utils.select("Baseline", text, "vertical_alignment"),
      "horizontal_alignment":
        utils.select("Center", text, "horizontal_alignment"),
      "direction": utils.select("UseSystemSetting", text, "direction"),
      "bounds": utils.select({"x": 0.0, "y": 0.0}, text, "bounds"),
      "wrap_mode": utils.select("None", text, "wrap_mode"),
      "ellipsis": utils.select("", text, "ellipsis"),
      "edge_softness": utils.select(0.3, text, "edge_softness"),
      "kerning_scale": utils.select(1.0, text, "kerning_scale")
    }
  }, {
    "def_type": "NameDef",
    "def": {
        "name": "text"
    }
  }] + if std.objectHas(text, "extra_defs") then text.extra_defs else [],

  // A template to use for simplification of layered button
  // (shadow <-ROOT-> background -> foreground -> text)
  button(args)::
  {
    local optionals =
    {
      button_shadow: if std.objectHas(args, "shadow") then [{
        components: $.button_shadow(args.shadow,
          button_bg.components[1].def.quad) +
          button_response.get_response(args.shadow)
      }] else [],
      button_text: if std.objectHas(args, "text") then [{
        components: $.button_text(args.text) +
        button_response.get_response(args.text)
      }] else []
    },
    local button_fg =
    {
      components: $.button_fg(),
      children: [] + optionals.button_text
    },
    local button_bg =
    {
      components: $.button_bg(args.background) +
        button_response.get_mapped_response(args.background) +
        responses.make_script_response(args),
      children: [button_fg]
    },

    components: $.button_root(args.position, utils.select({}, args, "rotation"),
        utils.select({}, args, "scale")),
    children: [button_bg,] + optionals.button_shadow
  }
}
