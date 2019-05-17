
package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])  # Apache 2.0

cc_library(
    name = "controller",
    srcs = ["controller_system.cc"],
    hdrs = ["controller_system.h"],
    deps = [
        "//:fbs",
        "//lullaby/contrib/fade",
        "//lullaby/contrib/fpl_mesh",
        "//lullaby/events",
        "//lullaby/modules/dispatcher",
        "//lullaby/modules/ecs",
        "//lullaby/modules/flatbuffers",
        "//lullaby/modules/input",
        "//lullaby/modules/input_processor",
        "//lullaby/modules/reticle",
        "//lullaby/systems/dispatcher",
        "//lullaby/systems/render",
        "//lullaby/systems/transform",
        "//lullaby/util:clock",
        "//lullaby/util:device_util",
        "//lullaby/util:interpolation",
        "//lullaby/util:math",
        "//lullaby/util:time",
        "//lullaby/util:trace",
    ],
)
