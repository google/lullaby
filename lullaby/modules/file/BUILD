
package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])  # Apache 2.0

cc_library(
    name = "file",
    srcs = [
        "asset_loader.cc",
        "file.cc",
        "file_loader.cc",
    ],
    hdrs = [
        "asset.h",
        "asset_loader.h",
        "file.h",
        "file_loader.h",
    ],
    deps = [
        "@fplbase//:fplbase",
        "//lullaby/util:async_processor",
        "//lullaby/util:logging",
        "//lullaby/util:time",
        "//lullaby/util:typeid",
    ],
)

