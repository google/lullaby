# Pipeline tool for converting 3D assets to Lullaby-compatible formats.

licenses(["notice"])  # Apache 2.0

package(
    default_visibility = ["//visibility:public"],
)

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
        "//lullaby/modules/render:tangent_generation",
        "//lullaby/util:bits",
        "//lullaby/util:filename",
        "//lullaby/util:math",
        "//lullaby/util:typeid",
        "//lullaby/util:variant",
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
        "@assimp//:assimp",
        "//:fbs",
        "//lullaby/util:filename",
        "//lullaby/util:make_unique",
        "//lullaby/tools/common:assimp_base_importer",
    ],
)

cc_library(
    name = "import_fbx",
    srcs = [
        "import_fbx.cc",
    ],
    copts = ["-Wno-null-dereference"],
    deps = [
        ":model_lib",
        "@libfbx//:libfbx",
        "//:fbs",
        "//lullaby/util:common_types",
        "//lullaby/tools/common:fbx_base_importer",
    ],
)

cc_library(
    name = "export_options",
    hdrs = [
        "export_options.h",
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
        ":export_options",
        ":model_lib",
        "//:fbs",
        "//lullaby/util:filename",
        "//lullaby/util:flatbuffer_writer",
        "//lullaby/util:inward_buffer",
        "//lullaby/tools/common:log",
        "@mathfu//:mathfu",
        "//third_party/openssl:crypto",
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
        ":export_options",
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
        "//lullaby/tools/common:file_utils",
    ],
)

cc_binary(
    name = "model_pipeline",
    srcs = [
        "main.cc",
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":export_options",
        ":import_asset",
        ":import_fbx",
        ":model_pipeline_lib",
        "@flatbuffers//:flatbuffers",
        "//lullaby/util:arg_parser",
        "//lullaby/util:filename",
        "//lullaby/tools/common:file_utils",
        "//lullaby/tools/common:log",
    ],
)
