
package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])  # Apache 2.0

cc_library(
    name = "render",
    srcs = [
        "image_util.cc",
        "mesh_data.cc",
        "mesh_util.cc",
        "nine_patch.cc",
        "render_view.cc",
        "vertex.cc",
        "vertex_format.cc",
    ],
    hdrs = [
        "image_util.h",
        "mesh_data.h",
        "mesh_util.h",
        "nine_patch.h",
        "render_view.h",
        "triangle_mesh.h",
        "vertex.h",
        "vertex_format.h",
    ],
    deps = [
        "//lullaby/modules/input",
        "//lullaby/util:color",
        "//lullaby/util:data_container",
        "//lullaby/util:logging",
        "//lullaby/util:math",
        "//lullaby/util:registry",
        "//lullaby/util:span",
        "@mathfu//:mathfu",
    ],
)
