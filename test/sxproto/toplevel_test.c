#include <assert.h>
#include <string.h>

#include <fildesh/sxproto.h>

static const char array_test_content[] = "\
(())\n\
(() (w 5) (x 6))\n\
()\n\
(() (y 7) (z 8))\n\
";

static
  void
array_test()
{
  DECLARE_STRLIT_FildeshX(in, array_test_content);
  FildeshO* err_out = open_FildeshOF("/dev/stderr");
  FildeshSxpb* const sxpb = slurp_sxpb_close_FildeshX(in, NULL, err_out);
  FildeshSxpbIT it;
  FildeshSxpbIT val_it;

  assert(sxpb);

  it = first_at_FildeshSxpb(sxpb, top_of_FildeshSxpb(sxpb));
  val_it = lookup_subfield_at_FildeshSxpb(sxpb, it, "w");
  assert(5 == unsigned_value_at_FildeshSxpb(sxpb, val_it));
  val_it = lookup_subfield_at_FildeshSxpb(sxpb, it, "x");
  assert(6 == unsigned_value_at_FildeshSxpb(sxpb, val_it));

  it = next_at_FildeshSxpb(sxpb, it);
  /* Empty message.*/
  assert(nullish_FildeshSxpbIT(first_at_FildeshSxpb(sxpb, it)));

  it = next_at_FildeshSxpb(sxpb, it);
  val_it = lookup_subfield_at_FildeshSxpb(sxpb, it, "y");
  assert(7 == unsigned_value_at_FildeshSxpb(sxpb, val_it));
  val_it = lookup_subfield_at_FildeshSxpb(sxpb, it, "z");
  assert(8 == unsigned_value_at_FildeshSxpb(sxpb, val_it));

  /* End of array.*/
  assert(nullish_FildeshSxpbIT(next_at_FildeshSxpb(sxpb, it)));

  close_FildeshSxpb(sxpb);
  close_FildeshO(err_out);
}

int main() {
  array_test();
  return 0;
}
