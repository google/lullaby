# Pipeline tool for converting 3D assets to Lullaby-compatible formats.

licenses(["notice"])  # Apache 2.0

package(
    default_visibility = ["//visibility:public"],
)

cc_library(
    name = "gltf_converter_lib",
    srcs = [
        "gltf_converter.cc",
    ],
    hdrs = [
        "gltf_converter.h",
    ],
    deps = [
        "//lullaby/util:common_types",
        "//lullaby/util:filename",
        "//lullaby/util:span",
        "//lullaby/util:string_view",
        "@rapidjson//:rapidjson",
    ],
)

cc_binary(
    name = "gltf_converter",
    srcs = [
        "main.cc",
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":gltf_converter_lib",
        "@flatbuffers//:flatbuffers",
        "//lullaby/util:arg_parser",
        "//lullaby/util:filename",
        "//lullaby/util:string_view",
        "//lullaby/tools/common:file_utils",
        "//lullaby/tools/common:log",
    ],
)
