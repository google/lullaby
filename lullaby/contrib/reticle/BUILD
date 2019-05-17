
package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])  # Apache 2.0

config_setting(
    name = "lullaby_hmd_reticle",
    values = {"define": "lullaby_hmd_reticle=1"},
)

cc_library(
    name = "reticle",
    srcs = [
        "reticle_bounded_movement_system.cc",
        "reticle_system.cc",
        "reticle_trail_system.cc",
    ],
    hdrs = [
        "reticle_bounded_movement_system.h",
        "reticle_system.h",
        "reticle_trail_system.h",
    ],
    defines = select({
        ":lullaby_hmd_reticle": ["LULLABY_HMD_RETICLE=1"],
        "//conditions:default": [],
    }) + select({
        "//conditions:default": ["LULLABY_ENABLE_TEST_ENGINE=0"],
    }),
    deps = [
        "//:fbs",
        "//lullaby/events",
        "//lullaby/modules/animation_channels:render_channels",
        "//lullaby/modules/animation_channels:transform_channels",
        "//lullaby/modules/config",
        "//lullaby/modules/dispatcher",
        "//lullaby/modules/ecs",
        "//lullaby/modules/flatbuffers",
        "//lullaby/modules/input",
        "//lullaby/modules/reticle",
        "//lullaby/modules/reticle:input_focus_locker",
        "//lullaby/systems/animation",
        "//lullaby/systems/collision",
        "//lullaby/contrib/cursor",
        "//lullaby/systems/dispatcher",
        "//lullaby/contrib/input_behavior",
        "//lullaby/systems/render",
        "//lullaby/systems/transform",
        "//lullaby/util:bits",
        "//lullaby/util:clock",
        "//lullaby/util:device_util",
        "//lullaby/util:math",
        "//lullaby/util:trace",
        "@mathfu//:mathfu",
    ],
)
