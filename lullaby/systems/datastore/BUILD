
package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])  # Apache 2.0

cc_library(
    name = "datastore",
    srcs = ["datastore_system.cc"],
    hdrs = ["datastore_system.h"],
    deps = [
        "//lullaby/generated",
        "//lullaby/modules/ecs",
        "//lullaby/modules/flatbuffers",
        "//lullaby/modules/script",
        "//lullaby/util:common_types",
        "//lullaby/util:logging",
        "//lullaby/util:variant",
    ],
)
