FBX_BUILD_FILE_CONTENTS = """
package(
    default_visibility = ["//visibility:public"],
)

cc_library(
    name = "libfbx",
    srcs = [
        "FBX_SDK_ROOT/lib/gcc4/x64/release/libfbxsdk.a"
    ],
    hdrs = [
        "FBX_SDK_ROOT/include/fbxsdk.h"
    ] + glob([
        "FBX_SDK_ROOT/include/fbxsdk/**/*.h"
    ]),
    includes = [
        "FBX_SDK_ROOT/include",
        "."
    ],
    linkopts = [
        "-ldl",
        "-pthread",
    ]
)
"""
