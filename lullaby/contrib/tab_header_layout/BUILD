# Description:
#   tab_header_layout, manages a set of tab headers that the user can switch
#   between. This is similar to Android's TabLayout control.


package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])  # Apache 2.0

cc_library(
    name = "tab_header_layout",
    srcs = ["tab_header_layout_system.cc"],
    hdrs = ["tab_header_layout_system.h"],
    deps = [
        "//:fbs",
        "//lullaby/events",
        "//lullaby/modules/animation_channels:render_channels",
        "//lullaby/modules/animation_channels:transform_channels",
        "//lullaby/modules/ecs",
        "//lullaby/modules/flatbuffers",
        "//lullaby/systems/animation",
        "//lullaby/systems/collision",
        "//lullaby/contrib/deform",
        "//lullaby/systems/dispatcher",
        "//lullaby/contrib/layout",
        "//lullaby/contrib/layout:layout_box",
        "//lullaby/systems/render",
        "//lullaby/systems/text",
        "//lullaby/systems/transform",
        "//lullaby/util:clock",
        "//lullaby/util:entity",
        "//lullaby/util:hash",
        "//lullaby/util:logging",
        "//lullaby/util:math",
        "//lullaby/util:registry",
        "@mathfu//:mathfu",
    ],
)
