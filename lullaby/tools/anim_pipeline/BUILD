# Pipeline tool for converting 3D assets to Lullaby-compatible formats.

licenses(["notice"])  # Apache 2.0

cc_library(
    name = "animation_lib",
    srcs = [
        "anim_curve.cc",
        "animation.cc",
    ],
    hdrs = [
        "anim_bone.h",
        "anim_curve.h",
        "animation.h",
        "import_options.h",
        "tolerances.h",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//:fbs",
        "//lullaby/util:fixed_string",
        "//lullaby/util:logging",
        "//lullaby/util:math",
        "//lullaby/util:optional",
        "//lullaby/tools/common:file_utils",
        "@mathfu//:mathfu",
        "@motive//:motive",
    ],
)

cc_library(
    name = "import_asset",
    srcs = [
        "import_asset.cc",
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":animation_lib",
        "@assimp//:assimp",
        "//:fbs",
        "//lullaby/util:filename",
        "//lullaby/util:logging",
        "//lullaby/util:math",
        "//lullaby/tools/common:assimp_base_importer",
    ],
)

cc_library(
    name = "import_fbx",
    srcs = [
        "import_fbx.cc",
    ],
    copts = ["-Wno-null-dereference"],
    visibility = ["//visibility:public"],
    deps = [
        ":animation_lib",
        "@libfbx//:libfbx",
        "//lullaby/util:filename",
        "//lullaby/util:logging",
        "//lullaby/util:math",
        "//lullaby/util:optional",
        "//lullaby/tools/common:fbx_base_importer",
        "@motive//:motive",
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
        "@motive//:motive_fbs",
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
        ":animation_lib",
        ":export",
        "//lullaby/util:common_types",
        "//lullaby/util:filename",
        "//lullaby/util:make_unique",
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
        ":animation_lib",
        ":export",
        ":import_asset",
        ":import_fbx",
        "//lullaby/util:arg_parser",
        "//lullaby/util:filename",
        "//lullaby/util:math",
        "//lullaby/tools/common:file_utils",
    ],
)
