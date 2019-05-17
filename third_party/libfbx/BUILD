package(
    default_visibility = ["//visibility:public"],
)

cc_library(
    name = "libfbx",
    srcs = select({
        "@bazel_tools//src/conditions:darwin": [
            "FBX_SDK_ROOT/lib/clang/release/libfbxsdk.a",
        ],
        "//conditions:default": [
            "FBX_SDK_ROOT/lib/gcc4/release/libfbxsdk.a",
        ],
    }),
    hdrs = [
        "FBX_SDK_ROOT/include/fbxsdk.h",
    ] + glob([
        "FBX_SDK_ROOT/include/fbxsdk/**/*.h",
    ]),
    includes = [
        ".",
        "FBX_SDK_ROOT/include",
    ],
    linkopts = [
        "-ldl",
        "-pthread",
    ],
)
