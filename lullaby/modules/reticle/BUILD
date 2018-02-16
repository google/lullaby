
package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])  # Apache 2.0

cc_library(
    name = "reticle",
    srcs = [
        "input_focus_locker.cc",
        "reticle_util.cc",
        "standard_input_pipeline.cc",
    ],
    hdrs = [
        "input_focus_locker.h",
        "reticle_util.h",
        "standard_input_pipeline.h",
    ],
    deps = [
        "//lullaby/events",
        "//lullaby/modules/input",
        "//lullaby/modules/input_processor",
        "//lullaby/systems/collision",
        "//lullaby/systems/cursor",
        "//lullaby/systems/input_behavior",
        "//lullaby/systems/transform",
        "//lullaby/util:clock",
        "//lullaby/util:controller_util",
        "//lullaby/util:entity",
        "//lullaby/util:math",
        "//lullaby/util:registry",
        "//lullaby/util:span",
        "//lullaby/util:trace",
        "@mathfu//:mathfu",
    ],
)
