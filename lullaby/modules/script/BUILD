
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
        "function_binder.cc",
        "script_engine.cc",
    ],
    hdrs = [
        "function_binder.h",
        "script_engine.h",
    ],
    defines = select({
        ":lullaby_script_lua": ["LULLABY_SCRIPT_LUA"],
        ":lullaby_script_js": ["LULLABY_SCRIPT_JS"],
        "//conditions:default": [],
    }),
    deps = [
        "//:fbs",
        "//lullaby/modules/file",
        "//lullaby/modules/function",
        "//lullaby/modules/lullscript",
        "//lullaby/util:built_in_functions",
        "//lullaby/util:hash",
        "//lullaby/util:registry",
        "//lullaby/util:logging",
    ] + select({
        ":lullaby_script_lua": [
            "//lullaby/modules/lua",
        ],
        ":lullaby_script_js": [
            "//lullaby/modules/javascript",
        ],
        "//conditions:default": [],
    }),
)
