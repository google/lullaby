
package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])  # Apache 2.0

cc_library(
    name = "fade",
    srcs = ["fade_system.cc"],
    hdrs = ["fade_system.h"],
    deps = [
        "//:fbs",
        "//lullaby/events",
        "//lullaby/modules/animation_channels:render_channels",
        "//lullaby/modules/dispatcher",
        "//lullaby/modules/ecs",
        "//lullaby/systems/animation",
        "//lullaby/systems/collision",
        "//lullaby/systems/dispatcher",
        "//lullaby/systems/render",
        "//lullaby/systems/transform",
        "//lullaby/util:clock",
        "//lullaby/util:logging",
        "@mathfu//:mathfu",
    ],
)
