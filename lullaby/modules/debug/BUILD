
package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])  # Apache 2.0

cc_library(
    name = "debug",
    srcs = [
        "debug_camera.cc",
        "debug_render.cc",
        "debug_render_impl.cc",
        "log_tag.cc",
    ],
    hdrs = [
        "debug_camera.h",
        "debug_render.h",
        "debug_render_draw_interface.h",
        "debug_render_impl.h",
        "log_tag.h",
    ],
    deps = [
        "//lullaby/events",
        "//lullaby/modules/dispatcher",
        "//lullaby/modules/input",
        "//lullaby/modules/render",
        "//lullaby/systems/render",
        "//lullaby/util:clock",
        "//lullaby/util:color",
        "//lullaby/util:hash",
        "//lullaby/util:logging",
        "//lullaby/util:math",
        "//lullaby/util:registry",
        "//lullaby/util:string_view",
        "//lullaby/util:time",
        "@mathfu//:mathfu",
    ],
)
