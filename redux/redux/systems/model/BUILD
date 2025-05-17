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
        "model_system.cc",
    ],
    hdrs = [
        "model_system.h",
    ],
    deps = [
        ":model_asset",
        ":model_def",
        "@absl//absl/container:flat_hash_map",
        "@absl//absl/log",
        "@absl//absl/log:check",
        "@absl//absl/types:span",
        "//redux/engines/physics",
        "//redux/engines/render",
        "//redux/modules/base:asset_loader",
        "//redux/modules/base:data_container",
        "//redux/modules/base:hash",
        "//redux/modules/base:registry",
        "//redux/modules/base:resource_manager",
        "//redux/modules/base:typeid",
        "//redux/modules/ecs",
        "//redux/modules/ecs:entity",
        "//redux/modules/graphics:image_data",
        "//redux/modules/graphics:material_data",
        "//redux/modules/graphics:mesh_data",
        "//redux/modules/math:bounds",
        "//redux/modules/math:matrix",
        "//redux/modules/math:vector",
        "//redux/systems/physics",
        "//redux/systems/render",
        "//redux/systems/rig",
        "//redux/systems/transform",
    ],
)

cc_library(
    name = "model_asset",
    srcs = [
        "model_asset.cc",
        "model_asset_factory.cc",
    ],
    hdrs = [
        "model_asset.h",
        "model_asset_factory.h",
    ],
    deps = [
        "@absl//absl/base:core_headers",
        "@absl//absl/container:flat_hash_map",
        "@absl//absl/types:span",
        "//redux/engines/physics",
        "//redux/engines/render",
        "//redux/modules/base:data_container",
        "//redux/modules/base:filepath",
        "//redux/modules/base:hash",
        "//redux/modules/base:registry",
        "//redux/modules/graphics:image_data",
        "//redux/modules/graphics:material_data",
        "//redux/modules/graphics:mesh_data",
        "//redux/modules/graphics:vertex_format",
        "//redux/modules/math:matrix",
    ],
)

cc_library(
    name = "gltf",
    srcs = ["gltf_asset.cc"],
    hdrs = ["gltf_asset.h"],
    deps = [
        ":model_asset",
        "@absl//absl/container:flat_hash_map",
        "@absl//absl/log",
        "@absl//absl/log:check",
        "@absl//absl/strings",
        "@absl//absl/types:span",
        "//draco/attributes:geometry_attribute",
        "//draco/attributes:geometry_indices",
        "//draco/attributes:point_attribute",
        "//draco/compression:decode",
        "//draco/core:decoder_buffer",
        "//draco/core:draco_types",
        "//draco/mesh",
        "//redux/modules/base:asset_loader",
        "//redux/modules/base:data_builder",
        "//redux/modules/base:data_container",
        "//redux/modules/base:hash",
        "//redux/modules/base:registry",
        "//redux/modules/graphics:enums",
        "//redux/modules/graphics:graphics_enums_fbs",
        "//redux/modules/graphics:image_data",
        "//redux/modules/graphics:material_data",
        "//redux/modules/graphics:mesh_data",
        "//redux/modules/graphics:texture_usage",
        "//redux/modules/graphics:vertex_attribute",
        "//redux/modules/graphics:vertex_format",
        "//redux/modules/math:bounds",
        "//redux/modules/math:matrix",
        "//redux/modules/math:transform",
        "//redux/modules/math:vector",
        "//tinygltf",
    ],
    alwayslink = 1,
)

cc_library(
    name = "redux_model",
    srcs = ["redux_model_asset.cc"],
    hdrs = ["redux_model_asset.h"],
    deps = [
        ":model_asset",
        "@absl//absl/container:flat_hash_map",
        "@absl//absl/log",
        "@absl//absl/log:check",
        "@absl//absl/types:span",
        "//redux/data/asset_defs:model_asset_def_fbs",
        "//redux/modules/base:data_container",
        "//redux/modules/base:hash",
        "//redux/modules/base:registry",
        "//redux/modules/codecs:decode_image",
        "//redux/modules/flatbuffers:math",
        "//redux/modules/flatbuffers:var",
        "//redux/modules/graphics:graphics_enums_fbs",
        "//redux/modules/graphics:image_data",
        "//redux/modules/graphics:image_utils",
        "//redux/modules/graphics:material_data",
        "//redux/modules/graphics:mesh_data",
        "//redux/modules/graphics:texture_usage",
        "//redux/modules/graphics:vertex_format",
        "//redux/modules/math:bounds",
        "//redux/modules/math:matrix",
        "//redux/modules/math:vector",
    ],
    alwayslink = 1,
)

cc_library(
    name = "usd",
    srcs = ["usd_asset.cc"],
    hdrs = ["usd_asset.h"],
    deps = [
        ":model_asset",
        "@absl//absl/container:flat_hash_map",
        "@absl//absl/log",
        "@absl//absl/log:check",
        "//redux/modules/base:data_builder",
        "//redux/modules/base:hash",
        "//redux/modules/base:registry",
        "//redux/modules/graphics:graphics_enums_fbs",
        "//redux/modules/graphics:material_data",
        "//redux/modules/graphics:mesh_data",
        "//redux/modules/graphics:texture_usage",
        "//redux/modules/graphics:vertex",
        "//redux/modules/graphics:vertex_attribute",
        "//redux/modules/graphics:vertex_format",
        "//redux/modules/math:bounds",
        "//redux/modules/math:vector",
        "//redux/modules/usd:usd_asset_resolver",
        "@usd//:ar",
        "@usd//:gf",
        "@usd//:sdf",
        "@usd//:tf",
        "@usd//:usd",
        "@usd//:usdGeom",
        "@usd//:usdShade",
        "@usd//:vt",
    ],
    alwayslink = 1,
)

cc_library(
    name = "static",
    srcs = ["static_register.cc"],
    deps = [":model"],
    alwayslink = 1,
)
