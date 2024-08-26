#ifndef FILDESH_COMMAND_HOOKUP_H_
#define FILDESH_COMMAND_HOOKUP_H_
#include <fildesh/fildesh.h>

typedef struct FildeshCommandHookup FildeshCommandHookup;

/* See the setup_commands() phase.*/
struct FildeshCommandHookup {
  char* temporary_directory;
  unsigned tmpfile_count;
  FildeshKV map;
  FildeshKV add_map; /* Temporarily hold new symbols for the current line.*/
  Fildesh_fd stdin_fd;
  Fildesh_fd stdout_fd;
  DECLARE_FildeshAT(const char*, stdargs);
  /* Stderr stays at fd 2 but should be closed explicitly if we dup2 over it.*/
  bool stderr_fd_opened;
  DECLARE_FildeshAT(struct { void (*f) (void*); void* x; }, cleanup_callbacks);
};

FildeshCommandHookup* new_FildeshCommandHookup(FildeshAlloc* alloc);
void
defer_FildeshCommandHookup(
    FildeshCommandHookup* cmd_hookup,
    void (*f) (void*),
    void* x);
int close_FildeshCommandHookup(FildeshCommandHookup* cmd_hookup);

char*
tmpfile_FildeshCommandHookup(
    FildeshCommandHookup* cmd_hookup,
    const char* extension,
    FildeshAlloc* alloc);

#endif
