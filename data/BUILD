load(
    "//third_party/fplbase:build_defs.bzl",
    "fpl_png_assets",
    "fpl_mesh_assets",
)
load(
    "//tools/build_defs/jsonnet:jsonnet.bzl",
    "jsonnet_library",
)
load(
    "@//dev:build_shader.bzl",
    "build_shader",
)
load(
    "@//dev:build_shader.bzl",
    "build_lullaby_shader",
)

licenses(["notice"])  # Apache 2.0

package(
    default_visibility = ["//visibility:public"],
)

exports_files(
    glob([
        "blueprints/*.json",
        "blueprints/*.jsonnet",
    ]),
)

exports_files(
    glob([
        "shaders/*",
        "jsonnet/*",
        "textures/*",
    ]),
)

Fileset(
    name = "shader_srcs",
    out = "shader_sources",
    entries = [
        FilesetEntry(
            files =
                glob([
                    "shaders/**/*.glsl*",
                    "lullshaders/**/*.jsonnet*",
                    "lullshaders/**/*.glsl*",
                ]),
        ),
    ],
)

Fileset(
    name = "jsonnet_srcs",
    out = "jsonnet_sources",
    entries = [
        FilesetEntry(
            files = glob(["jsonnet/*"]),
        ),
    ],
)


fpl_png_assets(
    name = "debug_font_texture",
    srcs = ["textures/debug_font.png"],
    strip_prefix = "",
)

filegroup(
    name = "shader_includes",
    srcs = glob([
        "shaders/*.glslh",
        "lullshaders/include/*.glslh",
    ]),
)

filegroup(
    name = "shader_pbr_includes",
    srcs = glob([
        "shaders/pbr/*.glslh",
        "shaders/pbr/*.h",
    ]),
)

filegroup(
    name = "depth_fragment",
    srcs = ["shaders/depth.glslf"],
)

filegroup(
    name = "light_shadow_texture_fragment",
    srcs = ["shaders/light_shadow_texture.glslf"],
)

filegroup(
    name = "pos_uv_vertex",
    srcs = ["shaders/pos_uv.glslv"],
)

filegroup(
    name = "sdf_glyph_aa_vertex",
    srcs = ["shaders/sdf_glyph_aa.glslv"],
)

filegroup(
    name = "sdf_glyph_aa_fragment",
    srcs = ["shaders/sdf_glyph_aa.glslf"],
)

filegroup(
    name = "uber_vertex",
    srcs = ["shaders/uber.glslv"],
)

filegroup(
    name = "vertex_color_texture",
    srcs = ["shaders/vertex_color_texture.glslv"],
)

filegroup(
    name = "texture_vertex",
    srcs = ["shaders/texture.glslv"],
)

build_shader(
    name = "color_shader",
    srcs = {
        "vertex": "shaders/color.glslv",
        "fragment": "shaders/color.glslf",
    },
    out = "shaders/color.fplshader",
    build_multiview = True,
    include_deps = [":shader_includes"],
)

build_shader(
    name = "batched_color_shader",
    srcs = {
        "vertex": "shaders/uber.glslv",
        "fragment": "shaders/color.glslf",
    },
    out = "shaders/batched_color.fplshader",
    build_multiview = True,
    defines = [
        "BATCHED_INDICES",
    ],
    include_deps = [":shader_includes"],
)

# Use for quads or nine patches without rounded corners
build_shader(
    name = "aa_color_shader",
    srcs = {
        "vertex": "shaders/uber.glslv",
        "fragment": "shaders/color.glslf",
    },
    out = "shaders/aa_color.fplshader",
    build_multiview = True,
    defines = [
        "TEX_COORD",
        "SDF_DIST_AA",
    ],
    include_deps = [":shader_includes"],
)

build_shader(
    name = "clip_aa_color_shader",
    srcs = {
        "vertex": "shaders/uber.glslv",
        "fragment": "shaders/color.glslf",
    },
    out = "shaders/clip_aa_color.fplshader",
    build_multiview = True,
    defines = [
        "TEX_COORD",
        "CLIP_REGION",
        "SDF_DIST_AA",
    ],
    include_deps = [":shader_includes"],
)

build_shader(
    name = "batched_clip_aa_color_shader",
    srcs = {
        "vertex": "shaders/uber.glslv",
        "fragment": "shaders/color.glslf",
    },
    out = "shaders/batched_clip_aa_color.fplshader",
    defines = [
        "BATCHED_INDICES",
        "TEX_COORD",
        "UV_BOUNDS",
        "CLAMP",
        "CLIP_REGION",
        "SDF_DIST_AA",
    ],
    include_deps = [":shader_includes"],
)

build_shader(
    name = "nine_patch_aa_color_shader",
    srcs = {
        "vertex": "shaders/uber.glslv",
        "fragment": "shaders/color.glslf",
    },
    out = "shaders/nine_patch_aa_color.fplshader",
    build_multiview = True,
    defines = [
        "NINE_PATCH_TEX_COORD_ALT",
        "SDF_DIST_AA",
    ],
    include_deps = [":shader_includes"],
)

build_shader(
    name = "nine_patch_aa_color_border_shader",
    srcs = {
        "vertex": "shaders/uber.glslv",
        "fragment": "shaders/color_border.glslf",
    },
    out = "shaders/nine_patch_aa_color_border.fplshader",
    build_multiview = True,
    defines = [
        "NINE_PATCH_TEX_COORD_ALT",
        "SDF_DIST_AA",
    ],
    include_deps = [":shader_includes"],
)

build_shader(
    name = "clip_nine_patch_aa_color_shader",
    srcs = {
        "vertex": "shaders/uber.glslv",
        "fragment": "shaders/color.glslf",
    },
    out = "shaders/clip_nine_patch_aa_color.fplshader",
    build_multiview = True,
    defines = [
        "NINE_PATCH_TEX_COORD_ALT",
        "CLIP_REGION",
        "SDF_DIST_AA",
    ],
    include_deps = [":shader_includes"],
)

build_shader(
    name = "clip_y_nine_patch_aa_color_shader",
    srcs = {
        "vertex": "shaders/uber.glslv",
        "fragment": "shaders/color.glslf",
    },
    out = "shaders/clip_y_nine_patch_aa_color.fplshader",
    build_multiview = True,
    defines = [
        "NINE_PATCH_TEX_COORD_ALT",
        "CLIP_REGION",
        "CLIP_Y_ONLY",
        "SDF_DIST_AA",
    ],
    include_deps = [":shader_includes"],
)

build_shader(
    name = "batched_clip_nine_patch_aa_color_shader",
    srcs = {
        "vertex": "shaders/uber.glslv",
        "fragment": "shaders/color.glslf",
    },
    out = "shaders/batched_clip_nine_patch_aa_color.fplshader",
    defines = [
        "BATCHED_INDICES",
        "NINE_PATCH_TEX_COORD_ALT",
        "UV_BOUNDS",
        "CLAMP",
        "CLIP_REGION",
        "SDF_DIST_AA",
    ],
    include_deps = [":shader_includes"],
)

build_shader(
    name = "composite_shadow_shader",
    srcs = {
        "vertex": "shaders/texture.glslv",
        "fragment": "shaders/composite_shadow.glslf",
    },
    out = "shaders/composite_shadow.fplshader",
    include_deps = [":shader_includes"],
)

build_shader(
    name = "depth_shader",
    srcs = {
        "vertex": "shaders/depth.glslv",
        "fragment": "shaders/depth.glslf",
    },
    out = "shaders/depth.fplshader",
    include_deps = [":shader_includes"],
)

build_shader(
    name = "light_shader",
    srcs = {
        "vertex": "shaders/light.glslv",
        "fragment": "shaders/light.glslf",
    },
    out = "shaders/light.fplshader",
    include_deps = [":shader_includes"],
)

build_shader(
    name = "light_texture_shader",
    srcs = {
        "vertex": "shaders/light_texture.glslv",
        "fragment": "shaders/light_texture.glslf",
    },
    out = "shaders/light_texture.fplshader",
    include_deps = [":shader_includes"],
)

build_shader(
    name = "light_shadow_shader",
    srcs = {
        "vertex": "shaders/light_shadow.glslv",
        "fragment": "shaders/light_shadow.glslf",
    },
    out = "shaders/light_shadow.fplshader",
    include_deps = [":shader_includes"],
)

build_shader(
    name = "light_shadow_texture_shader",
    srcs = {
        "vertex": "shaders/light_shadow_texture.glslv",
        "fragment": "shaders/light_shadow_texture.glslf",
    },
    out = "shaders/light_shadow_texture.fplshader",
    include_deps = [":shader_includes"],
)

build_shader(
    name = "progress_bar_shader",
    srcs = {
        "vertex": "shaders/progress_bar.glslv",
        "fragment": "shaders/progress_bar.glslf",
    },
    out = "shaders/progress_bar.fplshader",
    build_multiview = True,
    include_deps = [":shader_includes"],
)

build_shader(
    name = "reticle_shader",
    srcs = {
        "vertex": "shaders/reticle.glslv",
        "fragment": "shaders/reticle.glslf",
    },
    out = "shaders/reticle.fplshader",
    build_multiview = True,
    include_deps = [":shader_includes"],
)

build_shader(
    name = "vertex_reticle_shader",
    srcs = {
        "vertex": "shaders/vertex_color_texture.glslv",
        "fragment": "shaders/vertex_reticle.glslf",
    },
    out = "shaders/vertex_reticle.fplshader",
    build_multiview = True,
    include_deps = [":shader_includes"],
)

build_shader(
    name = "screen_space_texture_shader",
    srcs = {
        "vertex": "shaders/screen_space_texture.glslv",
        "fragment": "shaders/screen_space_texture.glslf",
    },
    out = "shaders/screen_space_texture.fplshader",
    include_deps = [":shader_includes"],
)

build_shader(
    name = "multiview_screen_space_texture_shader",
    srcs = {
        "vertex": "shaders/screen_space_texture.glslv",
        "fragment": "shaders/screen_space_texture_essl3.glslf",
    },
    out = "multiview/shaders/screen_space_texture.fplshader",
    defines = ["MULTIVIEW"],
    include_deps = [":shader_includes"],
    version = "300 es",
)

build_shader(
    name = "text_shader",
    srcs = {
        "vertex": "shaders/pos_uv.glslv",
        "fragment": "shaders/text.glslf",
    },
    out = "shaders/text.fplshader",
    build_multiview = True,
    defines = ["TEX_COORD"],
    include_deps = [":shader_includes"],
    version = "300 es",
)

build_shader(
    name = "sdf_glyph_aa_shader",
    srcs = {
        "vertex": "shaders/sdf_glyph_aa.glslv",
        "fragment": "shaders/sdf_glyph_aa.glslf",
    },
    out = "shaders/sdf_glyph_aa.fplshader",
    build_multiview = True,
    defines = ["TEX_COORD"],
    include_deps = [":shader_includes"],
)

build_shader(
    name = "sdf_glyph_aa_outline_shader",
    srcs = {
        "vertex": "shaders/sdf_glyph_aa.glslv",
        "fragment": "shaders/sdf_glyph_aa.glslf",
    },
    out = "shaders/sdf_glyph_aa_outline.fplshader",
    build_multiview = True,
    defines = [
        "TEX_COORD",
        "OUTLINE",
    ],
    include_deps = [":shader_includes"],
)

build_shader(
    name = "batched_sdf_glyph_aa_shader",
    srcs = {
        "vertex": "shaders/sdf_glyph_aa.glslv",
        "fragment": "shaders/sdf_glyph_aa.glslf",
    },
    out = "shaders/batched_sdf_glyph_aa.fplshader",
    build_multiview = True,
    defines = [
        "BATCHED_INDICES",
        "TEX_COORD",
        "SDF_DIST_AA",
    ],
    include_deps = [":shader_includes"],
)

build_shader(
    name = "sdf_glyph_aa_padding_shader",
    srcs = {
        "vertex": "shaders/sdf_glyph_aa.glslv",
        "fragment": "shaders/sdf_glyph_aa.glslf",
    },
    out = "shaders/sdf_glyph_aa_padding.fplshader",
    defines = [
        "TEX_COORD",
        "TEX_COORD_AA_PADDING",
        "SDF_DIST_AA",
    ],
    include_deps = [":shader_includes"],
)

build_shader(
    name = "clip_aa_sdf_glyph_aa_shader",
    srcs = {
        "vertex": "shaders/sdf_glyph_aa.glslv",
        "fragment": "shaders/sdf_glyph_aa.glslf",
    },
    out = "shaders/clip_aa_sdf_glyph_aa.fplshader",
    build_multiview = True,
    defines = [
        "TEX_COORD",
        "CLIP_REGION",
        "SDF_DIST_AA",
    ],
    include_deps = [":shader_includes"],
)

build_shader(
    name = "clip_y_aa_sdf_glyph_aa_shader",
    srcs = {
        "vertex": "shaders/sdf_glyph_aa.glslv",
        "fragment": "shaders/sdf_glyph_aa.glslf",
    },
    out = "shaders/clip_y_aa_sdf_glyph_aa.fplshader",
    build_multiview = True,
    defines = [
        "TEX_COORD",
        "CLIP_REGION",
        "CLIP_Y_ONLY",
        "SDF_DIST_AA",
    ],
    include_deps = [":shader_includes"],
)

build_shader(
    name = "batched_clip_y_aa_sdf_glyph_aa_shader",
    srcs = {
        "vertex": "shaders/sdf_glyph_aa.glslv",
        "fragment": "shaders/sdf_glyph_aa.glslf",
    },
    out = "shaders/batched_clip_y_aa_sdf_glyph_aa.fplshader",
    build_multiview = True,
    defines = [
        "BATCHED_INDICES",
        "TEX_COORD",
        "CLIP_REGION",
        "CLIP_Y_ONLY",
        "SDF_DIST_AA",
    ],
    include_deps = [":shader_includes"],
)

build_shader(
    name = "clip_aa_sdf_glyph_aa_padding_shader",
    srcs = {
        "vertex": "shaders/sdf_glyph_aa.glslv",
        "fragment": "shaders/sdf_glyph_aa.glslf",
    },
    out = "shaders/clip_aa_sdf_glyph_aa_padding.fplshader",
    build_multiview = True,
    defines = [
        "TEX_COORD",
        "TEX_COORD_AA_PADDING",
        "CLIP_REGION",
        "SDF_DIST_AA",
    ],
    include_deps = [":shader_includes"],
)

build_shader(
    name = "skinned_texture_shader",
    srcs = {
        "vertex": "shaders/skinned_texture.glslv",
        "fragment": "shaders/texture.glslf",
    },
    out = "shaders/skinned_texture.fplshader",
    defines = [
        "TEX_COORD",
        "UV_BOUNDS",
    ],
    include_deps = [":shader_includes"],
)

build_shader(
    name = "texture_shader",
    srcs = {
        "vertex": "shaders/texture.glslv",
        "fragment": "shaders/texture.glslf",
    },
    out = "shaders/texture.fplshader",
    build_multiview = True,
    defines = [
        "TEX_COORD",
        "UV_BOUNDS",
    ],
    include_deps = [":shader_includes"],
)

build_shader(
    name = "texture_2d_shader",
    srcs = {
        "vertex": "shaders/texture_2d.glslv",
        "fragment": "shaders/texture.glslf",
    },
    out = "shaders/texture_2d.fplshader",
    defines = [
        "TEX_COORD",
        "UV_BOUNDS",
    ],
    include_deps = [":shader_includes"],
)

build_shader(
    name = "texture_2d_external_oes_shader",
    srcs = {
        "vertex": "shaders/texture_2d.glslv",
        "fragment": "shaders/texture_external_oes.glslf",
    },
    out = "shaders/texture_2d_external_oes.fplshader",
    defines = [
        "TEX_COORD",
        "UV_BOUNDS",
    ],
    include_deps = [":shader_includes"],
)

build_shader(
    name = "pbr_shader",
    srcs = {
        "vertex": "shaders/pbr.glslv",
        "fragment": "shaders/pbr.glslf",
    },
    out = "shaders/pbr.fplshader",
    build_multiview = True,
    defines = [
        "TEX_COORD",
    ],
    include_deps = [
        ":shader_includes",
        ":shader_pbr_includes",
    ],
    version = "300 es",
)

build_shader(
    name = "clamped_texture_shader",
    srcs = {
        "vertex": "shaders/texture.glslv",
        "fragment": "shaders/texture.glslf",
    },
    out = "shaders/texture_clamped.fplshader",
    build_multiview = True,
    defines = [
        "TEX_COORD",
        "UV_BOUNDS",
        "CLAMP",
    ],
    include_deps = [":shader_includes"],
)

build_shader(
    name = "aa_texture_shader",
    srcs = {
        "vertex": "shaders/uber.glslv",
        "fragment": "shaders/texture.glslf",
    },
    out = "shaders/aa_texture.fplshader",
    build_multiview = True,
    defines = [
        "TEX_COORD",
        "UV_BOUNDS",
        "SDF_DIST_AA",
    ],
    include_deps = [":shader_includes"],
)

build_shader(
    name = "aa_texture_with_alt_shader",
    srcs = {
        "vertex": "shaders/uber.glslv",
        "fragment": "shaders/texture.glslf",
    },
    out = "shaders/aa_texture_with_alt.fplshader",
    build_multiview = True,
    defines = [
        "TEX_COORD_ALT",
        "UV_BOUNDS",
        "SDF_DIST_AA",
    ],
    include_deps = [":shader_includes"],
)

build_shader(
    name = "aa_texture_clamped_shader",
    srcs = {
        "vertex": "shaders/uber.glslv",
        "fragment": "shaders/texture.glslf",
    },
    out = "shaders/aa_texture_clamped.fplshader",
    build_multiview = True,
    defines = [
        "TEX_COORD",
        "UV_BOUNDS",
        "SDF_DIST_AA",
        "CLAMP",
    ],
    include_deps = [":shader_includes"],
)

build_shader(
    name = "clip_aa_texture_clamped_shader",
    srcs = {
        "vertex": "shaders/uber.glslv",
        "fragment": "shaders/texture.glslf",
    },
    out = "shaders/clip_aa_texture_clamped.fplshader",
    defines = [
        "TEX_COORD",
        "UV_BOUNDS",
        "CLIP_REGION",
        "SDF_DIST_AA",
        "CLAMP",
    ],
    include_deps = [":shader_includes"],
)

build_shader(
    name = "nine_patch_aa_texture_shader",
    srcs = {
        "vertex": "shaders/uber.glslv",
        "fragment": "shaders/texture.glslf",
    },
    out = "shaders/nine_patch_aa_texture.fplshader",
    build_multiview = True,
    defines = [
        "TEX_COORD",
        "UV_BOUNDS",
        "NINE_PATCH_TEX_COORD_ALT",
        "SDF_DIST_AA",
    ],
    include_deps = [":shader_includes"],
)

build_shader(
    name = "nine_patch_aa_texture_multisample_shader",
    srcs = {
        "vertex": "shaders/uber.glslv",
        "fragment": "shaders/texture.glslf",
    },
    out = "shaders/nine_patch_aa_texture_multisample.fplshader",
    build_multiview = True,
    defines = [
        "TEX_COORD",
        "UV_BOUNDS",
        "NINE_PATCH_TEX_COORD_ALT",
        "SDF_DIST_AA",
        "MULTISAMPLE",
    ],
    include_deps = [":shader_includes"],
)

build_shader(
    name = "nine_patch_aa_texture_clamped_shader",
    srcs = {
        "vertex": "shaders/uber.glslv",
        "fragment": "shaders/texture.glslf",
    },
    out = "shaders/nine_patch_aa_texture_clamped.fplshader",
    build_multiview = True,
    defines = [
        "TEX_COORD",
        "UV_BOUNDS",
        "NINE_PATCH_TEX_COORD_ALT",
        "SDF_DIST_AA",
        "CLAMP",
    ],
    include_deps = [":shader_includes"],
)

# Always clips if not in a ShaderClip region due to an issue with AA
build_shader(
    name = "clip_nine_patch_aa_texture_shader",
    srcs = {
        "vertex": "shaders/uber.glslv",
        "fragment": "shaders/texture.glslf",
    },
    out = "shaders/clip_nine_patch_aa_texture.fplshader",
    build_multiview = True,
    defines = [
        "TEX_COORD",
        "UV_BOUNDS",
        "NINE_PATCH_TEX_COORD_ALT",
        "CLIP_REGION",
        "SDF_DIST_AA",
    ],
    include_deps = [":shader_includes"],
)

build_shader(
    name = "clip_y_nine_patch_aa_texture_shader",
    srcs = {
        "vertex": "shaders/uber.glslv",
        "fragment": "shaders/texture.glslf",
    },
    out = "shaders/clip_y_nine_patch_aa_texture.fplshader",
    build_multiview = True,
    defines = [
        "TEX_COORD",
        "UV_BOUNDS",
        "NINE_PATCH_TEX_COORD_ALT",
        "CLIP_REGION",
        "CLIP_Y_ONLY",
        "SDF_DIST_AA",
    ],
    include_deps = [":shader_includes"],
)

build_shader(
    name = "clip_y_aa_texture_shader",
    srcs = {
        "vertex": "shaders/uber.glslv",
        "fragment": "shaders/texture.glslf",
    },
    out = "shaders/clip_y_aa_texture.fplshader",
    build_multiview = True,
    defines = [
        "TEX_COORD",
        "UV_BOUNDS",
        "CLIP_REGION",
        "CLIP_Y_ONLY",
        "SDF_DIST_AA",
    ],
    include_deps = [":shader_includes"],
)

build_shader(
    name = "batched_nine_patch_aa_texture_shader",
    srcs = {
        "vertex": "shaders/uber.glslv",
        "fragment": "shaders/texture.glslf",
    },
    out = "shaders/batched_nine_patch_aa_texture.fplshader",
    build_multiview = True,
    defines = [
        "BATCHED_INDICES",
        "TEX_COORD",
        "UV_BOUNDS",
        "CLAMP",
        "NINE_PATCH_TEX_COORD_ALT",
        "SDF_DIST_AA",
    ],
    include_deps = [":shader_includes"],
)

build_shader(
    name = "batched_clip_nine_patch_aa_texture_shader",
    srcs = {
        "vertex": "shaders/uber.glslv",
        "fragment": "shaders/texture.glslf",
    },
    out = "shaders/batched_clip_nine_patch_aa_texture.fplshader",
    build_multiview = True,
    defines = [
        "BATCHED_INDICES",
        "TEX_COORD",
        "UV_BOUNDS",
        "CLAMP",
        "NINE_PATCH_TEX_COORD_ALT",
        "CLIP_REGION",
        "SDF_DIST_AA",
    ],
    include_deps = [":shader_includes"],
)

build_shader(
    name = "batched_clip_y_nine_patch_aa_texture_shader",
    srcs = {
        "vertex": "shaders/uber.glslv",
        "fragment": "shaders/texture.glslf",
    },
    out = "shaders/batched_clip_y_nine_patch_aa_texture.fplshader",
    build_multiview = True,
    defines = [
        "BATCHED_INDICES",
        "TEX_COORD",
        "UV_BOUNDS",
        "CLAMP",
        "NINE_PATCH_TEX_COORD_ALT",
        "CLIP_REGION",
        "CLIP_Y_ONLY",
        "SDF_DIST_AA",
    ],
    include_deps = [":shader_includes"],
)

build_shader(
    name = "vertex_color_shader",
    srcs = {
        "vertex": "shaders/vertex_color.glslv",
        "fragment": "shaders/vertex_color.glslf",
    },
    out = "shaders/vertex_color.fplshader",
    build_multiview = True,
    include_deps = [":shader_includes"],
)

build_shader(
    name = "nine_patch_aa_vertical_gradient_texture_shader",
    srcs = {
        "vertex": "shaders/uber.glslv",
        "fragment": "shaders/vertical_gradient_texture.glslf",
    },
    out = "shaders/nine_patch_aa_vertical_gradient_texture.fplshader",
    build_multiview = True,
    defines = [
        "TEX_COORD_ALT",
        "UV_BOUNDS",
        "SDF_DIST_AA",
    ],
    include_deps = [":shader_includes"],
)

build_lullaby_shader(
    name = "unlit_shader",
    out = "shaders/unlit.lullshader",
    include_deps = [
        ":shader_includes",
    ],
    srcs_fragment = [
        "lullshaders/fragment_white.jsonnet",
        "lullshaders/fragment_color.jsonnet",
        "lullshaders/fragment_texture.jsonnet",
        "lullshaders/fragment_uniform_color.jsonnet",
    ],
    srcs_vertex = [
        "lullshaders/vertex_skinning.jsonnet",
        "lullshaders/vertex_position_multiview.jsonnet",
        "lullshaders/vertex_position.jsonnet",
        "lullshaders/vertex_color.jsonnet",
        "lullshaders/vertex_texture.jsonnet",
    ],
)

build_lullaby_shader(
    name = "cubemap_shader",
    out = "shaders/cubemap.lullshader",
    include_deps = [
        ":shader_includes",
        ":shader_pbr_includes",
    ],
    srcs_fragment = [
        "lullshaders/fragment_white.jsonnet",
        "lullshaders/fragment_color.jsonnet",
        "lullshaders/fragment_cubemap.jsonnet",
        "lullshaders/fragment_uniform_color.jsonnet",
        "lullshaders/fragment_tonemap_reinhard.jsonnet",
        "lullshaders/apply_gamma_fragment.jsonnet",
    ],
    srcs_vertex = [
        "lullshaders/vertex_position_multiview.jsonnet",
        "lullshaders/vertex_position.jsonnet",
        "lullshaders/vertex_color.jsonnet",
        "lullshaders/vertex_cubemap.jsonnet",
    ],
)

build_lullaby_shader(
    name = "depth_lullshader",
    out = "shaders/depth.lullshader",
    include_deps = [
        ":shader_includes",
    ],
    srcs_fragment = [
        "lullshaders/fragment_depth.jsonnet",
    ],
    srcs_vertex = [
        "lullshaders/vertex_position_multiview.jsonnet",
        "lullshaders/vertex_position.jsonnet",
    ],
)

build_lullaby_shader(
    name = "phong_shader",
    out = "shaders/phong.lullshader",
    include_deps = [
        ":shader_includes",
    ],
    srcs_fragment = [
        "lullshaders/fragment_white.jsonnet",
        "lullshaders/fragment_color.jsonnet",
        "lullshaders/fragment_texture.jsonnet",
        "lullshaders/fragment_uniform_color.jsonnet",
        "lullshaders/fragment_phong.jsonnet",
        "lullshaders/fragment_tonemap_reinhard.jsonnet",
    ],
    srcs_vertex = [
        "lullshaders/vertex_skinning.jsonnet",
        "lullshaders/vertex_position_multiview.jsonnet",
        "lullshaders/vertex_position.jsonnet",
        "lullshaders/view_direction_vertex.jsonnet",
        "lullshaders/vertex_color.jsonnet",
        "lullshaders/vertex_texture.jsonnet",
        "lullshaders/normal_vertex.jsonnet",
    ],
)

build_lullaby_shader(
    name = "pbr_shading_model",
    out = "shaders/pbr.lullshader",
    include_deps = [
        ":shader_includes",
        ":shader_pbr_includes",
    ],
    srcs_fragment = [
        "lullshaders/base_color_fragment.jsonnet",
        "lullshaders/base_color_alpha_mask_fragment.jsonnet",
        "lullshaders/occlusion_roughness_metallic_fragment.jsonnet",
        "lullshaders/normal_fragment.jsonnet",
        "lullshaders/pbr_indirect_light_fragment.jsonnet",
        "lullshaders/pbr_light_setup_fragment.jsonnet",
        "lullshaders/pbr_fragment.jsonnet",
        "lullshaders/pbr_light_integration_fragment.jsonnet",
        "lullshaders/emissive_fragment.jsonnet",
        "lullshaders/fragment_tonemap_reinhard.jsonnet",
        "lullshaders/apply_gamma_fragment.jsonnet",
        "lullshaders/apply_color_correction.jsonnet",
        "lullshaders/fragment_dither.jsonnet",
    ],
    srcs_vertex = [
        "lullshaders/vertex_skinning.jsonnet",
        "lullshaders/vertex_position.jsonnet",
        "lullshaders/view_direction_vertex.jsonnet",
        "lullshaders/vertex_texture.jsonnet",
        "lullshaders/vertex_color.jsonnet",
        "lullshaders/normal_vertex.jsonnet",
        "lullshaders/tangent_bitangent_normal_vertex.jsonnet",
    ],
)

filegroup(
    name = "simple_font_all_assets",
    srcs = [
        ":debug_font_texture",
        ":texture_shader",
    ],
)

filegroup(
    name = "debug_renderer_all_assets",
    srcs = [
        ":simple_font_all_assets",
        ":texture_2d_external_oes_shader",
        ":texture_2d_shader",
        ":vertex_color_shader",
    ],
)

filegroup(
    name = "jsonnet_files",
    srcs = [
        "jsonnet/animations.jsonnet",
        "jsonnet/audio.jsonnet",
        "jsonnet/button_response.jsonnet",
        "jsonnet/button_template.jsonnet",
        "jsonnet/colors.jsonnet",
        "jsonnet/common_values.jsonnet",
        "jsonnet/cursor_template.jsonnet",
        "jsonnet/events.jsonnet",
        "jsonnet/fade.jsonnet",
        "jsonnet/material_colors.jsonnet",
        "jsonnet/responses.jsonnet",
        "jsonnet/reticle_template.jsonnet",
        "jsonnet/utils.jsonnet",
    ],
)

jsonnet_library(
    name = "jsonnet",
    srcs = [
        ":jsonnet_files",
    ],
)

cc_library(
    name = "shader_pbr_headers",
    hdrs = glob([
        "shaders/pbr/*.h",
    ]),
)
