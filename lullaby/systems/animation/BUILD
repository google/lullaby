
package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])  # Apache 2.0

cc_library(
    name = "animation",
    srcs = [
        "animation_asset.cc",
        "animation_channel.cc",
        "animation_system.cc",
    ],
    hdrs = [
        "animation_asset.h",
        "animation_channel.h",
        "animation_system.h",
        "playback_parameters.h",
        "spline_modifiers.h",
    ],
    deps = [
        "//lullaby/generated",
        "//lullaby/events",
        "//lullaby/modules/dispatcher",
        "//lullaby/modules/ecs",
        "//lullaby/modules/file",
        "//lullaby/modules/script",
        "//lullaby/systems/dispatcher",
        "//lullaby/util:clock",
        "//lullaby/util:hash",
        "//lullaby/util:logging",
        "//lullaby/util:make_unique",
        "//lullaby/util:registry",
        "//lullaby/util:resource_manager",
        "//lullaby/util:time",
        "//lullaby/util:trace",
        "//lullaby/util:typeid",
        "@motive//:motive",
        "//third_party/motive:motive_fbs",
    ],
)
