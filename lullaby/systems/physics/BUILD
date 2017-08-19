
package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])  # Apache 2.0

cc_library(
    name = "physics",
    srcs = [
        "bullet_utils.cc",
        "physics_system.cc",
    ],
    hdrs = [
        "bullet_utils.h",
        "physics_system.h",
    ],
    deps = [
        "@bullet//:BulletCollision",
        "@bullet//:BulletDynamics",
        "//lullaby/generated",
        "//lullaby/events",
        "//lullaby/modules/ecs",
        "//lullaby/modules/flatbuffers",
        "//lullaby/systems/dispatcher",
        "//lullaby/systems/transform",
        "//lullaby/util:bits",
        "//lullaby/util:clock",
        "//lullaby/util:logging",
        "//lullaby/util:make_unique",
        "//lullaby/util:math",
        "//lullaby/util:time",
        "//lullaby/util:trace",
    ],
)
