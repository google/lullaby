
package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])  # Apache 2.0

cc_library(
    name = "file",
    srcs = [
        "asset_loader.cc",
    ],
    hdrs = [
        "asset.h",
        "asset_loader.h",
    ],
    deps = [
        "//lullaby/util:android_context",
        "//lullaby/util:async_processor",
        "//lullaby/util:filename",
        "//lullaby/util:logging",
        "//lullaby/util:registry",
        "//lullaby/util:time",
        "//lullaby/util:typeid",
    ],
)

cc_library(
    name = "file_loader",
    srcs = [
        "file_loader.cc",
        "tagged_file_loader.cc",
    ],
    hdrs = [
        "file_loader.h",
        "tagged_file_loader.h",
    ],
    deps = [
        "@fplbase//:utilities",
        "//lullaby/util:filename",
        "//lullaby/util:logging",
    ],
)

cc_library(
    name = "tagged_file_loader",
    srcs = [
        "tagged_file_loader.cc",
    ],
    hdrs = [
        "tagged_file_loader.h",
    ],
    deps = [
        ":file_loader",
        "@fplbase//:utilities",
        "//lullaby/util:filename",
        "//lullaby/util:logging",
        "//lullaby/util:typeid",
    ],
)

cc_library(
    name = "mock_tagged_file_loader",
    srcs = [
        "test/mock_tagged_file_loader.cc",
    ],
    hdrs = [
        "test/mock_tagged_file_loader.h",
    ],
    deps = [
        ":tagged_file_loader",
        "@fplbase//:fplbase",
        "//lullaby/util:filename",
        "//lullaby/util:logging",
        "//lullaby/util:typeid",
    ],
)
