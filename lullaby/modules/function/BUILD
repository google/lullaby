
package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])  # Apache 2.0

cc_library(
    name = "function",
    hdrs = [
        "call_native_function.h",
        "function_call.h",
        "variant_converter.h",
    ],
    deps = [
        "//lullaby/modules/dispatcher",
        "//lullaby/modules/serialize",
        "//lullaby/util:fixed_string",
        "//lullaby/util:hash",
        "//lullaby/util:optional",
        "//lullaby/util:string_view",
        "//lullaby/util:type_name_generator",
        "//lullaby/util:type_util",
        "//lullaby/util:typeid",
        "//lullaby/util:variant",
    ],
)
