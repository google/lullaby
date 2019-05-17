
package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])  # Apache 2.0

public_headers = [
    "html_tags.h",
    "text_system.h",
    "detail/util.h",
]

# 'text_system' is the target for the API.  Use this for other systems that
# depend on text_system.  Apps will need to choose an implementation (below).
cc_library(
    name = "text",
    srcs = [
        "text_system.cc",
    ],
    hdrs = public_headers,
    deps = [
        "//:fbs",
        "//lullaby/modules/dispatcher",
        "//lullaby/modules/ecs",
        "//lullaby/modules/script",
        "//lullaby/util:math",
        "//lullaby/util:string_preprocessor",
        "@mathfu//:mathfu",
    ],
)

# The flatui implementation.  Supports basic HTML parsing and i18n line
# breaking.  Currently some dependencies prevent compiling for windows.
cc_library(
    name = "flatui",
    srcs = [
        "flatui/flatui_text_system.cc",
        "flatui/font.cc",
        "flatui/text_buffer.cc",
        "flatui/text_task.cc",
    ],
    hdrs = public_headers + [
        "flatui/flatui_text_system.h",
        "flatui/font.h",
        "flatui/text_buffer.h",
        "flatui/text_component.h",
        "flatui/text_task.h",
    ],
    defines = ["LULLABY_TEXT_BACKEND_FLATUI"],
    deps = [
        ":text",
        "@flatui//:flatui",
        "@fplbase//:fplbase",
        "@fplbase//:glplatform",
        "//:fbs",
        "//lullaby/events",
        "//lullaby/modules/dispatcher",
        "//lullaby/modules/ecs",
        "//lullaby/modules/flatbuffers",
        "//lullaby/modules/render",
        "//lullaby/contrib/deform",
        "//lullaby/systems/dispatcher",
        "//lullaby/contrib/layout:layout_box",
        "//lullaby/systems/render",
        "//lullaby/systems/transform",
        "//lullaby/util:android_context",
        "//lullaby/util:async_processor",
        "//lullaby/util:entity",
        "//lullaby/util:filename",
        "//lullaby/util:logging",
        "//lullaby/util:math",
        "//lullaby/util:optional",
        "//lullaby/util:string_preprocessor",
        "//lullaby/util:trace",
        "//lullaby/util:typeid",
        "@mathfu//:mathfu",
    ],
)


# The GMock implementation.
cc_library(
    name = "mock",
    testonly = 1,
    hdrs = public_headers + [
        "testing/mock_text_system.h",
    ],
    deps = [
        ":text",
        "@gtest//:gtest",
        "//:fbs",
        "//lullaby/util:math",
        "@mathfu//:mathfu",
    ],
)

cc_library(
    name = "stub",
    testonly = 1,
    srcs = [
        "testing/stub_text_system.cc",
    ],
    hdrs = public_headers + [
        "testing/stub_text_system.h",
    ],
    deps = [
        ":text",
        "//:fbs",
        "//lullaby/modules/ecs",
        "//lullaby/util:math",
        "@mathfu//:mathfu",
    ],
)
