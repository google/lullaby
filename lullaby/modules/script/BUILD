
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

config_setting(
    name = "lullaby_disable_lullscript",
    values = {
        "define": "lullaby_disable_lullscript=1",
    },
    visibility = ["//visibility:public"],
)

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
        ":lullaby_script_lua": ["LULLABY_SCRIPT_LUA=1"],
        "//conditions:default": ["LULLABY_SCRIPT_LUA=0"],
    }) + select({
        ":lullaby_script_js": ["LULLABY_SCRIPT_JS=1"],
        "//conditions:default": ["LULLABY_SCRIPT_JS=0"],
    }) + select({
        ":lullaby_disable_lullscript": ["LULLABY_SCRIPT_LULLSCRIPT=0"],
        "//conditions:default": ["LULLABY_SCRIPT_LULLSCRIPT=1"],
    }) + select({
        ":lullaby_disable_function_binder": ["LULLABY_DISABLE_FUNCTION_BINDER=1"],
        "//conditions:default": ["LULLABY_DISABLE_FUNCTION_BINDER=0"],
    }),
    deps = [
        "//:fbs",
        "//lullaby/modules/file",
        "//lullaby/modules/function",
        "//lullaby/util:built_in_functions",
        "//lullaby/util:hash",
        "//lullaby/util:registry",
        "//lullaby/util:logging",
    ] + select({
        ":lullaby_script_lua": [
            "//lullaby/modules/lua",
        ],
        "//conditions:default": [],
    }) + select({
        ":lullaby_script_js": [
            "//lullaby/modules/javascript",
        ],
        "//conditions:default": [],
    }) + select({
        ":lullaby_disable_lullscript": [
        ],
        "//conditions:default": [
            "//lullaby/modules/lullscript",
        ],
    }),
)
