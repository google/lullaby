# Bullet physics backend for PhysicsEngine.

licenses(["notice"])

package(
    default_visibility = ["//redux:visibility"],
)

cc_library(
    name = "bullet",
    srcs = [
        "bullet_collision_shape.cc",
        "bullet_physics_engine.cc",
        "bullet_rigid_body.cc",
        "bullet_trigger_volume.cc",
        "thunks.cc",
    ],
    hdrs = [
        "bullet_collision_shape.h",
        "bullet_physics_engine.h",
        "bullet_rigid_body.h",
        "bullet_trigger_volume.h",
        "bullet_utils.h",
    ],
    deps = [
        "@absl//absl/base",
        "@bullet//:BulletCollision",
        "@bullet//:BulletDynamics",
        "@bullet//:LinearMath",
        "//redux/engines/physics",
        "//redux/engines/physics/thunks",
        "//redux/modules/base:bits",
        "//redux/modules/base:choreographer",
        "//redux/modules/base:static_registry",
        "//redux/modules/ecs:entity",
        "//redux/modules/math:bounds",
        "//redux/modules/math:quaternion",
        "//redux/modules/math:transform",
        "//redux/modules/math:vector",
    ],
)
