#

load(
    "//redux/tools:def_cc_library.bzl",
    "def_cc_library",
)

licenses(["notice"])

package(
    default_visibility = ["//redux:visibility"],
)

def_cc_library(
    name = "model_def",
    src = "model_def.def",
)

cc_library(
    name = "model",
    srcs = [
        "model_asset.cc",
        "model_system.cc",
    ],
    hdrs = [
        "model_asset.h",
        "model_system.h",
    ],
    deps = [
        ":model_def",
        "@absl//absl/container:flat_hash_map",
        "//redux/data/asset_defs:model_asset_def_fbs",
        "//redux/engines/physics",
        "//redux/engines/render",
        "//redux/modules/base:asset_loader",
        "//redux/modules/base:data_builder",
        "//redux/modules/base:hash",
        "//redux/modules/base:resource_manager",
        "//redux/modules/base:typeid",
        "//redux/modules/codecs:decode_image",
        "//redux/modules/ecs",
        "//redux/modules/flatbuffers:math",
        "//redux/modules/flatbuffers:var",
        "//redux/modules/graphics:image_data",
        "//redux/modules/graphics:image_utils",
        "//redux/modules/graphics:material_data",
        "//redux/modules/graphics:mesh_data",
        "//redux/systems/physics",
        "//redux/systems/render",
        "//redux/systems/rig",
    ],
)

cc_library(
    name = "static",
    srcs = ["static_register.cc"],
    deps = [":model"],
    alwayslink = 1,
)
