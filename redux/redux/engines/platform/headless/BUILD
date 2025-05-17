# Headless backend for platform abstraction library.

licenses(["notice"])

package(
    
    default_visibility = ["//redux:visibility"],
)

cc_library(
    name = "headless",
    srcs = ["headless_mainloop.cc"],
    hdrs = [
        "headless_mainloop.h",
    ],
    deps = [
        "@absl//absl/log:check",
        "@absl//absl/status",
        "@absl//absl/types:span",
        "//redux/engines/platform:device_manager",
        "//redux/engines/platform:device_profiles",
        "//redux/engines/platform:display",
        "//redux/engines/platform:keyboard",
        "//redux/engines/platform:mainloop",
        "//redux/engines/platform:mouse",
        "//redux/engines/platform:speaker",
        "//redux/modules/math:vector",
    ],
)
