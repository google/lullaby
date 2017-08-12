
package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])  # Apache 2.0

cc_library(
    name = "events",
    hdrs = [
        "animation_events.h",
        "audio_events.h",
        "entity_events.h",
        "input_events.h",
        "layout_events.h",
        "lifetime_events.h",
        "physics_events.h",
        "render_events.h",
        "scroll_events.h",
        "text_events.h",
        "ui_events.h",
    ],
    deps = [
        "//lullaby/modules/ecs:entity",
        "//lullaby/util:hash",
        "//lullaby/util:math",
        "//lullaby/util:optional",
        "//lullaby/util:typeid",
        "@mathfu//:mathfu",
    ],
)
