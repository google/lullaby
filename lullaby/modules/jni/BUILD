
package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])  # Apache 2.0

JNI_IF_NOT_ANDROID = {
    "//:target_os_android": [],
    "//conditions:default": [
    ],
}

cc_library(
    name = "android_context_jni",
    srcs = select({
        "//:target_os_android": [
            "android_context_jni.cc",
        ],
        "//conditions:default": [],
    }),
    deps = [
        ":jni_util",
        ":registry_jni",
        "//lullaby/util:android_context",
    ],
)

# Convenience target that includes all the libraries necessary for:
#   lullaby/java/com/google/lullaby:base
cc_library(
    name = "base_jni",
    deps = [
        ":registry_jni",
        "//lullaby/modules/script:script_jni",
    ],
)

cc_library(
    name = "jni_context",
    srcs = select({
        "//:target_os_android": [
            "jni_context.cc",
        ],
        "//conditions:default": [],
    }),
    hdrs = [
        "jni_context.h",
    ],
    deps = [
        ":jni_signature",
        ":scoped_java_exception_guard",
        "//lullaby/util:logging",
        "//lullaby/util:string_view",
        "//lullaby/util:typeid",
    ] + select(JNI_IF_NOT_ANDROID),
)

cc_library(
    name = "jni_convert",
    srcs = select({
        "//:target_os_android": [
            "jni_convert.cc",
        ],
        "//conditions:default": [],
    }),
    hdrs = [
        "jni_convert.h",
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":jni_context",
        ":jni_signature",
        "//lullaby/modules/dispatcher",
        "//lullaby/util:entity",
        "//lullaby/util:variant",
    ] + select(JNI_IF_NOT_ANDROID),
)

cc_library(
    name = "jni_signature",
    hdrs = [
        "jni_signature.h",
    ],
    deps = [
        "//lullaby/util:fixed_string",
        "//lullaby/util:hash",
    ] + select(JNI_IF_NOT_ANDROID),
)

cc_library(
    name = "jni_util",
    hdrs = [
        "jni_util.h",
    ],
    deps = select(JNI_IF_NOT_ANDROID),
)

cc_library(
    name = "registry_jni",
    srcs = select({
        "//:target_os_android": [
            "registry_jni.cc",
        ],
        "//conditions:default": [],
    }),
    hdrs = [
        "registry_jni.h",
    ],
    deps = [
        ":jni_context",
        ":jni_util",
        "//lullaby/util:registry",
    ],
)

cc_library(
    name = "scoped_java_exception_guard",
    hdrs = [
        "scoped_java_exception_guard.h",
    ],
    deps = [
        "//lullaby/util:logging",
    ] + select(JNI_IF_NOT_ANDROID),
)

cc_library(
    name = "scoped_java_global_ref",
    hdrs = [
        "scoped_java_global_ref.h",
    ],
    deps = [
        ":jni_context",
        "//lullaby/util:logging",
        "//lullaby/util:registry",
    ] + select(JNI_IF_NOT_ANDROID),
)

cc_library(
    name = "scoped_java_local_ref",
    hdrs = [
        "scoped_java_local_ref.h",
    ],
    deps = select(JNI_IF_NOT_ANDROID),
)
