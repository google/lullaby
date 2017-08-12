

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
        "lull/functions/compare.cc",
        "lull/functions/cond.cc",
        "lull/functions/do.cc",
        "lull/functions/map.cc",
        "lull/functions/math.cc",
        "lull/functions/mathfu.cc",
        "lull/functions/stringify.cc",
        "lull/lull_script_engine.cc",
        "lull/script_arg_list.cc",
        "lull/script_ast_builder.cc",
        "lull/script_compiler.cc",
        "lull/script_env.cc",
        "lull/script_frame.cc",
        "lull/script_parser.cc",
        "lull/script_scoped_symbol_table.cc",
        "lull/script_value.cc",
        "script_engine.cc",
    ],
    hdrs = [
        "call_native_function.h",
        "function_binder.h",
        "function_registry.h",
        "lull/functions/functions.h",
        "lull/lull_script_engine.h",
        "lull/script_arg_list.h",
        "lull/script_ast_builder.h",
        "lull/script_compiler.h",
        "lull/script_env.h",
        "lull/script_frame.h",
        "lull/script_parser.h",
        "lull/script_scoped_symbol_table.h",
        "lull/script_types.h",
        "lull/script_value.h",
        "script_engine.h",
    ],
    defines = select({
        ":lullaby_script_lua": ["LULLABY_SCRIPT_LUA"],
        ":lullaby_script_js": ["LULLABY_SCRIPT_JS"],
        "//conditions:default": [],
    }),
    deps = [
        "//lullaby/modules/file",
        "//lullaby/modules/serialize",
        "//lullaby/util:built_in_functions",
        "//lullaby/util:common_types",
        "//lullaby/util:hash",
        "//lullaby/util:logging",
        "//lullaby/util:optional",
        "//lullaby/util:registry",
        "//lullaby/util:span",
        "//lullaby/util:string_view",
        "//lullaby/util:type_util",
        "//lullaby/util:typeid",
        "//lullaby/util:variant",
        "//lullaby/generated",
        "@mathfu//:mathfu",
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

