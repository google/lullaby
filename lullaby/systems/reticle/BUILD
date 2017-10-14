
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
        "reticle_system.cc",
        "reticle_trail_system.cc",
    ],
    hdrs = [
        "reticle_system.h",
        "reticle_system_reticle_provider.h",
        "reticle_trail_system.h",
    ],
    defines = select({
        ":lullaby_hmd_reticle": ["LULLABY_HMD_RETICLE=1"],
        "//conditions:default": [],
    }),
    deps = [
        "//lullaby/generated",
        "//lullaby/events",
        "//lullaby/modules/animation_channels:render_channels",
        "//lullaby/modules/animation_channels:transform_channels",
        "//lullaby/modules/config",
        "//lullaby/modules/dispatcher",
        "//lullaby/modules/ecs",
        "//lullaby/modules/ecs:entity",
        "//lullaby/modules/flatbuffers",
        "//lullaby/modules/input",
        "//lullaby/modules/reticle",
        "//lullaby/systems/animation",
        "//lullaby/systems/collision",
        "//lullaby/systems/cursor",
        "//lullaby/systems/dispatcher",
        "//lullaby/systems/render",
        "//lullaby/systems/transform",
        "//lullaby/util:bits",
        "//lullaby/util:clock",
        "//lullaby/util:math",
        "//lullaby/util:trace",
        "@mathfu//:mathfu",
    ],
)
