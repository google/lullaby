
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
        "//:fbs",
        "//lullaby/events",
        "//lullaby/modules/dispatcher",
        "//lullaby/modules/ecs",
        "//lullaby/modules/file",
        "//lullaby/modules/script",
        "//lullaby/systems/dispatcher",
        "//lullaby/util:clock",
        "//lullaby/util:data_container",
        "//lullaby/util:hash",
        "//lullaby/util:logging",
        "//lullaby/util:make_unique",
        "//lullaby/util:registry",
        "//lullaby/util:resource_manager",
        "//lullaby/util:span",
        "//lullaby/util:time",
        "//lullaby/util:trace",
        "//lullaby/util:typeid",
        "@motive//:motive",
        "@motive//:motive_fbs",
    ],
)

cc_library(
    name = "animation_jni",
    srcs = [
        "animation_system_jni.cc",
    ],
    deps = [
        ":animation",
        "//lullaby/modules/ecs:ecs_jni",
    ],
)
