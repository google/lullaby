# BUILD file for a console tool for LullScript.

licenses(["notice"])  # Apache 2.0

cc_binary(
    name = "lull_script_console",
    srcs = [
        "lull_script.cc",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//lullaby/modules/function",
        "//lullaby/modules/lullscript",
        "//third_party/readline",
    ],
)
