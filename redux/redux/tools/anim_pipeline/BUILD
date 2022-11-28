# Tool for building redux animation files.

licenses(["notice"])

package(
    default_visibility = ["//redux:visibility"],
)

cc_library(
    name = "animation",
    srcs = [
        "anim_curve.cc",
        "animation.cc",
    ],
    hdrs = [
        "anim_curve.h",
        "animation.h",
        "import_options.h",
        "tolerances.h",
    ],
    deps = [
        "@absl//absl/types:span",
        "//redux/engines/animation",
        "//redux/engines/animation/spline:compact_spline",
        "//redux/engines/animation/spline:cubic_curve",
        "//redux/modules/math:matrix",
        "//redux/modules/math:quaternion",
        "//redux/modules/math:vector",
        "//redux/tools/common:axis_system",
    ],
)

cc_library(
    name = "import_asset",
    srcs = ["import_asset.cc"],
    deps = [
        ":animation",
        "@absl//absl/container:flat_hash_map",
        "@assimp//:assimp",
        "//redux/modules/base:data_buffer",
        "//redux/modules/base:filepath",
        "//redux/tools/common:assimp_utils",
    ],
)

cc_library(
    name = "import_fbx",
    srcs = ["import_fbx.cc"],
    deps = [
        ":animation",
        "@absl//absl/container:flat_hash_map",
        "@absl//absl/types:span",
        "@libfbx//:libfbx",
        "//redux/modules/base:filepath",
        "//redux/modules/base:logging",
        "//redux/modules/math:transform",
        "//redux/tools/common:fbx_utils",
    ],
)

cc_library(
    name = "export",
    srcs = ["export.cc"],
    hdrs = ["export.h"],
    deps = [
        ":animation",
        "//redux/data/asset_defs:anim_asset_def_fbs",
        "//redux/engines/animation",
        "//redux/engines/animation/spline:compact_spline",
        "//redux/modules/base:data_container",
        "//redux/modules/flatbuffers:common",
        "//redux/modules/flatbuffers:math",
        "//redux/modules/flatbuffers:var",
        "//redux/tools/common:flatbuffer_utils",
        "//redux/tools/common:log_utils",
    ],
)

cc_library(
    name = "anim_pipeline_lib",
    srcs = ["anim_pipeline.cc"],
    hdrs = ["anim_pipeline.h"],
    deps = [
        ":animation",
        ":export",
        "@absl//absl/container:flat_hash_map",
        "//redux/modules/base:data_container",
        "//redux/modules/base:filepath",
        "//redux/tools/common:log_utils",
    ],
)

cc_binary(
    name = "anim_pipeline",
    srcs = ["main.cc"],
    deps = [
        ":anim_pipeline_lib",
        ":animation",
        ":import_asset",
        ":import_fbx",
        "@absl//absl/flags:flag",
        "@absl//absl/flags:parse",
        "//redux/modules/base:filepath",
        "//redux/modules/base:logging",
        "//redux/modules/math:constants",
        "//redux/tools/common:file_utils",
        "//redux/tools/common:flatbuffer_utils",
        "//redux/tools/common:log_utils",
    ],
)
