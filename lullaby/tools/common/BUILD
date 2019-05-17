# Useful libraries for Lullaby tools.

package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])  # Apache 2.0

cc_library(
    name = "assimp_base_importer",
    srcs = [
        "assimp_base_importer.cc",
    ],
    hdrs = [
        "assimp_base_importer.h",
    ],
    deps = [
        "@assimp//:assimp",
        "//:fbs",
        "//lullaby/util:logging",
        "@mathfu//:mathfu",
    ],
)

cc_library(
    name = "fbx_base_importer",
    srcs = [
        "fbx_base_importer.cc",
    ],
    hdrs = [
        "fbx_base_importer.h",
    ],
    deps = [
        ":file_utils",
        "@libfbx//:libfbx",
        "//:fbs",
        "@mathfu//:mathfu",
    ],
)

cc_library(
    name = "file_utils",
    srcs = [
        "file_utils.cc",
    ],
    hdrs = [
        "file_utils.h",
    ],
)

cc_library(
    name = "jsonnet_utils",
    srcs = [
        "jsonnet_utils.cc",
    ],
    hdrs = [
        "jsonnet_utils.h",
    ],
    deps = [
        "@jsonnet//core:libjsonnet",
        "//lullaby/util:logging",
    ],
)

cc_library(
    name = "json_utils",
    srcs = [
        "json_utils.cc",
    ],
    hdrs = [
        "json_utils.h",
    ],
    deps = [
        ":file_utils",
        "@flatbuffers//:flatc_library",
        "@jsonnet//core:libjsonnet",
        "//lullaby/util:logging",
        "//lullaby/util:string_view",
    ],
)

cc_library(
    name = "log",
    srcs = [
        "log.cc",
    ],
    hdrs = [
        "log.h",
    ],
)
