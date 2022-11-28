# Platform abstraction (i.e. platform-specific event processing loop) library.

licenses(["notice"])

package(
    default_visibility = ["//redux:visibility"],
)

cc_library(
    name = "buffered_state",
    hdrs = ["buffered_state.h"],
)

cc_library(
    name = "device_manager",
    srcs = ["device_manager.cc"],
    hdrs = ["device_manager.h"],
    deps = [
        ":device_profiles",
        ":display",
        ":keyboard",
        ":mouse",
        ":speaker",
        "@absl//absl/synchronization",
        "@absl//absl/types:span",
        "//redux/modules/base:choreographer",
        "//redux/modules/base:logging",
        "//redux/modules/base:registry",
    ],
)

cc_library(
    name = "device_profiles",
    hdrs = ["device_profiles.h"],
    deps = [
        "@absl//absl/container:flat_hash_map",
        "//redux/modules/math:vector",
    ],
)

cc_library(
    name = "keycodes",
    srcs = ["keycodes.cc"],
    hdrs = ["keycodes.h"],
)

cc_library(
    name = "display",
    srcs = ["display.cc"],
    hdrs = ["display.h"],
    deps = [
        ":buffered_state",
        ":device_profiles",
        ":virtual_device",
        "@absl//absl/time",
        "//redux/modules/base:logging",
        "//redux/modules/math:vector",
    ],
)

cc_library(
    name = "keyboard",
    srcs = ["keyboard.cc"],
    hdrs = ["keyboard.h"],
    deps = [
        ":buffered_state",
        ":device_profiles",
        ":keycodes",
        ":virtual_device",
        "@absl//absl/time",
        "//redux/modules/base:logging",
        "//redux/modules/math:vector",
    ],
)

cc_library(
    name = "mainloop",
    srcs = [
        "mainloop.cc",
    ],
    hdrs = [
        "mainloop.h",
    ],
    deps = [
        ":device_manager",
        "@absl//absl/status",
        "//redux/modules/base:asset_loader",
        "//redux/modules/base:choreographer",
        "//redux/modules/base:registry",
        "//redux/modules/base:static_registry",
        "//redux/modules/math:vector",
    ],
)

cc_library(
    name = "mouse",
    srcs = ["mouse.cc"],
    hdrs = ["mouse.h"],
    deps = [
        ":buffered_state",
        ":device_profiles",
        ":virtual_device",
        "@absl//absl/time",
        "//redux/modules/base:logging",
        "//redux/modules/math:vector",
    ],
)

cc_library(
    name = "speaker",
    srcs = ["speaker.cc"],
    hdrs = ["speaker.h"],
    deps = [
        ":device_profiles",
        ":virtual_device",
        "@absl//absl/types:span",
    ],
)

cc_library(
    name = "virtual_device",
    srcs = ["virtual_device.cc"],
    hdrs = ["virtual_device.h"],
    deps = [
        "@absl//absl/time",
        "//redux/modules/math:vector",
    ],
)
