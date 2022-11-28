# Tool for building redux model files.

load(
    "//redux/tools:flatbuffer_cc_library.bzl",
    "flatbuffer_cc_library",
)

licenses(["notice"])

package(
    default_visibility = ["//redux:visibility"],
)

filegroup(
    name = "fbs",
    srcs = ["config.fbs"],
)

flatbuffer_cc_library(
    name = "config_fbs",
    srcs = ["config.fbs"],
    deps = [
        "//redux/modules/flatbuffers:common_fbs",
        "//redux/modules/flatbuffers:math_fbs",
        "//redux/modules/flatbuffers:var_fbs",
        "//redux/modules/graphics:graphics_enums_fbs",
        "//redux/tools/common:axis_system_fbs",
    ],
)

cc_library(
    name = "model",
    srcs = [
        "model.cc",
        "util.cc",
        "vertex.cc",
    ],
    hdrs = [
        "bone.h",
        "drawable.h",
        "material.h",
        "model.h",
        "texture_info.h",
        "util.h",
        "vertex.h",
    ],
    deps = [
        ":config_fbs",
        "@absl//absl/container:flat_hash_map",
        "@absl//absl/types:span",
        "//redux/modules/base:bits",
        "//redux/modules/base:data_container",
        "//redux/modules/base:filepath",
        "//redux/modules/base:typeid",
        "//redux/modules/flatbuffers:var",
        "//redux/modules/graphics:enums",
        "//redux/modules/graphics:image_utils",
        "//redux/modules/graphics:material_data",
        "//redux/modules/math:matrix",
        "//redux/modules/math:quaternion",
        "//redux/modules/math:vector",
        "//redux/modules/var",
        "//redux/tools/common:file_utils",
    ],
)

cc_library(
    name = "import_asset",
    srcs = ["import_asset.cc"],
    deps = [
        ":config_fbs",
        ":model",
        "@assimp//:assimp",
        "//redux/modules/base:data_buffer",
        "//redux/modules/base:filepath",
        "//redux/modules/graphics:image_utils",
        "//redux/tools/common:assimp_utils",
    ],
)

cc_library(
    name = "import_fbx",
    srcs = ["import_fbx.cc"],
    deps = [
        ":config_fbs",
        ":model",
        "@absl//absl/types:span",
        "@libfbx//:libfbx",
        "//redux/modules/base:logging",
        "//redux/tools/common:fbx_utils",
    ],
)

cc_library(
    name = "export",
    srcs = ["export.cc"],
    hdrs = ["export.h"],
    deps = [
        ":config_fbs",
        ":model",
        "@absl//absl/container:flat_hash_map",
        "@absl//absl/types:span",
        "//redux/data/asset_defs:model_asset_def_fbs",
        "//redux/data/asset_defs:texture_asset_def_fbs",
        "//redux/modules/base:data_container",
        "//redux/modules/flatbuffers:common",
        "//redux/modules/flatbuffers:math",
        "//redux/modules/flatbuffers:var",
        "//redux/modules/graphics:color",
        "//redux/tools/common:flatbuffer_utils",
        "//redux/tools/common:log_utils",
    ],
)

cc_library(
    name = "model_pipeline_lib",
    srcs = [
        "model_pipeline.cc",
        "texture_locator.cc",
    ],
    hdrs = [
        "model_pipeline.h",
        "texture_locator.h",
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":export",
        ":model",
        "@absl//absl/types:span",
        "@flatbuffers//:flatbuffers",
        "//redux/modules/base:filepath",
        "//redux/modules/base:logging",
        "//redux/tools/common:file_utils",
        "//redux/tools/common:log_utils",
    ],
)

cc_binary(
    name = "model_pipeline",
    srcs = ["main.cc"],
    deps = [
        ":import_asset",
        ":import_fbx",
        ":model",
        ":model_pipeline_lib",
        "@absl//absl/flags:flag",
        "@absl//absl/flags:parse",
        "//redux/modules/base:filepath",
        "//redux/modules/base:logging",
        "//redux/tools/common:axis_system",
        "//redux/tools/common:file_utils",
        "//redux/tools/common:flatbuffer_utils",
        "//redux/tools/common:log_utils",
    ],
)
