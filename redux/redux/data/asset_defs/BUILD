# Flatbuffer schemas for redux asset formats (eg. models, animations, etc.)

load(
    "//redux/tools:flatbuffer_cc_library.bzl",
    "flatbuffer_cc_library",
)

licenses(["notice"])

package(
    default_visibility = ["//redux:visibility"],
)

flatbuffer_cc_library(
    name = "anim_asset_def_fbs",
    srcs = ["anim_asset_def.fbs"],
    deps = [
        "//redux/modules/flatbuffers:common_fbs",
        "//redux/modules/flatbuffers:math_fbs",
        "//redux/modules/flatbuffers:var_fbs",
    ],
)

flatbuffer_cc_library(
    name = "model_asset_def_fbs",
    srcs = ["model_asset_def.fbs"],
    deps = [
        ":texture_asset_def_fbs",
        "//redux/modules/flatbuffers:common_fbs",
        "//redux/modules/flatbuffers:math_fbs",
        "//redux/modules/flatbuffers:var_fbs",
        "//redux/modules/graphics:graphics_enums_fbs",
    ],
)

flatbuffer_cc_library(
    name = "shader_asset_def_fbs",
    srcs = ["shader_asset_def.fbs"],
    deps = [
        "//redux/modules/flatbuffers:common_fbs",
        "//redux/modules/graphics:graphics_enums_fbs",
    ],
)

flatbuffer_cc_library(
    name = "texture_asset_def_fbs",
    srcs = ["texture_asset_def.fbs"],
    deps = [
        "//redux/modules/flatbuffers:common_fbs",
        "//redux/modules/flatbuffers:math_fbs",
        "//redux/modules/graphics:graphics_enums_fbs",
    ],
)
