
package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])  # Apache 2.0

cc_library(
    name = "visibility",
    srcs = ["visibility_system.cc"],
    hdrs = ["visibility_system.h"],
    deps = [
        "//:fbs",
        "//lullaby/events",
        "//lullaby/modules/dispatcher",
        "//lullaby/modules/ecs",
        "//lullaby/modules/flatbuffers",
        "//lullaby/systems/dispatcher",
        "//lullaby/systems/transform",
        "//lullaby/util:math",
        "//lullaby/util:trace",
    ],
)
