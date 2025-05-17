# Tool for building redux shader files.

licenses(["notice"])

package(
    
    default_visibility = ["//redux:visibility"],
)

cc_library(
    name = "shader_pipeline_lib",
    srcs = ["shader_pipeline.cc"],
    hdrs = ["shader_pipeline.h"],
    deps = [
        "@absl//absl/container:flat_hash_map",
        "@absl//absl/container:flat_hash_set",
        "@absl//absl/log",
        "@absl//absl/log:check",
        "@absl//absl/types:span",
        "@filament//:filamat",
        "@filament//:utils_hdrs",
        "//redux/data/asset_defs:shader_asset_def_fbs",
        "//redux/modules/base:data_builder",
        "//redux/modules/base:data_container",
        "//redux/modules/base:hash",
        "//redux/modules/flatbuffers:common",
        "//redux/modules/graphics:enums",
        "//redux/modules/graphics:texture_usage",
        "//redux/tools/common:file_utils",
        "//redux/tools/common:flatbuffer_utils",
    ],
)

cc_binary(
    name = "shader_pipeline",
    srcs = ["main.cc"],
    deps = [
        ":shader_pipeline_lib",
        "@absl//absl/flags:flag",
        "@absl//absl/flags:parse",
        "@absl//absl/log:check",
        "//redux/modules/base:filepath",
        "//redux/modules/base:logging",
        "//redux/tools/common:file_utils",
        "//redux/tools/common:json_utils",
        "//redux/tools/common:jsonnet_utils",
    ],
)
