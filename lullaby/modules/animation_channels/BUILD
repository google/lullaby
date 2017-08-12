
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
    name = "render_channels",
    srcs = [
        "render_channels.cc",
    ],
    hdrs = [
        "render_channels.h",
    ],
    deps = [
        "//lullaby/systems/animation",
        "//lullaby/systems/render",
        "//lullaby/systems/render:render_helpers",
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
        "//lullaby/systems/animation",
        "//lullaby/systems/transform",
        "//lullaby/util:logging",
        "//lullaby/util:registry",
        "@mathfu//:mathfu",
        "@motive//:motive",
    ],
)
