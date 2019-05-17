
package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])  # Apache 2.0

cc_library(
    name = "fpl_mesh",
    srcs = [
        "fpl_mesh_system.cc",
    ],
    hdrs = [
        "fpl_mesh_system.h",
    ],
    deps = [
        "@fplbase//:fplbase_fbs",
        "//:fbs",
        "//lullaby/modules/ecs",
        "//lullaby/modules/file",
        "//lullaby/modules/render:mesh",
        "//lullaby/modules/render:vertex",
        "//lullaby/systems/render",
        "//lullaby/systems/rig",
        "//lullaby/util:entity",
        "//lullaby/util:filename",
        "//lullaby/util:flatbuffer_reader",
        "//lullaby/util:make_unique",
        "//lullaby/util:registry",
        "//lullaby/util:resource_manager",
        "//lullaby/util:span",
        "//lullaby/util:string_view",
    ],
)
