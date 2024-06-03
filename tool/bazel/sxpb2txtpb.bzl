
def _sxpb2txtpb_impl(ctx):
  """Translate .sxpb file to .txtpb file."""
  out_file = ctx.outputs.out
  if not out_file:
    out_file = ctx.actions.declare_file(
        ctx.label.name.removesuffix("_txtpb") + ".txtpb")

  args = ctx.actions.args()
  args.add_joined(["stdin=open_readonly", ctx.file.src], join_with = ":")
  args.add_joined(["stdout=open_writeonly", out_file], join_with = ":")
  args.add("--")
  args.add(ctx.executable._sxpb2txtpb)
  ctx.actions.run(
      executable = ctx.executable._fildespawn,
      arguments = [args],
      inputs = [ctx.file.src],
      outputs = [out_file],
      tools = [ctx.executable._sxpb2txtpb],
  )
  return DefaultInfo(
      files = depset([out_file]),
      runfiles = ctx.runfiles(files = [out_file]),
  )


sxpb2txtpb = rule(
    implementation = _sxpb2txtpb_impl,
    attrs = {
        "src": attr.label(
            mandatory = True,
            allow_single_file = True,
            doc = "The file to decode.",
        ),
        "out": attr.output(
            mandatory = False,
            doc = "The file to encode.",
        ),
        "_fildespawn": attr.label(
            default = Label("//tool:fildespawn"),
            allow_single_file = True,
            executable = True,
            cfg = "exec",
        ),
        "_sxpb2txtpb": attr.label(
            default = Label("//tool:sxpb2txtpb"),
            allow_single_file = True,
            executable = True,
            cfg = "exec",
        ),
    },
)
