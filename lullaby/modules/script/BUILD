
package(
    default_visibility = ["//visibility:public"],
    features = ["-use_header_modules"],  # Incompatible with -fno-rtti.
)

licenses(["notice"])  # Apache 2.0

config_setting(
    name = "lullaby_disable_function_binder",
    values = {
        "define": "lullaby_disable_function_binder=1",
    },
    visibility = ["//visibility:public"],
)

cc_library(
    name = "script",
    srcs = [
        "function_binder.cc",
        "script_engine.cc",
    ],
    hdrs = [
        "function_binder.h",
        "script_engine.h",
    ],
    defines = select({
        ":lullaby_disable_function_binder": ["LULLABY_DISABLE_FUNCTION_BINDER=1"],
        "//conditions:default": ["LULLABY_DISABLE_FUNCTION_BINDER=0"],
    }),
    deps = [
        "//:fbs",
        "//lullaby/modules/dispatcher",
        "//lullaby/modules/file",
        "//lullaby/modules/function",
        "//lullaby/util:built_in_functions",
        "//lullaby/util:clock",
        "//lullaby/util:entity",
        "//lullaby/util:hash",
        "//lullaby/util:logging",
        "//lullaby/util:registry",
    ],
)

cc_library(
    name = "script_binder",
    srcs = [
        "script_engine_binder.cc",
    ],
    hdrs = [
        "script_engine_binder.h",
    ],
    deps = [
        ":script",
        "//lullaby/util:built_in_functions",
        "//lullaby/util:logging",
        "//lullaby/util:registry",
        "//lullaby/util:typeid",
    ],
)

cc_library(
    name = "script_jni",
    srcs = select({
        "//:target_os_android": [
            "function_binder_jni.cc",
            "script_engine_jni.cc",
        ],
        "//conditions:default": [],
    }),
    deps = [
        ":script",
        ":script_binder",
        "//lullaby/modules/function",
        "//lullaby/modules/jni:jni_context",
        "//lullaby/modules/jni:jni_convert",
        "//lullaby/modules/jni:jni_util",
        "//lullaby/modules/jni:registry_jni",
        "//lullaby/util:hash",
    ],
)
