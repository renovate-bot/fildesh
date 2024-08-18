#ifndef FILDESH_COMMAND_ALIAS_H_
#define FILDESH_COMMAND_ALIAS_H_
#include <string.h>

#include <fildesh/fildesh.h>

  void
fildesh_command_fix_known_flags(
    DECLARE_FildeshAT(char*, argv),
    FildeshKV* alias_map,
    FildeshAlloc* alloc);


static inline bool fildesh_eqstrlit(const char* a, const char* b) {
  return 0 == strcmp(a, b ? b : "");
}

static inline void ensure_strmap(FildeshKV* map, char* k, char* v) {
  const FildeshKV_id id = ensure_FildeshKV(map, k, strlen(k)+1);
  assign_at_FildeshKV(map, id, v, strlen(v)+1);
}

static inline char* lookup_strmap(FildeshKV* map, const char* k) {
  return (char*) lookup_value_FildeshKV(map, k, strlen(k)+1);
}

#endif
