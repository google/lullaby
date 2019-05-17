# Description:
#   Tooltips for any arbitrary device.


package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])  # Apache 2.0

cc_library(
    name = "device_tooltips",
    srcs = ["device_tooltips.cc"],
    hdrs = ["device_tooltips.h"],
    deps = [
        "//lullaby/events",
        "//lullaby/modules/dispatcher",
        "//lullaby/modules/ecs",
        "//lullaby/modules/input",
        "//lullaby/systems/dispatcher",
        "//lullaby/contrib/layout:layout_box",
        "//lullaby/systems/text",
        "//lullaby/systems/transform",
        "//lullaby/util:entity",
        "//lullaby/util:enum_hash",
        "//lullaby/util:registry",
    ],
)
