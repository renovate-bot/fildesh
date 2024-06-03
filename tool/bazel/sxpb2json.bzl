
def _sxpb2json_impl(ctx):
  """Translate .sxpb file to .json file."""
  out_file = ctx.outputs.out
  if not out_file:
    out_file = ctx.actions.declare_file(
        ctx.label.name.removesuffix("_json") + ".json")

  args = ctx.actions.args()
  args.add_joined(["stdin=open_readonly", ctx.file.src], join_with = ":")
  args.add_joined(["stdout=open_writeonly", out_file], join_with = ":")
  args.add("--")
  args.add(ctx.executable._sxpb2json)
  ctx.actions.run(
      executable = ctx.executable._fildespawn,
      arguments = [args],
      inputs = [ctx.file.src],
      outputs = [out_file],
      tools = [ctx.executable._sxpb2json],
  )
  return DefaultInfo(
      files = depset([out_file]),
      runfiles = ctx.runfiles(files = [out_file]),
  )


sxpb2json = rule(
    implementation = _sxpb2json_impl,
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
        "_sxpb2json": attr.label(
            default = Label("//tool:sxpb2json"),
            allow_single_file = True,
            executable = True,
            cfg = "exec",
        ),
    },
)
