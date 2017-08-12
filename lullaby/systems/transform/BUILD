
package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])  # Apache 2.0

cc_library(
    name = "transform",
    srcs = ["transform_system.cc"],
    hdrs = ["transform_system.h"],
    deps = [
        "//lullaby/generated",
        "//lullaby/events",
        "//lullaby/modules/dispatcher",
        "//lullaby/modules/ecs",
        "//lullaby/modules/flatbuffers",
        "//lullaby/modules/script",
        "//lullaby/systems/dispatcher",
        "//lullaby/util:bits",
        "//lullaby/util:logging",
        "//lullaby/util:math",
        "@mathfu//:mathfu",
    ],
)
