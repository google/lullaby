
package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])  # Apache 2.0

cc_library(
    name = "slideshow",
    srcs = ["slideshow_system.cc"],
    hdrs = ["slideshow_system.h"],
    deps = [
        "//:fbs",
        "//lullaby/events",
        "//lullaby/modules/animation_channels:transform_channels",
        "//lullaby/modules/dispatcher",
        "//lullaby/modules/ecs",
        "//lullaby/systems/animation",
        "//lullaby/systems/dispatcher",
        "//lullaby/systems/transform",
        "//lullaby/util:clock",
        "//lullaby/util:logging",
    ],
)
