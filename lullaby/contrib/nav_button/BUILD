
package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])  # Apache 2.0

cc_library(
    name = "nav_button",
    srcs = ["nav_button_system.cc"],
    hdrs = ["nav_button_system.h"],
    deps = [
        "//:fbs",
        "//lullaby/events",
        "//lullaby/modules/animation_channels:render_channels",
        "//lullaby/modules/ecs",
        "//lullaby/modules/flatbuffers",
        "//lullaby/systems/animation",
        "//lullaby/systems/dispatcher",
        "//lullaby/systems/render",
        "//lullaby/systems/text",
        "//lullaby/systems/transform",
        "//lullaby/util:logging",
        "@mathfu//:mathfu",
    ],
)
