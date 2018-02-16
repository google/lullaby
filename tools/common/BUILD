# Useful libraries for Lullaby tools.

licenses(["notice"])  # Apache 2.0

cc_library(
    name = "common",
    srcs = [
        "file_utils.cc",
    ],
    hdrs = [
        "file_utils.h",
    ],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "json_utils",
    srcs = [
        "json_utils.cc",
    ],
    hdrs = [
        "json_utils.h",
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":common",
        "@flatbuffers//:flatc_library",
        "//third_party/jsonnet:libjsonnet",
        "//lullaby/util:logging",
        "//lullaby/util:string_view",
    ],
)
