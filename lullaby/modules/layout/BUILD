
package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])  # Apache 2.0

cc_library(
    name = "layout",
    srcs = [
        "layout.cc",
    ],
    hdrs = [
        "layout.h",
        "layout_manager.h",
    ],
    deps = [
        "//:fbs",
        "//lullaby/contrib/layout:layout_box",
        "//lullaby/systems/transform",
        "//lullaby/util:clock",
        "//lullaby/util:entity",
        "//lullaby/util:logging",
        "//lullaby/util:math",
        "//lullaby/util:registry",
        "@mathfu//:mathfu",
    ],
)
