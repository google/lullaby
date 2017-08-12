
package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])  # Apache 2.0

cc_library(
    name = "map_events",
    srcs = ["map_events_system.cc"],
    hdrs = ["map_events_system.h"],
    deps = [
        "//lullaby/generated",
        "//lullaby/modules/ecs",
        "//lullaby/systems/dispatcher",
        "//lullaby/systems/transform",
        "//lullaby/util:logging",
        "//lullaby/util:math",
    ],
)
