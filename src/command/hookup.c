#include "hookup.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "include/fildesh/fildesh_compat_errno.h"
#include "include/fildesh/fildesh_compat_fd.h"
#include "include/fildesh/fildesh_compat_file.h"
#include "src/syntax/symval.h"

static FildeshCommandHookup null_FildeshCommandHookup() {
  FildeshCommandHookup oo;
  const FildeshKV empty_map = DEFAULT_FildeshKV;
  oo.map = empty_map;
  oo.add_map = empty_map;
  oo.temporary_directory = NULL;
  oo.tmpfile_count = 0;
  oo.stdin_fd = 0;
  oo.stdout_fd = 1;
  init_FildeshAT(oo.stdargs);
  oo.stderr_fd_opened = false;
  init_FildeshAT(oo.cleanup_callbacks);
  return oo;
}

  FildeshCommandHookup*
new_FildeshCommandHookup(FildeshAlloc* alloc)
{
  FildeshCommandHookup* cmd_hookup = fildesh_allocate(
      FildeshCommandHookup, 1, alloc);
  *cmd_hookup = null_FildeshCommandHookup();
  cmd_hookup->map.alloc = alloc;
  cmd_hookup->add_map.alloc = alloc;
  return cmd_hookup;
}

  void
defer_FildeshCommandHookup(
    FildeshCommandHookup* cmd_hookup,
    void (*f) (void*),
    void* x)
{
  grow_FildeshAT(cmd_hookup->cleanup_callbacks, 1);
  last_FildeshAT(cmd_hookup->cleanup_callbacks).f = f;
  last_FildeshAT(cmd_hookup->cleanup_callbacks).x = x;
}

static void call_cleanup_callbacks(FildeshCommandHookup* cmd_hookup) {
  const size_t n = count_of_FildeshAT(cmd_hookup->cleanup_callbacks);
  size_t i;
  for (i = 0; i < n; ++i) {
    /* Do in reverse because it's a stack.*/
    (*cmd_hookup->cleanup_callbacks)[n-i-1].f(
        (*cmd_hookup->cleanup_callbacks)[n-i-1].x);
  }
  close_FildeshAT(cmd_hookup->cleanup_callbacks);
}

static void lose_SymVal(SymVal* v) {
  v->kind = NSymValKinds;
}

static void remove_temporary_directory(void* temporary_directory) {
  if (0 != fildesh_compat_file_rmdir((char*)temporary_directory)) {
    fildesh_compat_errno_trace();
    fildesh_log_warningf(
        "Temp directory not removed: %s",
        (char*)temporary_directory);
  }
  fildesh_log_trace("freed temporary_directory");
}

  int
close_FildeshCommandHookup(FildeshCommandHookup* cmd_hookup)
{
  int istat = 0;
  FildeshKV_id id;
  FildeshKV* map = &cmd_hookup->map;
  call_cleanup_callbacks(cmd_hookup);
  for (id = first_FildeshKV(map);
       !fildesh_nullid(id);
       id = next_at_FildeshKV(map, id))
  {
    SymVal* x = (SymVal*) value_at_FildeshKV(map, id);
    if (x->kind == ODescVal && istat == 0) {
      fildesh_log_errorf("Dangling output stream! Symbol: %s",
                         (char*) key_at_FildeshKV(map, id));
      istat = -1;
    }
    else if (x->kind == TmpFileVal) {
      fildesh_compat_file_rm(x->as.iofilename);
    }
    lose_SymVal(x);
  }
  close_FildeshKV(&cmd_hookup->map);
  close_FildeshKV(&cmd_hookup->add_map);
  if (cmd_hookup->temporary_directory) {
    remove_temporary_directory(cmd_hookup->temporary_directory);
    free(cmd_hookup->temporary_directory);
  }
  /* Close stdio if they are still valid and have been changed.*/
  if (cmd_hookup->stdin_fd >= 0 && cmd_hookup->stdin_fd != 0) {
    fildesh_compat_fd_close(cmd_hookup->stdin_fd);
  }
  if (cmd_hookup->stdout_fd >= 0 && cmd_hookup->stdout_fd != 1) {
    fildesh_compat_fd_close(cmd_hookup->stdout_fd);
  }
  close_FildeshAT(cmd_hookup->stdargs);
  if (cmd_hookup->stderr_fd_opened) {
    fildesh_compat_fd_close(2);
  }
  *cmd_hookup = null_FildeshCommandHookup();
  return istat;
}

  char*
tmpfile_FildeshCommandHookup(
    FildeshCommandHookup* cmd_hookup,
    const char* extension,
    FildeshAlloc* alloc)
{
  char buf[2048];
  assert(extension);
  if (!cmd_hookup->temporary_directory) {
    cmd_hookup->temporary_directory = fildesh_compat_file_mktmpdir("fildesh");
    fildesh_compat_errno_clear();
    if (!cmd_hookup->temporary_directory) {
      fildesh_log_error("Unable to create temp directory.");
      return NULL;
    }
  }

  sprintf(buf, "%s/%u%s", cmd_hookup->temporary_directory,
          cmd_hookup->tmpfile_count, extension);
  cmd_hookup->tmpfile_count += 1;
  return strdup_FildeshAlloc(alloc, buf);
}
