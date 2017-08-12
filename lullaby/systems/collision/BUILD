
package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])  # Apache 2.0

cc_library(
    name = "collision",
    srcs = [
        "collision_system.cc",
    ],
    hdrs = [
        "collision_system.h",
    ],
    deps = [
        "//lullaby/generated",
        "//lullaby/events",
        "//lullaby/modules/ecs",
        "//lullaby/modules/flatbuffers",
        "//lullaby/systems/dispatcher",
        "//lullaby/systems/transform",
        "//lullaby/util:logging",
        "//lullaby/util:math",
    ],
)
