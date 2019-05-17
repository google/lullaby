
package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])  # Apache 2.0

cc_library(
    name = "events",
    hdrs = [
        "animation_events.h",
        "audio_events.h",
        "battery_state_changed.h",
        "camera_events.h",
        "controller_events.h",
        "entity_events.h",
        "fade_events.h",
        "grab_events.h",
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
        "//lullaby/modules/input",
        "//lullaby/util:entity",
        "//lullaby/util:hash",
        "//lullaby/util:math",
        "//lullaby/util:optional",
        "//lullaby/util:typeid",
        "@mathfu//:mathfu",
    ],
)
