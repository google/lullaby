# Useful libraries for Lullaby tools.

package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])  # Apache 2.0

cc_library(
    name = "fbx_utils",
    srcs = [
        "fbx_utils.cc",
    ],
    hdrs = [
        "fbx_utils.h",
    ],
    deps = [
        "//third_party/libfbx:libfbx",
        "//:fbs",
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
