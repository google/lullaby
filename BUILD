# Lullaby, a collection of C++ libraries for building XR applications.


load(
    "@//dev:generate_schema_cc_library.bzl",
    "generate_schema_cc_library",
)


package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])  # Apache 2.0

exports_files(["LICENSE"])

config_setting(
    name = "is_debug",
    values = {
        "compilation_mode": "dbg",
    },
    visibility = ["//visibility:public"],
)

config_setting(
    name = "is_fastbuild",
    values = {
        "compilation_mode": "fastbuild",
    },
    visibility = ["//visibility:public"],
)

config_setting(
    name = "target_os_android",
    constraint_values = [
        "@bazel_tools//platforms:android",
    ],
)

config_setting(
    name = "target_os_windows",
    constraint_values = [
        "@bazel_tools//platforms:windows",
    ],
)

config_setting(
    name = "target_os_ios",
    constraint_values = [
        "@bazel_tools//platforms:ios",
    ],
)

lullaby_schemas = [
    "schemas/lull/animation_def.fbs",
    "schemas/lull/animation_response_def.fbs",
    "schemas/lull/animation_stategraph.fbs",
    "schemas/lull/audio_environment_def.fbs",
    "schemas/lull/audio_listener_def.fbs",
    "schemas/lull/audio_playback_types.fbs",
    "schemas/lull/audio_response_def.fbs",
    "schemas/lull/audio_source_def.fbs",
    "schemas/lull/axis_system.fbs",
    "schemas/lull/backdrop_def.fbs",
    "schemas/lull/blend_shape_def.fbs",
    "schemas/lull/blueprint_def.fbs",
    "schemas/lull/clip_def.fbs",
    "schemas/lull/collision_def.fbs",
    "schemas/lull/common.fbs",
    "schemas/lull/config_def.fbs",
    "schemas/lull/controller_def.fbs",
    "schemas/lull/cursor_def.fbs",
    "schemas/lull/cursor_trail_def.fbs",
    "schemas/lull/datastore_def.fbs",
    "schemas/lull/deform_def.fbs",
    "schemas/lull/dispatcher_def.fbs",
    "schemas/lull/fade_in_def.fbs",
    "schemas/lull/face_point_mutator_def.fbs",
    "schemas/lull/fpl_mesh_def.fbs",
    "schemas/lull/gltf_asset_def.fbs",
    "schemas/lull/grab_def.fbs",
    "schemas/lull/input_behavior_def.fbs",
    "schemas/lull/layout_def.fbs",
    "schemas/lull/light_def.fbs",
    "schemas/lull/linear_grabbable_def.fbs",
    "schemas/lull/lull_common.fbs",
    "schemas/lull/map_events_def.fbs",
    "schemas/lull/material_def.fbs",
    "schemas/lull/model_asset_def.fbs",
    "schemas/lull/model_def.fbs",
    "schemas/lull/model_pipeline_def.fbs",
    "schemas/lull/mutator_def.fbs",
    "schemas/lull/name_def.fbs",
    "schemas/lull/nav_button_def.fbs",
    "schemas/lull/nine_patch_def.fbs",
    "schemas/lull/pano_def.fbs",
    "schemas/lull/physics_shape_def.fbs",
    "schemas/lull/physics_shapes.fbs",
    "schemas/lull/planar_grab_input_def.fbs",
    "schemas/lull/planar_grabbable_def.fbs",
    "schemas/lull/render_def.fbs",
    "schemas/lull/render_pass_def.fbs",
    "schemas/lull/render_state_def.fbs",
    "schemas/lull/render_target_def.fbs",
    "schemas/lull/reticle_behaviour_def.fbs",
    "schemas/lull/reticle_boundary_def.fbs",
    "schemas/lull/reticle_def.fbs",
    "schemas/lull/reticle_trail_def.fbs",
    "schemas/lull/rigid_body_def.fbs",
    "schemas/lull/script_def.fbs",
    "schemas/lull/scroll_def.fbs",
    "schemas/lull/shader_clip_def.fbs",
    "schemas/lull/shader_def.fbs",
    "schemas/lull/shape_def.fbs",
    "schemas/lull/skeleton_def.fbs",
    "schemas/lull/slider_def.fbs",
    "schemas/lull/slideshow_def.fbs",
    "schemas/lull/snap_def.fbs",
    "schemas/lull/sort_mode.fbs",
    "schemas/lull/spatial_grab_input_def.fbs",
    "schemas/lull/spherical_grab_input_def.fbs",
    "schemas/lull/stategraph_def.fbs",
    "schemas/lull/stay_in_box_mutator_def.fbs",
    "schemas/lull/tab_header_layout_def.fbs",
    "schemas/lull/text_def.fbs",
    "schemas/lull/text_input_def.fbs",
    "schemas/lull/texture_atlas_def.fbs",
    "schemas/lull/texture_def.fbs",
    "schemas/lull/track_hmd_def.fbs",
    "schemas/lull/transform_def.fbs",
    "schemas/lull/variant_def.fbs",
    "schemas/lull/vertex_attribute_def.fbs",
    "schemas/lull/visibility_def.fbs",
    "schemas/lull/word_art_def.fbs",
]


generate_schema_cc_library(
    name = "fbs",
    srcs = lullaby_schemas,
    visibility = ["//visibility:public"],
    out_prefix = "lullaby/generated/"
)

