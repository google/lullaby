"""Creates a bazel repository from a path stored in an environment variable."""

def _env_repository(ctx):
    src_dir = ctx.os.environ[ctx.attr.env]
    target_dir = ctx.attr.env
    ctx.symlink(src_dir, target_dir)

    if ctx.attr.build_file:
        build_file_contents = ctx.read(ctx.attr.build_file)
        ctx.file("BUILD", build_file_contents)

    return None

env_repository = repository_rule(
    implementation = _env_repository,
    local = True,
    attrs = {
        "env": attr.string(mandatory = True),
        "build_file": attr.label(),
    },
)
