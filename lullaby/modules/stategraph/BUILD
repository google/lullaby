
package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])  # Apache 2.0

cc_library(
    name = "stategraph",
    srcs = [
        "stategraph.cc",
        "stategraph_track.cc",
    ],
    hdrs = [
        "stategraph.h",
        "stategraph_signal.h",
        "stategraph_state.h",
        "stategraph_track.h",
        "stategraph_transition.h",
    ],
    deps = [
        "//lullaby/util:clock",
        "//lullaby/util:hash",
        "//lullaby/util:logging",
        "//lullaby/util:optional",
        "//lullaby/util:selector",
        "//lullaby/util:span",
        "//lullaby/util:typed_pointer",
        "//lullaby/util:typeid",
        "//lullaby/util:variant",
        "@mathfu//:mathfu",
    ],
)
