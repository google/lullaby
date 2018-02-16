
package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])  # Apache 2.0

cc_library(
    name = "clip",
    srcs = ["clip_system.cc"],
    hdrs = ["clip_system.h"],
    deps = [
        "//:fbs",
        "//lullaby/events",
        "//lullaby/modules/dispatcher",
        "//lullaby/modules/ecs",
        "//lullaby/systems/render",
        "//lullaby/systems/transform",
        "//lullaby/util:logging",
    ],
)
