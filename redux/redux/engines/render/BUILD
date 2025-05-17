# Render engine.

licenses(["notice"])

package(
    
    default_visibility = ["//redux:visibility"],
)

cc_library(
    name = "render",
    hdrs = [
        "indirect_light.h",
        "light.h",
        "mesh.h",
        "mesh_factory.h",
        "render_engine.h",
        "render_layer.h",
        "render_layer_options.h",
        "render_scene.h",
        "render_target.h",
        "render_target_factory.h",
        "renderable.h",
        "shader.h",
        "shader_factory.h",
        "texture.h",
        "texture_factory.h",
    ],
    deps = [
        "@absl//absl/types:span",
        "//redux/modules/base:bits",
        "//redux/modules/base:hash",
        "//redux/modules/base:registry",
        "//redux/modules/base:resource_manager",
        "//redux/modules/base:typeid",
        "//redux/modules/graphics:color",
        "//redux/modules/graphics:graphics_enums_fbs",
        "//redux/modules/graphics:image_data",
        "//redux/modules/graphics:mesh_data",
        "//redux/modules/graphics:texture_usage",
        "//redux/modules/math:bounds",
        "//redux/modules/math:matrix",
        "//redux/modules/math:vector",
        "//redux/modules/var",
    ],
)
