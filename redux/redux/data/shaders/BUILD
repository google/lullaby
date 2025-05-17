# Predefined shaders used for rendering.

load(
    "//redux/tools:build_shader.bzl",
    "build_shader",
)

licenses(["notice"])

package(
    
    default_visibility = ["//redux:visibility"],
)

build_shader(
    name = "text",
    src = "text.jsonnet",
    out = "text.rxshader",
    include_deps = glob([
        "glsl/*",
    ]),
)

build_shader(
    name = "unlit",
    src = "unlit.jsonnet",
    out = "unlit.rxshader",
    include_deps = glob([
        "glsl/*",
    ]),
)

build_shader(
    name = "metallic_roughness",
    src = "metallic_roughness.jsonnet",
    out = "metallic_roughness.rxshader",
    include_deps = glob([
        "glsl/*",
    ]),
)

build_shader(
    name = "phong",
    src = "phong.jsonnet",
    out = "phong.rxshader",
    include_deps = glob([
        "glsl/*",
    ]),
)
