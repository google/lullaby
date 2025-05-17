# Harfbuzz based implementation for glyph sequencing.

licenses(["notice"])

package(
    
    default_visibility = ["//redux:visibility"],
)

cc_library(
    name = "sequencer",
    srcs = [
        "sequencer.cc",
    ],
    deps = [
        "@absl//absl/log",
        "@absl//absl/log:check",
        "@harfbuzz//:harfbuzz",
        "//redux/engines/text",
    ],
)
