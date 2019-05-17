
package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])  # Apache 2.0

cc_library(
    name = "audio_channels",
    srcs = [
        "audio_channels.cc",
    ],
    hdrs = [
        "audio_channels.h",
    ],
    deps = [
        "//lullaby/systems/animation",
        "//lullaby/systems/audio",
        "//lullaby/util:registry",
    ],
)

cc_library(
    name = "deform_channels",
    srcs = [
        "deform_channels.cc",
    ],
    hdrs = [
        "deform_channels.h",
    ],
    deps = [
        "//lullaby/systems/animation",
        "//lullaby/contrib/deform",
        "//lullaby/util:logging",
        "//lullaby/util:registry",
    ],
)

cc_library(
    name = "nine_patch_channels",
    srcs = [
        "nine_patch_channels.cc",
    ],
    hdrs = [
        "nine_patch_channels.h",
    ],
    deps = [
        "//lullaby/systems/animation",
        "//lullaby/systems/nine_patch",
        "//lullaby/util:registry",
        "@motive//:motive",
    ],
)

cc_library(
    name = "render_channels",
    srcs = [
        "render_channels.cc",
    ],
    hdrs = [
        "render_channels.h",
    ],
    deps = [
        "//lullaby/modules/dispatcher",
        "//lullaby/systems/animation",
        "//lullaby/systems/render",
        "//lullaby/systems/render:render_helpers",
        "//lullaby/systems/transform",
        "//lullaby/util:color",
        "//lullaby/util:logging",
        "//lullaby/util:registry",
        "@motive//:motive",
    ],
)

cc_library(
    name = "skeleton_channel",
    srcs = [
        "skeleton_channel.cc",
    ],
    hdrs = [
        "skeleton_channel.h",
    ],
    deps = [
        "//lullaby/systems/animation",
        "//lullaby/systems/blend_shape",
        "//lullaby/systems/transform",
        "//lullaby/util:logging",
        "//lullaby/util:registry",
        "@motive//:motive",
    ],
)

cc_library(
    name = "transform_channels",
    srcs = [
        "transform_channels.cc",
    ],
    hdrs = [
        "transform_channels.h",
    ],
    deps = [
        "//lullaby/modules/dispatcher",
        "//lullaby/systems/animation",
        "//lullaby/systems/transform",
        "//lullaby/util:logging",
        "//lullaby/util:registry",
        "@mathfu//:mathfu",
        "@motive//:motive",
    ],
)
