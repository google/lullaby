
package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])  # Apache 2.0

cc_library(
    name = "layout_box",
    srcs = ["layout_box_system.cc"],
    hdrs = ["layout_box_system.h"],
    deps = [
        "//lullaby/events",
        "//lullaby/modules/ecs",
        "//lullaby/modules/script",
        "//lullaby/systems/dispatcher",
        "//lullaby/systems/transform",
        "//lullaby/util:math",
        "//lullaby/util:optional",
    ],
)

cc_library(
    name = "layout",
    srcs = ["layout_system.cc"],
    hdrs = ["layout_system.h"],
    deps = [
        ":layout_box",
        "//lullaby/generated",
        "//lullaby/events",
        "//lullaby/modules/dispatcher",
        "//lullaby/modules/ecs",
        "//lullaby/modules/flatbuffers",
        "//lullaby/modules/layout",
        "//lullaby/modules/script",
        "//lullaby/systems/dispatcher",
        "//lullaby/systems/transform",
        "//lullaby/util:logging",
        "//lullaby/util:math",
    ],
)
