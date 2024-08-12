#include <assert.h>
#include <string.h>

/* #define FILDESH_LOG_TRACE_ON */
#include <fildesh/sxproto.h>

static
  const FildeshSxprotoField*
sxproto_schema()
{
  static FildeshSxprotoField grammar_manyof[] = {
    {"", 1 FILL_FildeshSxprotoField_STRING(1, INT_MAX)},
    {"char", 2 FILL_FildeshSxprotoField_STRING(1, INT_MAX)},
    {"charnot", 3 FILL_FildeshSxprotoField_STRING(0, INT_MAX)},
    {"lone", 4 FILL_RECURSIVE_FildeshSxprotoField_MANYOF},
    {"loneof", 5 FILL_RECURSIVE_FildeshSxprotoField_MANYOF},
    {"one", 6 FILL_RECURSIVE_FildeshSxprotoField_MANYOF},
    {"oneof", 7 FILL_RECURSIVE_FildeshSxprotoField_MANYOF},
    {"many", 8 FILL_RECURSIVE_FildeshSxprotoField_MANYOF},
    {"manyof", 9 FILL_RECURSIVE_FildeshSxprotoField_MANYOF},
    {"some", 10 FILL_RECURSIVE_FildeshSxprotoField_MANYOF},
    {"someof", 11 FILL_RECURSIVE_FildeshSxprotoField_MANYOF},
  };
  static FildeshSxprotoField rule_fields[] = {
    {"name", 1 FILL_FildeshSxprotoField_STRING(1, INT_MAX)},
    {"as", 2 FILL_FildeshSxprotoField_LONEOF(grammar_manyof)},
  };
  static FildeshSxprotoField schema[] = {
    {NULL, 1 FILL_FildeshSxprotoField_MESSAGES(rule_fields)},
  };
  lone_toplevel_initialization_FildeshSxprotoField(schema);
  return schema;
}

static
  FildeshSxpb*
open_grammar_sxpb()
{
#include "eg/sxpb_string/grammar.embed.h"
  FildeshX grammar_sxpb_in = FildeshX_of_bytestring(
      files_bytes[nfiles-1], files_nbytes[nfiles-1]);
  FildeshO* err_out = open_FildeshOF("/dev/stderr");
  FildeshSxpb* sxpb = slurp_sxpb_close_FildeshX(
      &grammar_sxpb_in,
      sxproto_schema(),
      err_out);
  assert(sxpb);
  close_FildeshO(err_out);
  return sxpb;
}

static
  void
recursive_validate(
    const FildeshSxpb* sxpb,
    FildeshSxpbIT m_it,
    const FildeshKV* map)
{
  FildeshSxpbIT it;
  const char* name = name_at_FildeshSxpb(sxpb, m_it);
  fildesh_log_trace(name);
  assert(name);
  if (name[0] == 'c') {return;}
  for (it = first_at_FildeshSxpb(sxpb, m_it);
       !nullish_FildeshSxpbIT(it);
       it = next_at_FildeshSxpb(sxpb, it))
  {
    name = name_at_FildeshSxpb(sxpb, it);
    if (name) {
      recursive_validate(sxpb, it, map);
    }
    else {
      FildeshKV_id map_it;
      name = str_value_at_FildeshSxpb(sxpb, it);
      map_it = lookup_FildeshKV(map, name, strlen(name)+1);
      assert(!fildesh_nullid(map_it));
    }
  }
}

static void validate_grammar_sxpb(FildeshSxpb* sxpb)
{
  FildeshKV map[1] = {DEFAULT_FildeshKV};
  FildeshSxpbIT it;
  for (it = first_at_FildeshSxpb(sxpb, top_of_FildeshSxpb(sxpb));
       !nullish_FildeshSxpbIT(it);
       it = next_at_FildeshSxpb(sxpb, it))
  {
    const char* name = NULL;
    if (lone_subfield_at_FildeshSxpb_to_str(&name, sxpb, it, "name")) {
      FildeshKV_id map_it = ensuref_FildeshKV(map, name, strlen(name)+1);
      assign_at_FildeshKV(map, map_it, &it, sizeof(it));
    }
  }
  for (it = first_at_FildeshSxpb(sxpb, top_of_FildeshSxpb(sxpb));
       !nullish_FildeshSxpbIT(it);
       it = next_at_FildeshSxpb(sxpb, it))
  {
    FildeshSxpbIT m_it = lookup_subfield_at_FildeshSxpb(sxpb, it, "as");
    assert(!nullish_FildeshSxpbIT(m_it));
    m_it = first_at_FildeshSxpb(sxpb, m_it);
    recursive_validate(sxpb, m_it, map);
  }
  close_FildeshKV(map);
}

int main() {
  FildeshSxpb* sxpb = open_grammar_sxpb();

  validate_grammar_sxpb(sxpb);

  close_FildeshSxpb(sxpb);
  return 0;
}
