
package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])  # Apache 2.0

cc_library(
    name = "serialize",
    hdrs = [
        "buffer_serializer.h",
        "serialize.h",
        "serialize_traits.h",
        "variant_serializer.h",
    ],
    deps = [
        "//lullaby/util:clock",
        "//lullaby/util:common_types",
        "//lullaby/util:hash",
        "//lullaby/util:logging",
        "//lullaby/util:string_view",
        "//lullaby/util:variant",
        "@mathfu//:mathfu",
    ],
)
