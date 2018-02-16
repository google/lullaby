
package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])  # Apache 2.0

cc_library(
    name = "snap",
    srcs = ["snap_system.cc"],
    hdrs = ["snap_system.h"],
    deps = [
        "//:fbs",
        "//lullaby/events",
        "//lullaby/modules/animation_channels:transform_channels",
        "//lullaby/modules/ecs",
        "//lullaby/systems/animation",
        "//lullaby/systems/transform",
        "//lullaby/util:clock",
        "//lullaby/util:entity",
        "//lullaby/util:logging",
        "//lullaby/util:math",
    ],
)
