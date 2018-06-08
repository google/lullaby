# Pipeline tool for converting 3D assets to Lullaby-compatible formats.

licenses(["notice"])  # Apache 2.0

cc_library(
    name = "animation_lib",
    srcs = [
        "animation.cc",
    ],
    hdrs = [
        "anim_bone.h",
        "animation.h",
        "tolerances.h",
    ],
    deps = [
        "@motive//:motive",
    ],
)

cc_library(
    name = "import_fbx",
    srcs = [
        "import_fbx.cc",
    ],
    deps = [
        ":animation_lib",
        "@libfbx//:libfbx",
        "//lullaby/util:logging",
        "//lullaby/tools/common:fbx_utils",
    ],
)

cc_library(
    name = "export",
    srcs = [
        "export.cc",
    ],
    hdrs = [
        "export.h",
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":animation_lib",
        "@flatbuffers//:flatbuffers",
        "//lullaby/util:common_types",
        "//third_party/motive:motive_fbs",
    ],
)

cc_library(
    name = "anim_pipeline_lib",
    srcs = [
        "anim_pipeline.cc",
    ],
    hdrs = [
        "anim_pipeline.h",
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":export",
        ":import_fbx",
        "//lullaby/util:common_types",
    ],
)

cc_binary(
    name = "anim_pipeline",
    srcs = [
        "main.cc",
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":anim_pipeline_lib",
        ":export",
        "//lullaby/util:arg_parser",
        "//lullaby/util:filename",
        "//lullaby/tools/common:file_utils",
    ],
)
