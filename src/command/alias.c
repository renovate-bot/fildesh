#include "alias.h"

#include <assert.h>

  void
fildesh_command_fix_known_flags(
    DECLARE_FildeshAT(char*, argv),
    FildeshKV* alias_map,
    FildeshAlloc* alloc)
{
  const unsigned argv_count = (
      count_of_FildeshAT(argv) - (last_FildeshAT(argv) ? 0 : 1));
  char* replacement;

  assert(argv_count > 0);
  assert((*argv)[0]);

  replacement = lookup_strmap(alias_map, (*argv)[0]);
  if (replacement) {
    (*argv)[0] = replacement;
  }

  if (fildesh_eqstrlit("cat", (*argv)[0])) {
    if (argv_count == 1 ||
        (*argv)[1][0] != '-' ||
        (*argv)[1][1] == '\0' ||
        ((*argv)[1][1] == '-' && (*argv)[1][2] == '\0')) {
      (*argv)[0] = strdup_FildeshAlloc(alloc, "splice");
    }
  }
  else if (fildesh_eqstrlit("sed", (*argv)[0])) {
    unsigned i;
    for (i = 1; i < argv_count; ++i) {
      const char* arg = (*argv)[i];
      if (fildesh_eqstrlit("--line-buffered", arg)) {
#ifdef __APPLE__
        const char line_buffering_flag[] = "-l";
#else
        const char line_buffering_flag[] = "-u";
#endif
        (*argv)[i] = strdup_FildeshAlloc(alloc, line_buffering_flag);
      }
    }
  }
  else if (fildesh_eqstrlit("tr", (*argv)[0])) {
    if (argv_count == 3 &&
        (*argv)[1][0] != '\0' && (*argv)[1][1] == '\0' &&
        (*argv)[2][0] != '\0' && (*argv)[2][1] == '\0') {
      (*argv)[0] = strdup_FildeshAlloc(alloc, "replace_string");
    }
  }
  else if (fildesh_eqstrlit("xargz", (*argv)[0])) {
    unsigned i = 1;
    if (fildesh_eqstrlit("--", (*argv)[i])) {
      i += 1;
    }
    if ((*argv)[i]) {
      replacement = lookup_strmap(alias_map, (*argv)[i]);
      if (replacement) {
        (*argv)[i] = replacement;
      }
    }
  }
}
