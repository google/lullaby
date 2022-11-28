# Line breaking implementation using libunibreak.

licenses(["notice"])

package(
    default_visibility = ["//redux:visibility"],
)

cc_library(
    name = "breaking",
    srcs = [
        "breaking.cc",
    ],
    deps = [
        "@libunibreak//:libunibreak",
        "//redux/engines/text",
    ],
)
