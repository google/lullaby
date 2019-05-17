
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
        "collision_provider.h",
        "collision_system.h",
    ],
    deps = [
        "//:fbs",
        "//lullaby/events",
        "//lullaby/modules/ecs",
        "//lullaby/modules/flatbuffers",
        "//lullaby/systems/dispatcher",
        "//lullaby/systems/transform",
        "//lullaby/util:entity",
        "//lullaby/util:hash",
        "//lullaby/util:logging",
        "//lullaby/util:math",
    ],
)
