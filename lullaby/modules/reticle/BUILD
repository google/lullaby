
package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])  # Apache 2.0

# TODO Move standard_input_pipeline to modules/input_pipeline.
cc_library(
    name = "reticle",
    srcs = [
        "reticle_util.cc",
        "standard_input_pipeline.cc",
    ],
    hdrs = [
        "reticle_util.h",
        "standard_input_pipeline.h",
    ],
    deps = [
        ":input_focus_locker",
        "//lullaby/events",
        "//lullaby/modules/camera",
        "//lullaby/modules/input",
        "//lullaby/modules/input_processor",
        "//lullaby/modules/script",
        "//lullaby/systems/collision",
        "//lullaby/contrib/cursor",
        "//lullaby/contrib/input_behavior",
        "//lullaby/systems/transform",
        "//lullaby/util:clock",
        "//lullaby/util:device_util",
        "//lullaby/util:entity",
        "//lullaby/util:math",
        "//lullaby/util:optional",
        "//lullaby/util:registry",
        "//lullaby/util:span",
        "//lullaby/util:trace",
        "@mathfu//:mathfu",
    ],
)

cc_library(
    name = "reticle_jni",
    srcs = [
        "standard_input_pipeline_jni.cc",
    ],
    deps = [
        ":reticle",
        "//lullaby/modules/jni:registry_jni",
    ],
)

# TODO Move input_focus_locker to modules/input_pipeline.
cc_library(
    name = "input_focus_locker",
    srcs = [
        "input_focus_locker.cc",
    ],
    hdrs = [
        "input_focus_locker.h",
    ],
    deps = [
        "//lullaby/modules/input",
        "//lullaby/modules/input_processor",
        "//lullaby/systems/transform",
        "//lullaby/util:entity",
        "//lullaby/util:math",
        "//lullaby/util:registry",
        "@mathfu//:mathfu",
    ],
)
