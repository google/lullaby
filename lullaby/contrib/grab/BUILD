
package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])  # Apache 2.0

cc_library(
    name = "grab",
    srcs = ["grab_system.cc"],
    hdrs = ["grab_system.h"],
    deps = [
        "//:fbs",
        "//lullaby/contrib/mutator",
        "//lullaby/events",
        "//lullaby/modules/ecs",
        "//lullaby/modules/flatbuffers",
        "//lullaby/modules/input",
        "//lullaby/systems/dispatcher",
        "//lullaby/contrib/input_behavior",
        "//lullaby/systems/transform",
        "//lullaby/util:math",
    ],
)

cc_library(
    name = "planar_grab_input",
    srcs = ["planar_grab_input_system.cc"],
    hdrs = ["planar_grab_input_system.h"],
    deps = [
        ":grab",
        "//:fbs",
        "//lullaby/modules/ecs",
        "//lullaby/modules/flatbuffers",
        "//lullaby/modules/input",
        "//lullaby/modules/input_processor",
        "//lullaby/modules/reticle:input_focus_locker",
        "//lullaby/systems/dispatcher",
        "//lullaby/systems/transform",
        "//lullaby/util:math",
        "@mathfu//:mathfu",
    ],
)

cc_library(
    name = "spatial_grab_input",
    srcs = ["spatial_grab_input_system.cc"],
    hdrs = ["spatial_grab_input_system.h"],
    deps = [
        ":grab",
        "//:fbs",
        "//lullaby/modules/ecs",
        "//lullaby/modules/flatbuffers",
        "//lullaby/modules/input",
        "//lullaby/modules/input_processor",
        "//lullaby/modules/reticle:input_focus_locker",
        "//lullaby/systems/dispatcher",
        "//lullaby/systems/transform",
        "//lullaby/util:math",
        "@mathfu//:mathfu",
    ],
)

cc_library(
    name = "spherical_grab_input",
    srcs = ["spherical_grab_input_system.cc"],
    hdrs = ["spherical_grab_input_system.h"],
    deps = [
        ":grab",
        "//:fbs",
        "//lullaby/contrib/controller",
        "//lullaby/modules/ecs",
        "//lullaby/modules/flatbuffers",
        "//lullaby/modules/input",
        "//lullaby/modules/input_processor",
        "//lullaby/modules/reticle:input_focus_locker",
        "//lullaby/contrib/cursor",
        "//lullaby/systems/dispatcher",
        "//lullaby/systems/transform",
        "//lullaby/util:math",
        "@mathfu//:mathfu",
    ],
)
