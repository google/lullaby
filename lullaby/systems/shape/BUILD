
package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])  # Apache 2.0

cc_library(
    name = "shape",
    srcs = ["shape_system.cc"],
    hdrs = ["shape_system.h"],
    deps = [
        "//:fbs",
        "//lullaby/modules/ecs",
        "//lullaby/modules/render",
        "//lullaby/systems/render",
    ],
)
