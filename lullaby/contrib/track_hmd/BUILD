
package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])  # Apache 2.0

cc_library(
    name = "track_hmd",
    srcs = ["track_hmd_system.cc"],
    hdrs = ["track_hmd_system.h"],
    deps = [
        "//:fbs",
        "//lullaby/modules/ecs",
        "//lullaby/modules/flatbuffers",
        "//lullaby/modules/input",
        "//lullaby/systems/transform",
        "//lullaby/util:clock",
        "//lullaby/util:math",
        "//lullaby/util:trace",
        "@mathfu//:mathfu",
    ],
)
