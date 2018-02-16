
package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])  # Apache 2.0

cc_library(
    name = "input_processor",
    srcs = [
        "input_processor.cc",
    ],
    hdrs = [
        "input_processor.h",
    ],
    deps = [
        "//lullaby/events",
        "//lullaby/modules/dispatcher",
        "//lullaby/modules/input",
        "//lullaby/systems/dispatcher",
        "//lullaby/systems/transform",
        "//lullaby/util:bits",
        "//lullaby/util:clock",
        "//lullaby/util:entity",
        "//lullaby/util:enum_hash",
        "//lullaby/util:math",
        "//lullaby/util:registry",
        "//lullaby/util:variant",
    ],
)
