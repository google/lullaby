# Pipeline tool for packing textures into a KTX container.

licenses(["notice"])  # Apache 2.0

package(
    default_visibility = ["//visibility:public"],
)

cc_binary(
    name = "pack_ktx",
    srcs = [
        "pack_ktx.cc",
    ],
    deps = [
        ":pack_ktx_lib",
        "//lullaby/util:arg_parser",
        "//lullaby/util:filename",
    ],
)

cc_library(
    name = "pack_ktx_lib",
    srcs = [
        "ktx_astc_image.cc",
        "ktx_direct_image.cc",
        "ktx_image.cc",
    ],
    hdrs = [
        "ktx_astc_image.h",
        "ktx_direct_image.h",
        "ktx_image.h",
    ],
    deps = [
        "@ktx//:ktx",
        "//lullaby/util:logging",
    ],
)

