
package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])  # Apache 2.0

cc_library(
    name = "cursor",
    srcs = [
        "cursor_system.cc",
        "cursor_trail_system.cc",
    ],
    hdrs = [
        "cursor_system.h",
        "cursor_trail_system.h",
    ],
    deps = [
        "//lullaby/generated",
        "//lullaby/events",
        "//lullaby/modules/animation_channels:render_channels",
        "//lullaby/modules/animation_channels:transform_channels",
        "//lullaby/modules/config",
        "//lullaby/modules/dispatcher",
        "//lullaby/modules/ecs",
        "//lullaby/modules/flatbuffers",
        "//lullaby/modules/input",
        "//lullaby/modules/render",
        "//lullaby/systems/animation",
        "//lullaby/systems/collision",
        "//lullaby/systems/dispatcher",
        "//lullaby/systems/render",
        "//lullaby/systems/transform",
        "//lullaby/util:bits",
        "//lullaby/util:clock",
        "//lullaby/util:math",
        "//lullaby/util:time",
        "//lullaby/util:trace",
        "@mathfu//:mathfu",
    ],
)
