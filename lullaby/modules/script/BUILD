

package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])  # Apache 2.0

config_setting(
    name = "lullaby_script_lua",
    values = {
        "define": "lullaby_script_lua=1",
    },
    visibility = ["//visibility:public"],
)

config_setting(
    name = "lullaby_script_js",
    values = {
        "define": "lullaby_script_js=1",
    },
    visibility = ["//visibility:public"],
)

cc_library(
    name = "script",
    srcs = [
        "script_engine.cc",
    ],
    hdrs = [
        "function_binder.h",
        "function_registry.h",
        "script_engine.h",
    ],
    defines = select({
        ":lullaby_script_lua": ["LULLABY_SCRIPT_LUA"],
        ":lullaby_script_js": ["LULLABY_SCRIPT_JS"],
        "//conditions:default": [],
    }),
    deps = [
        "//lullaby/generated",
        "//lullaby/modules/file",
        "//lullaby/modules/function",
        "//lullaby/modules/script/lull",
        "//lullaby/util:built_in_functions",
        "//lullaby/util:common_types",
        "//lullaby/util:hash",
        "//lullaby/util:logging",
        "//lullaby/util:registry",
        "//lullaby/util:string_view",
        "//lullaby/util:typeid",
        "//lullaby/util:type_util",
        "//lullaby/util:variant",
    ] + select({
        ":lullaby_script_lua": [
            "//lullaby/modules/script/lua",
        ],
        ":lullaby_script_js": [
            "//lullaby/modules/script/javascript",
        ],
        "//conditions:default": [],
    }),
)
