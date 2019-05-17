
package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])  # Apache 2.0

cc_library(
    name = "config",
    srcs = [
        "config.cc",
    ],
    hdrs = [
        "config.h",
    ],
    deps = [
        "//:fbs",
        "//lullaby/modules/file",
        "//lullaby/modules/flatbuffers",
        "//lullaby/util:flatbuffer_reader",
        "//lullaby/util:registry",
        "//lullaby/util:string_view",
        "//lullaby/util:typeid",
        "//lullaby/util:variant",
    ],
)
