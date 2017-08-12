
package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])  # Apache 2.0

cc_library(
    name = "scroll",
    srcs = [
        "scroll_channels.cc",
        "scroll_content_layout_system.cc",
        "scroll_snap_to_grandchildren_system.cc",
        "scroll_snap_to_grid_system.cc",
        "scroll_system.cc",
    ],
    hdrs = [
        "scroll_channels.h",
        "scroll_content_layout_system.h",
        "scroll_snap_to_grandchildren_system.h",
        "scroll_snap_to_grid_system.h",
        "scroll_system.h",
    ],
    deps = [
        "//lullaby/generated",
        "//lullaby/events",
        "//lullaby/modules/dispatcher",
        "//lullaby/modules/ecs",
        "//lullaby/modules/ecs:entity",
        "//lullaby/modules/flatbuffers",
        "//lullaby/modules/input",
        "//lullaby/systems/animation",
        "//lullaby/systems/dispatcher",
        "//lullaby/systems/transform",
        "//lullaby/util:bits",
        "//lullaby/util:clock",
        "//lullaby/util:logging",
        "//lullaby/util:math",
        "//lullaby/util:registry",
        "//lullaby/util:time",
        "//lullaby/util:trace",
        "@mathfu//:mathfu",
        "@motive//:motive",
    ],
)
