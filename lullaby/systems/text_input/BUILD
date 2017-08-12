
package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])  # Apache 2.0

cc_library(
    name = "text_input",
    srcs = [
        "edit_text.cc",
        "text_input_system.cc",
    ],
    hdrs = [
        "edit_text.h",
        "text_input_system.h",
    ],
    deps = [
        "//lullaby/generated",
        "//lullaby/events",
        "//lullaby/modules/dispatcher",
        "//lullaby/modules/ecs",
        "//lullaby/modules/flatbuffers",
        "//lullaby/modules/input",
        "//lullaby/systems/dispatcher",
        "//lullaby/systems/render",
        "//lullaby/systems/transform",
        "//lullaby/util:clock",
        "//lullaby/util:logging",
        "//lullaby/util:math",
        "//lullaby/util:periodic_function",
        "//lullaby/util:string_preprocessor",
        "//lullaby/util:utf8",
    ],
)
