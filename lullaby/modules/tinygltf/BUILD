
package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])  # Apache 2.0

cc_library(
    name = "tinygltf",
    srcs = [
        "tinygltf_util.cc",
    ],
    hdrs = [
        "tinygltf_util.h",
    ],
    deps = [
        "//lullaby/util:optional",
        "//lullaby/util:span",
        "@motive//:motive",
        "@tinygltf//:tinygltf",
    ],
)
