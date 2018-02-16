# Pipeline tool for converting 3D assets to Lullaby-compatible formats.

licenses(["notice"])  # Apache 2.0

cc_library(
    name = "model_lib",
    srcs = [
        "model.cc",
        "util.cc",
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
        "//:fbs",
        "//lullaby/util:bits",
        "//lullaby/util:filename",
        "//lullaby/util:typeid",
        "//lullaby/util:variant",
        "//third_party/lullaby/tools/common",
        "@mathfu//:mathfu",
    ],
)

cc_library(
    name = "import_asset",
    srcs = [
        "import_asset.cc",
    ],
    deps = [
        ":model_lib",
        "//third_party/assimp:assimp_no_google",
        "//:fbs",
        "//lullaby/util:common_types",
        "//lullaby/util:filename",
    ],
)

cc_library(
    name = "import_fbx",
    srcs = [
        "import_fbx.cc",
    ],
    deps = [
        ":model_lib",
        "//third_party/libfbx:2016_1_2",
        "//:fbs",
        "//lullaby/util:common_types",
        "//lullaby/util:filename",
    ],
)

cc_library(
    name = "model_exporters",
    srcs = [
        "export.cc",
    ],
    hdrs = [
        "export.h",
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":model_lib",
        "@flatbuffers//:flatbuffers",
        "//:fbs",
        "//lullaby/util:filename",
        "//lullaby/util:flatbuffer_writer",
        "//lullaby/util:inward_buffer",
        "@mathfu//:mathfu",
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
        ":model_exporters",
        ":model_lib",
        "@flatbuffers//:flatbuffers",
        "//:fbs",
        "//lullaby/modules/flatbuffers",
        "//lullaby/util:filename",
        "//lullaby/util:flatbuffer_reader",
        "//lullaby/util:flatbuffer_writer",
        "//lullaby/util:inward_buffer",
        "//lullaby/util:logging",
        "//lullaby/util:span",
        "//lullaby/util:string_view",
        "//third_party/lullaby/tools/common",
    ],
)

cc_binary(
    name = "model_pipeline",
    srcs = [
        "main.cc",
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":import_asset",
        ":import_fbx",
        ":model_pipeline_lib",
        "@flatbuffers//:flatbuffers",
        "//lullaby/util:arg_parser",
        "//lullaby/util:filename",
        "//third_party/lullaby/tools/common",
    ],
)
