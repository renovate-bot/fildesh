#include "src/command/alias.h"

#include <assert.h>

static void fix_flags_test() {
  FildeshAlloc* alloc = open_FildeshAlloc();
  FildeshKV alias_map[1] = {DEFAULT_FildeshKV};
  DECLARE_DEFAULT_FildeshAT(char*, argv);
  char* const cat_str = (char*)"cat";
  char* const file_str = (char*)"in.txt";

  push_FildeshAT(argv, cat_str);
  fildesh_command_fix_known_flags(argv, alias_map, alloc);
  assert(fildesh_eqstrlit("splice", (*argv)[0]));
  clear_FildeshAT(argv);

  push_FildeshAT(argv, cat_str);
  push_FildeshAT(argv, NULL);
  fildesh_command_fix_known_flags(argv, alias_map, alloc);
  assert(fildesh_eqstrlit("splice", (*argv)[0]));
  clear_FildeshAT(argv);

  push_FildeshAT(argv, cat_str);
  push_FildeshAT(argv, file_str);
  fildesh_command_fix_known_flags(argv, alias_map, alloc);
  assert(fildesh_eqstrlit("splice", (*argv)[0]));
  clear_FildeshAT(argv);

  push_FildeshAT(argv, cat_str);
  push_FildeshAT(argv, (char*)"-");
  push_FildeshAT(argv, file_str);
  fildesh_command_fix_known_flags(argv, alias_map, alloc);
  assert(fildesh_eqstrlit("splice", (*argv)[0]));
  clear_FildeshAT(argv);

  push_FildeshAT(argv, cat_str);
  push_FildeshAT(argv, (char*)"--");
  push_FildeshAT(argv, file_str);
  fildesh_command_fix_known_flags(argv, alias_map, alloc);
  assert(fildesh_eqstrlit("splice", (*argv)[0]));
  clear_FildeshAT(argv);

  push_FildeshAT(argv, cat_str);
  push_FildeshAT(argv, (char*)"-A");
  push_FildeshAT(argv, file_str);
  fildesh_command_fix_known_flags(argv, alias_map, alloc);
  assert(fildesh_eqstrlit("cat", (*argv)[0]));
  clear_FildeshAT(argv);

  push_FildeshAT(argv, (char*)"sed");
  push_FildeshAT(argv, file_str);
  push_FildeshAT(argv, (char*)"--line-buffered");
  push_FildeshAT(argv, file_str);
  fildesh_command_fix_known_flags(argv, alias_map, alloc);
  assert(fildesh_eqstrlit("sed", (*argv)[0]));
  assert(fildesh_eqstrlit("in.txt", (*argv)[1]));
  assert(!fildesh_eqstrlit("--line-buffered", (*argv)[2]));
  clear_FildeshAT(argv);

  close_FildeshAT(argv);
  close_FildeshKV(alias_map);
  close_FildeshAlloc(alloc);
}

int main() {
  fix_flags_test();
  return 0;
}
