"""Custom rules for OSS bazel project."""

def _local_repository(ctx):
    ctx.symlink(ctx.os.environ[ctx.attr.env], ctx.attr.env)
    ctx.file("BUILD", ctx.attr.build_file)
    return None

local_repository_env = repository_rule(
    implementation = _local_repository,
    local = True,
    attrs = {
        "env": attr.string(mandatory = True),
        "build_file": attr.string(mandatory = True),
    },
)
