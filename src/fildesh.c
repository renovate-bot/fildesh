/** \file fildesh.c
 *
 * This code is written by Alex Klinkhamer.
 * It uses the ISC license (see the LICENSE file in the top-level directory).
 **/
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fildesh/fildesh.h>

#include "include/fildesh_posix_thread.h"
#include "include/fildesh/fildesh_compat_errno.h"
#include "include/fildesh/fildesh_compat_fd.h"
#include "include/fildesh/fildesh_compat_file.h"
#include "include/fildesh/fildesh_compat_sh.h"
#include "include/fildesh/fildesh_compat_string.h"

#include "src/bin/version.h"
#include "src/builtin/fildesh_builtin.h"
#include "src/command/alias.h"
#include "src/command/hookup.h"
#include "src/syntax/defstr.h"
#include "src/syntax/line.h"
#include "src/syntax/opt.h"
#include "src/syntax/symval.h"

/* Defined in src/bin/main.c*/
void push_fildesh_exit_callback(void (*f) (void*), void* x);


enum CommandKind {
  RunCommand, HereDocCommand,
  StdinCommand, StdoutCommand, StderrCommand,
  DefCommand,
  BarrierCommand,
  NCommandKinds
};


typedef enum CommandKind CommandKind;
typedef struct Command Command;
typedef struct BuiltinCommandThreadArg BuiltinCommandThreadArg;


struct Command
{
  char* line;
  unsigned line_num;
  CommandKind kind;
  DECLARE_FildeshAT(char*, args);
  DECLARE_FildeshAT(char*, tmp_files);
  pthread_t thread;
  FildeshCompat_pid pid;
  int status;
  Fildesh_fd stdis; /**< Standard input stream.**/
  DECLARE_FildeshAT(int, is); /**< Input streams.**/
  Fildesh_fd stdos; /**< Standard output stream.**/
  DECLARE_FildeshAT(int, os); /**< Output streams.**/
  /* Exit status stream.**/
  Fildesh_fd status_fd;
  /** File descriptor to close upon exit.**/
  DECLARE_FildeshAT(int, exit_fds);
  /** If >= 0, this is a file descriptor that will
   * close when the program command is safe to run.
   **/
  Fildesh_fd exec_fd;
  /** Whether exec_fd actually has bytes, rather than just used for signaling.*/
  bool exec_fd_has_bytes;
  /** If != NULL, this is the contents of a file to execute.**/
  const char* exec_doc;

  /** Use these input streams to fill corresponding (null) arguments.**/
  DECLARE_FildeshAT(struct { int fd; bool scrap_newline; }, iargs);

  /** Use this if it's a HERE document.**/
  char* doc;

  /** Use this to allocate stuff.
   **/
  FildeshAlloc* alloc;
};

struct BuiltinCommandThreadArg {
  Command* command;  /* Cleanup but don't free.*/
  char** argv;  /* Free nested.*/
};

  static void
init_Command(Command* cmd, FildeshAlloc* alloc)
{
  cmd->kind = NCommandKinds;
  cmd->line_num = 0;
  init_FildeshAT(cmd->args);
  init_FildeshAT(cmd->tmp_files);
  cmd->pid = -1;
  cmd->stdis = -1;
  init_FildeshAT(cmd->is);
  cmd->stdos = -1;
  init_FildeshAT(cmd->os);
  cmd->status_fd = -1;
  init_FildeshAT(cmd->exit_fds);
  cmd->exec_fd = -1;
  cmd->exec_fd_has_bytes = false;
  cmd->exec_doc = NULL;
  init_FildeshAT(cmd->iargs);
  cmd->alloc = alloc;
}

  static void
close_Command (Command* cmd)
{
  unsigned i;
  /* Close fd being used for stdin.*/
  if (cmd->stdis >= 0) {
    fildesh_compat_fd_close(cmd->stdis);
    cmd->stdis = -1;
  }
  /* Close fd being used for stdout unless it's /dev/stderr.*/
  if (cmd->stdos >= 0 && cmd->stdos != 2) {
    fildesh_compat_fd_close(cmd->stdos);
    cmd->stdos = -1;
  }

  for (i = 0; i < count_of_FildeshAT(cmd->is); ++i) {
    fildesh_compat_fd_close((*cmd->is)[i]);
  }
  close_FildeshAT(cmd->is);

  for (i = 0; i < count_of_FildeshAT(cmd->os); ++i) {
    fildesh_compat_fd_close((*cmd->os)[i]);
  }
  close_FildeshAT(cmd->os);

  if (cmd->status_fd >= 0) {
    fildesh_compat_fd_close(cmd->status_fd);
    cmd->status_fd = -1;
  }
  close_FildeshAT(cmd->exit_fds);

  cmd->exec_fd = -1;
  cmd->exec_fd_has_bytes = false;
  cmd->exec_doc = NULL;

  close_FildeshAT(cmd->iargs);
}

static
  Fildesh_fd*
build_fds_to_inherit_Command(Command* cmd)
{
  size_t i, off;
  Fildesh_fd* fds = (Fildesh_fd*) malloc(
      sizeof(Fildesh_fd) *
      (count_of_FildeshAT(cmd->is) +
       count_of_FildeshAT(cmd->os) +
       1));

  off = 0;
  for (i = 0; i < count_of_FildeshAT(cmd->is); ++i) {
    fds[off++] = (*cmd->is)[i];
  }
  close_FildeshAT(cmd->is);
  for (i = 0; i < count_of_FildeshAT(cmd->os); ++i) {
    fds[off++] = (*cmd->os)[i];
  }
  close_FildeshAT(cmd->os);
  fds[off] = -1;
  return fds;
}

  static void
lose_Command (Command* cmd)
{
  unsigned i;
  close_Command (cmd);
  switch (cmd->kind) {
    case DefCommand:
    case RunCommand:
    case StdinCommand:
    case StdoutCommand:
    case StderrCommand:
      close_FildeshAT(cmd->args);
      break;
    case HereDocCommand:
      break;
    default:
      break;
  }

  for (i = 0; i < count_of_FildeshAT(cmd->tmp_files); ++i) {
    fildesh_compat_file_rm((*cmd->tmp_files)[i]);
  }
  close_FildeshAT(cmd->tmp_files);
  cmd->kind = NCommandKinds;
}

  static void
lose_Commands (void* arg)
{
  Command** cmds = (Command**) arg;
  unsigned i;
  for (i = 0; i < count_of_FildeshAT(cmds); ++i) {
    if ((*cmds)[i].kind == RunCommand && (*cmds)[i].pid > 0)
      fildesh_compat_sh_kill((*cmds)[i].pid);
    if ((*cmds)[i].kind != NCommandKinds)
      lose_Command (&(*cmds)[i]);
  }
  close_FildeshAT(cmds);
  free(cmds);
}

static void close_FildeshCommandHookup_generic(void* arg) {
  close_FildeshCommandHookup((FildeshCommandHookup*) arg);
}
static void close_FildeshAlloc_generic(void* arg) {
  close_FildeshAlloc((FildeshAlloc*) arg);
}
static void close_FildeshKV_generic(void* arg) {
  close_FildeshKV((FildeshKV*) arg);
}
static void close_FildeshO_generic(void* arg) {
  close_FildeshO((FildeshO*) arg);
}

static inline
  bool
eq_cstr(const char* a, const char* b)
{
  if (a == b)  return true;
  if (!a)  return false;
  if (!b)  return false;
  return (0 == strcmp (a, b));
}

static inline
  bool
pfxeq_cstr(const char* pfx, const char* s)
{
  if (!s)  return false;
  return (0 == strncmp(pfx, s, strlen (pfx)));
}

static char* lace_strdup(const char* s) {
  return fildesh_compat_string_duplicate(s);
}

static char* strdup_fd_Command(Command* cmd, Fildesh_fd fd)
{
  char buf[FILDESH_INT_BASE10_SIZE_MAX];
  fildesh_encode_int_base10(buf, fd);
  return strdup_FildeshAlloc(cmd->alloc, buf);
}

static char* lace_fd_strdup(Fildesh_fd fd) {
  char buf[FILDESH_INT_BASE10_SIZE_MAX];
  fildesh_encode_int_base10(buf, fd);
  return lace_strdup(buf);
}

static char* lace_fd_path_strdup(Fildesh_fd fd) {
  char buf[FILDESH_FD_PATH_SIZE_MAX];
  fildesh_encode_fd_path(buf, fd);
  return lace_strdup(buf);
}

  static unsigned
count_ws (const char* s)
{
  return strspn (s, fildesh_compat_string_blank_bytes);
}
  static unsigned
count_non_ws (const char* s)
{
  return strcspn (s, fildesh_compat_string_blank_bytes);
}

  static void
perror_Command(const Command* cmd, const char* msg, const char* msg2)
{
  if (msg && msg2) {
    fildesh_log_errorf("Problem on line %u. %s: %s", cmd->line_num, msg, msg2);
  } else if (msg) {
    fildesh_log_errorf("Problem on line %u. %s.", cmd->line_num, msg);
  } else {
    fildesh_log_errorf("Problem on line %u.", cmd->line_num);
  }
}

static
  int
parse_file(
    FildeshCommandHookup* cmd_hookup,
    Command** cmds,
    FildeshX* in,
    size_t* ret_line_count,
    const char* this_filename,
    FildeshKV* map,
    FildeshAlloc* scope_alloc,
    FildeshAlloc* global_alloc,
    FildeshO* tmp_out)
{
  int istat = 0;
  size_t text_nlines = *ret_line_count;
  while (istat == 0) {
    char* line;
    Command* cmd;
    const char* emsg = parse_fildesh_string_definition(
        in, &text_nlines, map, global_alloc, tmp_out);
    if (!emsg) {continue;}
    if (emsg[0]) {
      istat = -1;
      fildesh_log_errorf(
          "In %s, line %u: %s",
          this_filename, (unsigned)text_nlines+1, emsg);
      break;
    }
    line = fildesh_syntax_parse_line(in, &text_nlines, global_alloc, tmp_out);
    if (!line) {
      break;
    }
    cmd = grow1_FildeshAT(cmds);
    init_Command(cmd, scope_alloc);
    cmd->line = line;
    cmd->line_num = text_nlines;

    if (line[0] == '$' && line[1] == '(' &&
        line[2] == 'H' && line[3] != 'F')
    {
      SymVal* sym;
      SymValKind sym_kind;

      cmd->kind = HereDocCommand;
      cmd->doc = fildesh_syntax_parse_here_doc(
          in, line, &text_nlines, global_alloc, tmp_out);

      sym_kind = parse_fildesh_SymVal_arg(cmd->line, false);
      assert(sym_kind == HereDocVal);
      sym = declare_fildesh_SymVal(map, HereDocVal, cmd->line);
      if (!sym) {istat = -1; break;}
      sym->as.here_doc = cmd->doc;
    }
    else if (pfxeq_cstr ("$(<<", line))
    {
      char* filename = &line[4];
      FildeshX* src = NULL;

      filename = &filename[count_ws (filename)];
      filename[strcspn (filename, ")")] = '\0';

      src = open_sibling_FildeshXF(this_filename, filename);
      if (!src) {
        perror_Command(cmd, "Failed to include file", filename);
        istat = -1;
        break;
      }
      lose_Command(&last_FildeshAT(cmds));
      mpop_FildeshAT(cmds, 1);

      *ret_line_count = 0;
      istat = parse_file(
          cmd_hookup, cmds,
          src, ret_line_count, filename_FildeshXF(src),
          map, scope_alloc, global_alloc, tmp_out);
      if (istat == 0 && count_of_FildeshAT(cmds) > 0 &&
          last_FildeshAT(cmds).kind == BarrierCommand)
      {
        perror_Command(
            &last_FildeshAT(cmds),
            "No barrier allowed in included file", filename);
        istat = -1;
      }
      close_FildeshX(src);
    }
    else if (pfxeq_cstr ("$(> ", line))
    {
      char* begline;
      char* sym_name = line;
      char* concatenated_args = NULL;
      const char* emsg;

      cmd->kind = RunCommand;

      sym_name = &sym_name[count_non_ws(sym_name)];
      sym_name = &sym_name[count_ws(sym_name)];
      begline = strchr(sym_name, ')');
      if (!begline) {
        perror_Command(cmd, "Unclosed paren in variable def.", 0);
        istat = -1;
        break;
      }

      begline[0] = '\0';
      begline = &begline[1];

      push_FildeshAT(cmd->args, (char*)"splice");
      push_FildeshAT(cmd->args, (char*) "/");
      emsg = fildesh_syntax_sep_line(
          cmd->args, begline, map, scope_alloc, tmp_out);
      if (emsg) {fildesh_log_error(emsg); break;}

      concatenated_args = fildesh_syntax_maybe_concatenate_args(
          (unsigned) count_of_FildeshAT(cmd->args)-2,
          (const char* const*)(void*)&(*cmd->args)[2],
          global_alloc);
      if (concatenated_args) {
        SymVal* sym = declare_fildesh_SymVal(map, HereDocVal, sym_name);
        if (!sym) {istat = -1; break;}
        sym->as.here_doc = concatenated_args;

        cmd->kind = HereDocCommand;
        cmd->doc = concatenated_args;
        close_FildeshAT(cmd->args);
        continue;
      }
      push_FildeshAT(cmd->args, (char*)"/");

      {
        char* buf = fildesh_allocate(char, 6+strlen(sym_name), scope_alloc);
        sprintf(buf, "$(O %s)", sym_name);
        push_FildeshAT(cmd->args, buf);
      }

      cmd = grow1_FildeshAT(cmds);
      init_Command(cmd, scope_alloc);
      cmd->kind = DefCommand;
      cmd->line_num = text_nlines;
      push_FildeshAT(cmd->args, (char*)"elastic");
      {
        char* buf = fildesh_allocate(char, 6+strlen(sym_name), scope_alloc);
        sprintf(buf, "$(X %s)", sym_name);
        push_FildeshAT(cmd->args, buf);
      }
      cmd->line = sym_name;
    }
    else if (pfxeq_cstr("$(tmpfile ", line))
    {
      char* begline;
      char* sym_name = line;
      SymVal* sym;

      sym_name = &sym_name[count_non_ws(sym_name)];
      sym_name = &sym_name[count_ws(sym_name)];
      begline = strchr(sym_name, ')');
      if (!begline) {
        perror_Command(cmd, "Unclosed paren in tmpfile def.", 0);
        istat = -1;
        break;
      }
      begline[0] = '\0';
      sym = declare_fildesh_SymVal(map, TmpFileVal, sym_name);
      if (!sym) {istat = -1; break;}
      sym->as.iofilename = tmpfile_FildeshCommandHookup(
          cmd_hookup, ".txt", global_alloc);

      lose_Command(cmd);
      mpop_FildeshAT(cmds, 1);
    }
    else
    {
      const char* emsg;
      cmd->kind = RunCommand;
      emsg = fildesh_syntax_sep_line(
          cmd->args, cmd->line, map, scope_alloc, tmp_out);
      if (emsg) {
        perror_Command(cmd, emsg, 0);
        istat = -1;
        break;
      }
      else if (0 == strcmp((*cmd->args)[0], "$(barrier)")) {
        if (count_of_FildeshAT(cmd->args) != 1) {
          perror_Command(cmd, "Barrier does not accept args.", 0);
          istat = -1;
        }
        cmd->kind = BarrierCommand;
        close_FildeshAT(cmd->args);
        break;
      }
    }
  }
  *ret_line_count = text_nlines;
  return istat;
}

  static unsigned
add_ios_Command (Command* cmd, int in, int out)
{
  unsigned idx = UINT_MAX;
  if (in >= 0) {
    idx = count_of_FildeshAT(cmd->is);
    push_FildeshAT(cmd->is, in);
  }

  if (out >= 0) {
    idx = count_of_FildeshAT(cmd->os);
    push_FildeshAT(cmd->os, out);
  }
  return idx;
}

  static void
add_iarg_Command (Command* cmd, int in, bool scrap_newline)
{
  grow_FildeshAT(cmd->iargs, 1);
  last_FildeshAT(cmd->iargs).fd = in;
  last_FildeshAT(cmd->iargs).scrap_newline = scrap_newline;
}

  static char*
add_fd_arg_Command (Command* cmd, int fd)
{
  char buf[FILDESH_FD_PATH_SIZE_MAX];
  fildesh_encode_fd_path(buf, fd);
  return strdup_FildeshAlloc(cmd->alloc, buf);
}

  static char*
add_tmp_file_Command(Command* cmd,
                     FildeshCommandHookup* cmd_hookup,
                     const char* extension)
{
  push_FildeshAT(cmd->tmp_files, tmpfile_FildeshCommandHookup(
          cmd_hookup, extension, cmd->alloc));
  return (*cmd->tmp_files)[count_of_FildeshAT(cmd->tmp_files) - 1];
}

static
  char*
write_heredoc_tmpfile(Command* cmd, FildeshCommandHookup* cmd_hookup, const char* doc)
{
  FildeshO* out;
  char* filename = add_tmp_file_Command(cmd, cmd_hookup, ".txt");
  if (!filename) {return NULL;}
  /* Write the temp file now.*/
  out = open_FildeshOF(filename);
  if (!out) {return NULL;}
  put_bytestring_FildeshO(out, (const unsigned char*)doc, strlen(doc));
  close_FildeshO(out);
  return filename;
}

static
  Fildesh_fd
pipe_from_elastic(Command* elastic_cmd)
{
  Fildesh_fd fd[2];
  if (0 != fildesh_compat_fd_pipe(&fd[1], &fd[0])) {
    return -1;
  }
  add_ios_Command(elastic_cmd, -1, fd[1]);
  push_FildeshAT(elastic_cmd->args, add_fd_arg_Command(elastic_cmd, fd[1]));
  return fd[0];
}

#define FailBreak(cmd, msg, arg) { \
  perror_Command(cmd, msg, arg); \
  istat = -1; \
  break; \
}

static
  int
transfer_map_entries(FildeshKV* map, FildeshKV* add_map, const Command* cmd)
{
  int istat = 0;
  FildeshKV_id add_id;
  for (add_id = any_id_FildeshKV(add_map);
       !fildesh_nullid(add_id);
       add_id = any_id_FildeshKV(add_map))
  {
    const char* add_key = (const char*) key_at_FildeshKV(add_map, add_id);
    const SymVal* add_sym = (const SymVal*) value_at_FildeshKV(add_map, add_id);
    const FildeshKV_id id = ensure_FildeshKV( map, add_key, size_of_key_at_FildeshKV(add_map, add_id));
    const SymVal* sym = (const SymVal*) value_at_FildeshKV(map, id);
    if (sym && !(sym->kind==NSymValKinds || sym->kind==HereDocVal || sym->kind==DefVal)) {
      FailBreak(cmd, "Trying to overwrite an existing stream variable", add_key);
    }
    assign_at_FildeshKV(map, id, add_sym, sizeof(*add_sym));
    remove_at_FildeshKV(add_map, add_id);
  }
  return istat;
}

static
  bool
zero_argv_SymValKind(SymValKind kind)
{
  return (
      kind == IDescVal || kind == ODescVal || kind == IODescVal ||
      kind == IFutureDescVal || kind == OFutureDescVal ||
      kind == IFutureDescFileVal || kind == OFutureDescFileVal ||
      kind == ODescStatusVal);
}

static
  int
setup_commands(Command** cmds, FildeshCommandHookup* cmd_hookup)
{
  FildeshKV* map = &cmd_hookup->map;
  /* Temporarily hold new symbols for the current line.*/
  FildeshKV* add_map = &cmd_hookup->add_map;
  int istat = 0;
  unsigned command_index;

  for (command_index = 0;
       command_index < count_of_FildeshAT(cmds) && istat == 0;
       ++command_index)
  {
    unsigned arg_q, arg_r;
    Command* cmd = &(*cmds)[command_index];

    /* The command defines a HERE document.*/
    if (cmd->kind == HereDocCommand)
    {
      /* Command symbol was parsed during parsing
       * but we need to overwrite the symbol
       * just in case there are multiple occurrences.
       */
      SymVal* sym = declare_fildesh_SymVal(map, HereDocVal, cmd->line);
      if (!sym) {istat = -1; break;}
      sym->as.here_doc = cmd->doc;

      /* The loops should not run.*/
      assert( count_of_FildeshAT(cmd->args) == 0 ); /* Invariant.*/
    }

    for (arg_r = 0; arg_r < count_of_FildeshAT(cmd->args) && istat == 0; ++ arg_r) {
      char* arg = (*cmd->args)[arg_r];
      const bool on_argv = !zero_argv_SymValKind(
          peek_fildesh_SymVal_arg(arg, (arg_r == 0)));

      if (eq_cstr("stdin", arg)) {
        cmd->kind = StdinCommand;
        cmd->stdis = cmd_hookup->stdin_fd;
        cmd_hookup->stdin_fd = -1;
        if (cmd->stdis < 0) {
          FailBreak(cmd, "Cannot have multiple stdin commands", arg);
        }
        assert(on_argv);
      }
      else if (eq_cstr("stdout", arg)) {
        cmd->kind = StdoutCommand;
        cmd->stdos = cmd_hookup->stdout_fd;
        cmd_hookup->stdout_fd = -1;
        if (cmd->stdos < 0) {
          FailBreak(cmd, "Cannot have multiple stdout commands", arg);
        }
        assert(on_argv);
      }
      else if (eq_cstr("stderr", arg)) {
        cmd->kind = StderrCommand;
        assert(on_argv);
      }
      else if (eq_cstr("stdargz", arg)) {
        unsigned i;
        (*cmd->args)[arg_r] = (char*)"oargz";
        push_FildeshAT(cmd->args, (char*) "--");
        for (i = 0; i < count_of_FildeshAT(cmd_hookup->stdargs); ++i) {
          push_FildeshAT(cmd->args, (char*)(*cmd_hookup->stdargs)[i]);
        }
        assert(on_argv);
      }
      else if (fildesh_eqstrlit("builtin", arg)) {
        if (arg_r+1 < count_of_FildeshAT(cmd->args) &&
            fildesh_eqstrlit("--", (*cmd->args)[arg_r+1])) {
          unsigned i;
          for (i = arg_r+1; i+1 < count_of_FildeshAT(cmd->args); ++i) {
            (*cmd->args)[i] = (*cmd->args)[i+1];
          }
          mpop_FildeshAT(cmd->args, 1);
        }
        if (arg_r+1 < count_of_FildeshAT(cmd->args) &&
            (*cmd->args)[arg_r+1] &&
            !fildesh_builtin_main_fn_lookup((*cmd->args)[arg_r+1])) {
          FailBreak(cmd, "Unknown builtin", (*cmd->args)[arg_r+1]);
        }
        assert(on_argv);
      }
      if (on_argv) {break;}
    }

    arg_q = 0;
    for (arg_r = 0; arg_r < count_of_FildeshAT(cmd->args) && istat == 0; ++ arg_r)
    {
      char* arg = (*cmd->args)[arg_r];
      const SymValKind kind = parse_fildesh_SymVal_arg(arg, (arg_r == 0));


      if (arg_q == 0 && (kind == ODescFileVal || kind == OFutureDescFileVal)) {
        FailBreak(cmd, "Cannot execute file that this command intends to write",
                  arg);
      }
      else if (arg_q == 0 && kind == IFutureDescFileVal) {
        FailBreak(cmd, "Executable bytes cannot come from below", arg);
      }
      else if (kind == HereDocVal || kind == IDescArgVal)
      {
        SymVal* sym = getf_fildesh_SymVal(map, arg);
        if (sym->kind == HereDocVal) {
          (*cmd->args)[arg_q] = sym->as.here_doc;
        }
        else if (sym->kind == ODescVal) {
          Fildesh_fd fd = sym->as.file_desc;
          sym->kind = NSymValKinds;
          add_iarg_Command (cmd, fd, true);
          (*cmd->args)[arg_q] = NULL;
        }
        else if (sym->kind == DefVal) {
          Fildesh_fd fd = pipe_from_elastic(&(*cmds)[sym->cmd_idx]);
          if (fd < 0) {
            FailBreak(cmd, "Failed to create pipe for variable", arg);
          }
          add_iarg_Command (cmd, fd, true);
          (*cmd->args)[arg_q] = NULL;
        }
        else {
          FailBreak(cmd, "Unknown source for argument", arg);
        }
        ++ arg_q;
      }
      else if (cmd->kind == StdoutCommand && kind == IDescVal)
      {
        SymVal* sym = getf_fildesh_SymVal(map, arg);
        Command* last = &(*cmds)[sym->cmd_idx];

        if (last->kind != RunCommand) {
          FailBreak(cmd, "Stdout stream not coming from a command?", arg);
        }

        fildesh_compat_fd_close(sym->as.file_desc);

        if (sym->ios_idx < count_of_FildeshAT(last->os)) {
          fildesh_compat_fd_copy_to((*last->os)[sym->ios_idx], cmd->stdos);
        }
        else {
          fildesh_compat_fd_copy_to(last->stdos, cmd->stdos);
        }
        sym->kind = NSymValKinds;
        cmd->stdos = -1;
      }
      else if (cmd->kind == StderrCommand && kind == IDescVal)
      {
        SymVal* sym = getf_fildesh_SymVal(map, arg);
        Command* last = &(*cmds)[sym->cmd_idx];

        if (last->kind != RunCommand) {
          FailBreak(cmd, "Stderr stream not coming from a command?", arg);
        }

        if (sym->arg_idx < UINT_MAX) {
          (*last->args)[sym->arg_idx] = (char*)"/dev/stderr";
        }

        fildesh_compat_fd_close(sym->as.file_desc);

        if (sym->ios_idx < count_of_FildeshAT(last->os)) {
          unsigned i;
          unsigned n = count_of_FildeshAT(last->os) - 1;
          /* Close fd and remove it from the list of fds to close.*/
          fildesh_compat_fd_close((*last->os)[sym->ios_idx]);
          for (i = sym->ios_idx; i < n; ++i) {
            (*last->os)[i] = (*last->os)[i+1];
          }
          mpop_FildeshAT(last->os, 1);
        }
        else {
          fildesh_compat_fd_close(last->stdos);
          last->stdos = 2;
        }
        sym->kind = NSymValKinds;
        cmd->stdos = -1;
      }
      else if (kind == IOFileVal) {
        SymVal* sym = getf_fildesh_SymVal(map, arg);
        if (sym->kind != IOFileVal && sym->kind != TmpFileVal) {
          FailBreak(cmd, "Not declared as a file", arg);
        }
        (*cmd->args)[arg_q] = (char*)sym->as.iofilename;
        arg_q += 1;
      }
      else if (kind == IDescVal ||
               kind == IDescFileVal ||
               kind == IODescVal)
      {
        SymVal* sym = getf_fildesh_SymVal(map, arg);
        int fd = sym->as.file_desc;
        if (sym->kind == HereDocVal) {
          /* Do nothing.*/
        }
        else if (sym->kind == ODescVal) {
          sym->kind = NSymValKinds;
        }
        else if (sym->kind == DefVal) {
          fd = pipe_from_elastic(&(*cmds)[sym->cmd_idx]);
        }
        else {
          FailBreak(cmd, "Unknown source for", arg);
        }

        if (sym->kind == HereDocVal) {
          char* filename = write_heredoc_tmpfile(
              cmd, cmd_hookup, sym->as.here_doc);
          if (!filename) {
            FailBreak(cmd, "Cannot create tmpfile for heredoc.", NULL);
          }

          assert(kind != IODescVal);
          if (kind == IDescVal) {
            fd = fildesh_arg_open_readonly(filename);
            cmd->stdis = fd;
          }
          else {
            assert(kind == IDescFileVal);
            (*cmd->args)[arg_q] = filename;
            if (arg_q == 0)
              cmd->exec_doc = sym->as.here_doc;
            ++ arg_q;
          }
        }
        else if (kind == IDescVal || kind == IODescVal) {
          cmd->stdis = fd;
        }
        else if (kind == IDescFileVal) {
          add_ios_Command(cmd, fd, -1);
          if (arg_q > 0) {
            (*cmd->args)[arg_q] = add_fd_arg_Command (cmd, fd);
          } else {
            (*cmd->args)[0] = add_tmp_file_Command(cmd, cmd_hookup, ".exe");
            if (!(*cmd->args)[0]) {
              FailBreak(cmd, "Cannot create tmpfile for executable.", NULL);
            }
            if (sym->arg_idx < UINT_MAX) {
              static const char dev_fd_prefix[] = "/dev/fd/";
              static const unsigned dev_fd_prefix_length =
                sizeof(dev_fd_prefix)-1;
              Command* src_cmd = &(*cmds)[sym->cmd_idx];
              char* src_fd_filename = (*src_cmd->args)[sym->arg_idx];
              int src_fd = -1;
              assert(0 == memcmp(src_fd_filename, dev_fd_prefix,
                                 dev_fd_prefix_length));
              fildesh_parse_int(&src_fd,
                                &src_fd_filename[dev_fd_prefix_length]);
              assert(src_fd >= 0);

              push_FildeshAT(src_cmd->exit_fds, src_fd);
              /* Replace fd path with tmpfile name.*/
              (*src_cmd->args)[sym->arg_idx] = (*cmd->args)[0];
            } else {
              /* Source command isn't directly creating file.*/
              cmd->exec_fd_has_bytes = true;
            }
            cmd->exec_fd = fd;
          }
          ++ arg_q;
        }
        else {
          FailBreak(cmd, "Unexpected kind.", NULL);
        }
      }
      else if (kind == OFutureDescVal || kind == OFutureDescFileVal)
      {
        SymVal* sym = getf_fildesh_SymVal(map, arg);
        int fd;

        if (sym->kind != IFutureDescVal) {
          FailBreak(cmd, "Argument should be a stream to the past", arg);
        }

        sym->kind = NSymValKinds;
        fd = sym->as.file_desc;

        if (kind == OFutureDescVal)
        {
          cmd->stdos = fd;
        }
        else
        {
          add_ios_Command (cmd, fd, -1);
          if (sym->arg_idx > 0)
          {
            (*cmd->args)[arg_q] = add_fd_arg_Command(cmd, fd);
          }
          else
          {
            char* s;
            s = add_tmp_file_Command(&(*cmds)[sym->cmd_idx],
                cmd_hookup, ".exe");
            if (!s) {
              FailBreak(cmd, "Cannot create tmpfile for executable.", NULL);
            }
            (*(*cmds)[sym->cmd_idx].args)[0] = s;
            (*cmd->args)[arg_q] = s;
          }
          ++ arg_q;
        }
      }

      if (cmd->kind == StdinCommand && kind == ODescVal)
      {
        SymVal* sym = declare_fildesh_SymVal(add_map, ODescVal, arg);
        if (!sym) {istat = -1; break;}
        sym->cmd_idx = command_index;
        sym->as.file_desc = cmd->stdis;
        cmd->stdis = -1;
        sym->arg_idx = UINT_MAX;
        sym->ios_idx = UINT_MAX;
      }
      else if (kind == ODescVal || kind == IODescVal ||
               kind == ODescStatusVal ||
               kind == ODescFileVal)
      {
        Fildesh_fd fd[2];
        SymVal* sym = declare_fildesh_SymVal(add_map, ODescVal, arg);
        if (!sym) {istat = -1; break;}
        sym->cmd_idx = command_index;
        sym->arg_idx = UINT_MAX;
        sym->ios_idx = UINT_MAX;

        if (0 != fildesh_compat_fd_pipe(&fd[1], &fd[0])) {
          FailBreak(cmd, "Failed to create pipe for variable", arg);
        }
        assert(fd[0] >= 0);
        assert(fd[1] >= 0);

        sym->as.file_desc = fd[0];

        if (kind == ODescVal || kind == IODescVal) {
          cmd->stdos = fd[1];
        } else if (kind == ODescStatusVal) {
          cmd->status_fd = fd[1];
        } else {
          sym->ios_idx = add_ios_Command (cmd, -1, fd[1]);
          (*cmd->args)[arg_q] = add_fd_arg_Command(cmd, fd[1]);
          sym->arg_idx = arg_q;
          ++ arg_q;
        }
      }
      else if (kind == IFutureDescVal || kind == IFutureDescFileVal)
      {
        Fildesh_fd fd[2];
        SymVal* sym = declare_fildesh_SymVal(add_map, IFutureDescVal, arg);
        if (!sym) {istat = -1; break;}
        sym->cmd_idx = command_index;
        sym->arg_idx = UINT_MAX;
        sym->ios_idx = UINT_MAX;

        if (0 != fildesh_compat_fd_pipe(&fd[1], &fd[0])) {
          FailBreak(cmd, "Failed to create pipe for variable", arg);
        }

        sym->as.file_desc = fd[1];
        if (kind == IFutureDescVal)
        {
          cmd->stdis = fd[0];
        }
        else
        {
          assert(arg_q > 0 && "executable bytes cannot come from below");
          sym->ios_idx = add_ios_Command (cmd, -1, fd[0]);
          (*cmd->args)[arg_q] = add_fd_arg_Command(cmd, fd[0]);
          ++ arg_q;
        }
      }

      if (kind == NSymValKinds) {
        (*cmd->args)[arg_q] = arg;
        ++ arg_q;
      }
    }

    /* This guard might not be necessary.*/
    if (istat != 0) {break;}

    if (count_of_FildeshAT(cmd->args) > 0)
      mpop_FildeshAT(cmd->args, count_of_FildeshAT(cmd->args) - arg_q);

    if (cmd->kind == DefCommand) {
      SymVal* sym = declare_fildesh_SymVal(map, DefVal, cmd->line);
      if (!sym) {istat = -1; break;}
      sym->cmd_idx = command_index;
    }

    istat = transfer_map_entries(map, add_map, cmd);
  }
  return istat;
}

  static void
output_Command(FILE* out, const Command* cmd)
{
  unsigned i;
  if (cmd->kind != RunCommand)  return;

  fprintf(out, "COMMAND line %u: ", cmd->line_num);
  for (i = 0; i < (unsigned)count_of_FildeshAT(cmd->args); ++i) {
    if (i > 0)  fputc(' ', out);
    if ((*cmd->args)[i]) {
      fputs((*cmd->args)[i], out);
    } else {
      fputs("NULL", out);
    }
  }
  fputc('\n', out);
}

  static void
show_usage()
{
  const char fildesh_exe[] = "fildesh";
  fprintf(stderr, "Usage: %s [[-f] SCRIPTFILE | -- SCRIPT]\n",
          fildesh_exe);
}

FILDESH_POSIX_THREAD_CALLBACK(builtin_command_thread_fn, BuiltinCommandThreadArg*, st)
{
  Command* cmd = st->command;
  unsigned offset = 0;
  unsigned argc = 0;
  char** argv;
  FildeshX** inputs = NULL;
  FildeshO** outputs = NULL;
  int (*main_fn)(unsigned, char**, FildeshX**, FildeshO**) = NULL;
  unsigned i;
  char* name = NULL;
  bool only_argv = false;

  assert(count_of_FildeshAT(cmd->iargs) == 0);
  assert(cmd->exec_fd < 0);

  for (i = 0; st->argv[i]; ++i) {
    argc = i+1;
  }
  assert(argc >= 3);

  for (i = 2; i < argc; ++i) {
    if (0 == strcmp("-as", st->argv[i-1])) {
      offset = i;
      break;
    }
  }
  /* We should have found something. Retain argv[0].*/
  assert(offset >= 2);
  name = st->argv[offset];
  st->argv[offset] = st->argv[offset-2];
  st->argv[offset-2] = name;

  main_fn = fildesh_builtin_threadsafe_fn_lookup(name);
  assert(main_fn);

  argc -= offset;
  argv = &st->argv[offset];
  only_argv = (0 == strcmp("execfd", name));
  inputs = (FildeshX**) malloc(sizeof(FildeshX*) * (argc+1));
  outputs = (FildeshO**) malloc(sizeof(FildeshO*) * (argc+1));
  for (i = 0; i <= argc; ++i) {
    inputs[i] = NULL;
    outputs[i] = NULL;
  }
  if (cmd->stdis >= 0) {
    inputs[0] = open_fd_FildeshX(cmd->stdis);
    cmd->stdis = -1;
  }
  if (cmd->stdos >= 0) {
    if (cmd->stdos == 2) {
      outputs[0] = open_FildeshOF("/dev/stderr");
    }
    else {
      outputs[0] = open_fd_FildeshO(cmd->stdos);
    }
    cmd->stdos = -1;
  }
  /* Much like `cmd->stdis` and `cmd->stdos` above, we forget
   * about all file descriptors and trust that the builtin will close them.
   */
  mpop_FildeshAT(cmd->is, count_of_FildeshAT(cmd->is));
  mpop_FildeshAT(cmd->os, count_of_FildeshAT(cmd->os));

  if (false) {
    for (i = 0; i < argc; ++i) {
      fildesh_log_tracef("%u argv[%u]: %s", cmd->line_num, i, argv[i]);
    }
  }

  if (main_fn && !only_argv) {
    cmd->status = main_fn(argc, argv, inputs, outputs);
  } else if (main_fn && only_argv) {
    cmd->status = main_fn(argc, argv, NULL, NULL);
  } else {
    cmd->status = -1;
  }

  for (i = 0; i < argc; ++i) {
    /* Builtin should have freed everything.*/
    assert(!inputs[i]);
    assert(!outputs[i]);
  }

  /* Free memory.*/
  free(inputs);
  free(outputs);
  close_Command(cmd);
  for (i = 0; st->argv[i]; ++i) {
    free(st->argv[i]);
  }
  free(st->argv);
  free(st);
}

static
  void
add_inheritfd_flags_Command(char*** argv, Command* cmd, bool inprocess) {
  unsigned i;

  if (inprocess) {
    /* Let command inherit all input file descriptors except:
     * - The fd providing executable bytes (or signaling that they are ready).
     * - The fds providing input arguments. See inprocess else clause.
     */
    for (i = 0; i < count_of_FildeshAT(cmd->is); ++i) {
      const Fildesh_fd fd = (*cmd->is)[i];
      if (fd != cmd->exec_fd) {
        push_FildeshAT(argv, lace_strdup("-inheritfd") );
        push_FildeshAT(argv, lace_fd_strdup(fd) );
      }
    }
    /* Let command inherit all output file descriptors except:
     * - The fds that must be closed on exit.
     */
    for (i = 0; i < count_of_FildeshAT(cmd->os); ++i) {
      const Fildesh_fd fd = (*cmd->os)[i];
      bool inherit = true;
      unsigned j;
      for (j = 0; j < count_of_FildeshAT(cmd->exit_fds) && inherit; ++j) {
        inherit = (fd != (*cmd->exit_fds)[j]);
      }
      if (inherit) {
        push_FildeshAT(argv, lace_strdup("-inheritfd"));
        push_FildeshAT(argv, lace_fd_strdup(fd));
      }
    }

    if (cmd->stdis >= 0) {
      push_FildeshAT(argv, lace_strdup("-stdin"));
      push_FildeshAT(argv, lace_fd_path_strdup(cmd->stdis));
      push_FildeshAT(cmd->is, cmd->stdis);
    }
    cmd->stdis = -1;
    if (cmd->stdos >= 0) {
      push_FildeshAT(argv, lace_strdup("-stdout"));
      push_FildeshAT(argv, lace_fd_path_strdup(cmd->stdos));
      push_FildeshAT(cmd->os, cmd->stdos);
    }
    cmd->stdos = -1;
  } else {
    for (i = 0; i < count_of_FildeshAT(cmd->iargs); ++i) {
      add_ios_Command(cmd, (*cmd->iargs)[i].fd, -1);
    }
  }
  close_FildeshAT(cmd->iargs);

  if (cmd->exec_fd >= 0) {
    if (!cmd->exec_fd_has_bytes) {
      push_FildeshAT(argv, lace_strdup("-waitfd"));
      push_FildeshAT(argv, lace_fd_strdup(cmd->exec_fd));
    }
    add_ios_Command(cmd, cmd->exec_fd, -1);
  }
  cmd->exec_fd = -1;

  for (i = 0; i < count_of_FildeshAT(cmd->exit_fds); ++i) {
    push_FildeshAT(argv, lace_strdup("-exitfd"));
    push_FildeshAT(argv, lace_fd_strdup((*cmd->exit_fds)[i]));
  }
  close_FildeshAT(cmd->exit_fds);

  if (cmd->status_fd >= 0) {
    push_FildeshAT(argv, lace_strdup("-o?"));
    push_FildeshAT(argv, lace_fd_path_strdup(cmd->status_fd));
    push_FildeshAT(cmd->os, cmd->status_fd);
  }
  cmd->status_fd = -1;
}

  static int
spawn_commands(const char* fildesh_exe, Command** cmds,
               FildeshKV* alias_map, bool forkonly)
{
  typedef struct uint2 uint2;
  struct uint2 {
    unsigned s[2];
  };
  DECLARE_DEFAULT_FildeshAT(char*, argv);
  DECLARE_DEFAULT_FildeshAT(uint2, fdargs);
  unsigned i;
  int istat = 0;

  for (i = 0; i < count_of_FildeshAT(cmds) && istat == 0; ++i)
  {
    Command* cmd = &(*cmds)[i];
    bool use_thread = false;
    unsigned argi, j;

    if (cmd->kind != RunCommand && cmd->kind != DefCommand)  continue;

    for (argi = 0; argi < count_of_FildeshAT(cmd->args); ++argi)
    {
      if (!(*cmd->args)[argi]) {
        uint2 p;
        assert(count_of_FildeshAT(fdargs) < count_of_FildeshAT(cmd->iargs));
        p.s[0] = argi;
        p.s[1] = (*cmd->iargs)[count_of_FildeshAT(fdargs)].fd;
        push_FildeshAT(fdargs, p);
      }
    }
    assert(count_of_FildeshAT(fdargs) == count_of_FildeshAT(cmd->iargs));

    fildesh_command_fix_known_flags(cmd->args, alias_map, cmd->alloc);

    if (cmd->exec_fd >= 0 || count_of_FildeshAT(fdargs) > 0 ||
        cmd->status_fd >=0 || count_of_FildeshAT(cmd->exit_fds) > 0)
    {
      const char* execfd_exe = lookup_strmap(alias_map, "execfd");
      char* execfd_fmt = NULL;
      if (execfd_exe) {
        push_FildeshAT(argv, lace_strdup(execfd_exe));
      } else {
        if (!forkonly) {
          use_thread = true;
        }
        push_FildeshAT(argv, lace_strdup(fildesh_exe));
        push_FildeshAT(argv, lace_strdup("-as"));
        push_FildeshAT(argv, lace_strdup("execfd"));
      }
      if (cmd->exec_fd >= 0) {
        push_FildeshAT(argv, lace_strdup("-exe"));
        push_FildeshAT(argv, lace_strdup((*cmd->args)[0]));
        if (cmd->exec_fd_has_bytes) {
          uint2 p;
          p.s[0] = 0;
          p.s[1] = cmd->exec_fd;
          push_FildeshAT(fdargs, p);
        }
      }

      add_inheritfd_flags_Command(argv, cmd, use_thread);

      execfd_fmt = (char*)malloc(2*count_of_FildeshAT(cmd->args));
      for (j = 0; j < count_of_FildeshAT(cmd->args); ++j) {
        execfd_fmt[2*j] = 'a';
        execfd_fmt[2*j+1] = '_';
      }
      execfd_fmt[2*count_of_FildeshAT(cmd->args)-1] = '\0';

      for (j = 0; j < count_of_FildeshAT(fdargs); ++j) {
        uint2 p = (*fdargs)[j];
        (*cmd->args)[p.s[0]] = strdup_fd_Command(cmd, p.s[1]);
        execfd_fmt[2*p.s[0]] = 'x';
      }

      push_FildeshAT(argv, execfd_fmt);
      push_FildeshAT(argv, lace_strdup("--"));
      push_FildeshAT(argv, lace_strdup((*cmd->args)[0]));
    }
    else if (0 == strcmp("builtin", (*cmd->args)[0])) {
      assert(fildesh_builtin_main_fn_lookup((*cmd->args)[1]));
      if (!forkonly) {
        use_thread = !!fildesh_builtin_threadsafe_fn_lookup((*cmd->args)[1]);
      }
      push_FildeshAT(argv, lace_strdup(fildesh_exe));
      push_FildeshAT(argv, lace_strdup("-as"));
    }
    else if (fildesh_builtin_main_fn_lookup((*cmd->args)[0])) {
      if (!forkonly) {
        use_thread = !!fildesh_builtin_threadsafe_fn_lookup((*cmd->args)[0]);
      }
      push_FildeshAT(argv, lace_strdup(fildesh_exe));
      push_FildeshAT(argv, lace_strdup("-as"));
      push_FildeshAT(argv, lace_strdup((*cmd->args)[0]));
    }
    else {
      push_FildeshAT(argv, lace_strdup((*cmd->args)[0]));
    }

    for (j = 1; j < count_of_FildeshAT(cmd->args); ++j)
      push_FildeshAT(argv, lace_strdup((*cmd->args)[j]));

    push_FildeshAT(argv, NULL);

    if (cmd->exec_doc)
    {
      cmd->exec_doc = 0;
      fildesh_compat_file_chmod_u_rwx((*cmd->args)[0], 1, 1, 1);
    }

    if (use_thread) {
      BuiltinCommandThreadArg* arg = (BuiltinCommandThreadArg*)
        malloc(sizeof(BuiltinCommandThreadArg));
      arg->command = cmd;
      arg->argv = (char**)malloc(sizeof(char*) * count_of_FildeshAT(argv));
      memcpy(arg->argv, (*argv), sizeof(char*) * count_of_FildeshAT(argv));
      cmd->pid = 0;
      istat = pthread_create(
          &cmd->thread, NULL, builtin_command_thread_fn, arg);
      if (istat < 0) {
        fildesh_log_errorf("Could not pthread_create(). File: %s", (*argv)[0]);
      }
    } else {
      Fildesh_fd* fds_to_inherit =
        build_fds_to_inherit_Command(cmd);
      cmd->pid = fildesh_compat_fd_spawnvp(
          cmd->stdis, cmd->stdos, 2, fds_to_inherit, (const char**)(*argv));
      free(fds_to_inherit);
      cmd->stdis = -1;
      cmd->stdos = -1;
      close_Command(cmd);
      if (cmd->pid < 0) {
        fildesh_log_errorf("Could not spawnvp(). File: %s", (*argv)[0]);
        istat = -1;
      }
      for (argi = 0; argi < count_of_FildeshAT(argv); ++argi)
        free((*argv)[argi]);
    }
    mpop_FildeshAT(fdargs, count_of_FildeshAT(fdargs));
    mpop_FildeshAT(argv, count_of_FildeshAT(argv));
  }
  close_FildeshAT(argv);
  close_FildeshAT(fdargs);
  return istat;
}


/** Returns an appropriate exit status.**/
static int handle_flag_tmpdir_from_env(const char* var) {
  const char* d = NULL;
  int istat;
  if (var) {
    d = getenv(var);
  }
  if (!d) {
    fildesh_log_errorf("Failed to read environment variable: %s", var);
    return 64;
  }
  istat = fildesh_compat_sh_setenv("TMPDIR", d);
#ifdef _MSC_VER
  if (istat == 0) {
    istat = fildesh_compat_sh_setenv("TEMP", d);
  }
#endif
  if (istat != 0) {
    fildesh_compat_errno_trace();
    fildesh_log_error("Failed to set testing temporary directory.");
    return 71;
  }
  return 0;
}


  int
fildesh_builtin_fildesh_main(unsigned argc, char** argv,
                             FildeshX** inputv, FildeshO** outputv)
{
  char* fildesh_exe = argv[0];
  Command** cmds = NULL;
  bool use_stdin = true;
  FildeshX* script_in = NULL;
  size_t script_line_count = 0;
  FildeshO tmp_out[1] = {DEFAULT_FildeshO};
  FildeshAlloc* global_alloc;
  FildeshCommandHookup* cmd_hookup;
  unsigned argi = 1;
  unsigned i;
  FildeshKV alias_map[1] = {DEFAULT_FildeshKV};
  bool forkonly = false;
  bool exiting = false;
  int exstatus = 0;
  int istat;

  /* We shouldn't have an error yet.*/
  fildesh_compat_errno_clear();

  /* With all the signal handling below,
   * primarily to clean up the temp directory,
   * it's not clear how to act as a proper builtin.
   */
  assert(!inputv);
  assert(!outputv);

  global_alloc = open_FildeshAlloc();
  cmd_hookup = new_FildeshCommandHookup(global_alloc);
  defer_FildeshCommandHookup(cmd_hookup, close_FildeshO_generic, tmp_out);
  defer_FildeshCommandHookup(cmd_hookup, close_FildeshKV_generic, alias_map);

  while (argi < argc && exstatus == 0 && !exiting) {
    const char* arg;
    arg = argv[argi++];
    if (arg[0] == '-' && arg[1] == '-' && arg[2] != '\0') {
      arg = &arg[1];
    }
    if (eq_cstr (arg, "--")) {
      use_stdin = false;
      if (argi >= argc) {
        show_usage();
        exstatus = 64;
        break;
      }
      if (!script_in) {
        script_in = open_FildeshXA();
      }
      while (argi < argc) {
        size_t sz;
        arg = argv[argi++];
        sz = strlen(arg);
        memcpy(grow_FildeshX(script_in, sz), arg, sz);
        *grow_FildeshX(script_in, 1) = '\n';
      }
    }
    else if (eq_cstr (arg, "-version")) {
      FildeshO* out = open_FildeshOF("/dev/stdout");
      putstrlit_FildeshO(out, fildesh_bin_version);
      putc_FildeshO(out, '\n');
      close_FildeshO(out);
      exiting = true;
    }
    else if (eq_cstr (arg, "-as")) {
      argi -= 1;
      argv[argi] = fildesh_exe;
      exstatus = fildesh_main_builtin(argc-argi, &argv[argi]);
      exiting = true;
    }
    else if (eq_cstr(arg, "-alias")) {
      char* k = argv[argi++];
      char* v = NULL;
     if (k) {
       v = strchr(k, '=');
     }
     if (k && v) {
       v[0] = '\0';
       v = &v[1];
       ensure_strmap(alias_map, k, v);
       if (eq_cstr(k, "fildesh")) {
         fildesh_exe = v;
       }
     } else {
        fildesh_log_errorf("Failed alias: %s", k);
        exstatus = 64;
      }
    }
    else if (eq_cstr(arg, "-a")) {
      char* k = argv[argi++];
      char* v = NULL;
     if (k) {
       v = strchr(k, '=');
     }
     if (k && v) {
       SymVal* sym;
       v[0] = '\0';
       v = &v[1];
       sym = declare_fildesh_SymVal(&cmd_hookup->map, HereDocVal, k);
       if (sym) {
         sym->as.here_doc = v;
       }
       else {
         exstatus = 64;
       }
     } else {
        fildesh_log_errorf("Bad -a arg: %s", k);
        exstatus = 64;
      }
    }
    else if (eq_cstr(arg, "-tmpdir_from_env")) {
      exstatus = handle_flag_tmpdir_from_env(argv[argi++]);
    }
    else if (eq_cstr(arg, "-setenv")) {
      char* k = argv[argi++];
      char* v = NULL;
      if (k) {
        v = strchr(k, '=');
      }
      if (k && v) {
        v[0] = '\0';
        v = &v[1];
        istat = fildesh_compat_sh_setenv(k, v);
        if (istat != 0) {
          fildesh_log_error("Can't -setenv!");
          exstatus = 71;
        }
      } else {
        fildesh_log_errorf("Bad -setenv arg: %s", k);
        exstatus = 64;
      }
    }
    else if (eq_cstr(arg, "-forkonly")) {
      forkonly = true;
    }
    else if (eq_cstr (arg, "-stdin")) {
      const char* stdin_filepath = argv[argi++];
      Fildesh_fd fd = fildesh_arg_open_readonly(stdin_filepath);
      if (fd >= 0) {
        istat = fildesh_compat_fd_move_to(cmd_hookup->stdin_fd, fd);
        if (istat != 0) {
          fildesh_log_error("Failed to dup2 -stdin.");
          exstatus = 72;
        }
      } else {
        fildesh_log_errorf("Failed to open stdin: %s", stdin_filepath);
        exstatus = 66;
      }
    }
    else if (eq_cstr (arg, "-stdout")) {
      const char* stdout_filepath = argv[argi++];
      Fildesh_fd fd = fildesh_arg_open_writeonly(stdout_filepath);
      if (fd == 2) {
        fildesh_log_error("The -stdout flag does not support redirecting to stderr.");
        exstatus = 64;
      }
      else if (fd >= 0) {
        istat = fildesh_compat_fd_move_to(cmd_hookup->stdout_fd, fd);
        if (istat != 0) {
          fildesh_log_error("Failed to dup2 -stdout.");
          exstatus = 72;
        }
      } else {
        fildesh_log_errorf("Failed to open stdout: %s", stdout_filepath);
        exstatus = 73;
      }
    }
    else if (eq_cstr (arg, "-stderr")) {
      const char* stderr_filepath = argv[argi++];
      Fildesh_fd fd = -1;
      if (0 == strcmp("-", stderr_filepath) ||
          0 == strcmp("/dev/stdout", stderr_filepath) ||
          0 == strcmp("/dev/fd/1", stderr_filepath))
      {
        if (cmd_hookup->stdout_fd == 1) {
          cmd_hookup->stdout_fd = -1;
        }
        else {
          fildesh_log_error("The -stderr flag does not support redirecting to a redirected stdout.");
          exstatus = 64;
        }
      }
      if (exstatus == 0) {
        fd = fildesh_arg_open_writeonly(stderr_filepath);
      }
      if (fd == 2) {
        /* No-op if fd isn't changing.*/
      }
      else if (fd >= 0) {
        istat = fildesh_compat_fd_move_to(2, fd);
        if (istat == 0) {
          cmd_hookup->stderr_fd_opened = true;
        }
        else {
          fildesh_log_error("Failed to dup2 -stderr.");
          exstatus = 72;
        }
      }
      else if (exstatus != 0) {
        /* No-op if we already set an exit code.*/
      }
      else {
        fildesh_log_errorf("Failed to open stderr: %s", stderr_filepath);
        exstatus = 73;
      }
    }
    else {
      /* Optional -f flag.*/
      if (eq_cstr (arg, "-x") || eq_cstr (arg, "-f")) {
        if (argi >= argc) {
          show_usage();
          exstatus = 64;
          break;
        }
        arg = argv[argi++];
      }
      use_stdin = false;
      if (!arg) {
        script_in = open_arg_FildeshXF(argi-1, argv, inputv);
        if (!script_in) {
          fildesh_log_errorf("Cannot read script from builtin.");
          exstatus = 66;
        }
        break;
      }
      if (0 == strcmp("-", arg) ||
          0 == strcmp("/dev/stdin", arg) ||
          0 == strcmp("/dev/fd/0", arg))
      {
        cmd_hookup->stdin_fd = -1;
      }
      script_in = open_arg_FildeshXF(argi-1, argv, inputv);
      if (!script_in) {
        fildesh_compat_errno_trace();
        fildesh_log_errorf("Cannot read script. File: %s", arg);
        exstatus = 66;
      }
      break;
    }
  }

  if (!exiting && exstatus == 0) {
    assert(argi <= argc);
    assert(!argv[argc]);
    exstatus = fildesh_syntax_parse_flags(
        &argv[argi], &cmd_hookup->map, cmd_hookup->stdargs,  tmp_out);
  }
  if (exiting || exstatus != 0) {
    close_FildeshCommandHookup(cmd_hookup);
    close_FildeshAlloc(global_alloc);
    return exstatus;
  }
  push_fildesh_exit_callback(close_FildeshAlloc_generic, global_alloc);
  push_fildesh_exit_callback(close_FildeshCommandHookup_generic, cmd_hookup);

  {
    DECLARE_DEFAULT_FildeshAT(Command, tmp_cmds);
    cmds = (Command**)malloc(sizeof(tmp_cmds));
    memcpy(cmds, tmp_cmds, sizeof(tmp_cmds));
  }
  defer_FildeshCommandHookup(cmd_hookup, lose_Commands, cmds);

  if (use_stdin) {
    script_in = open_arg_FildeshXF(0, argv, inputv);
    cmd_hookup->stdin_fd = -1;
  }
  if (cmd_hookup->stdin_fd == 0) {
    cmd_hookup->stdin_fd = fildesh_compat_fd_claim(cmd_hookup->stdin_fd);
  }
  if (cmd_hookup->stdout_fd == 1) {
    cmd_hookup->stdout_fd = fildesh_compat_fd_claim(cmd_hookup->stdout_fd);
  }
  /* Stderr stays at fd 2. Spawned processes inherit it directly.
   * It will be closed before exit iff it was copied from a different fd.
   */

  fildesh_compat_errno_trace();
  while (exstatus == 0 && script_in) {
    const Fildesh_fd stdout_fd = cmd_hookup->stdout_fd;
    FildeshAlloc* scope_alloc = open_FildeshAlloc();
    istat = parse_file(
        cmd_hookup, cmds,
        script_in, &script_line_count, filename_FildeshXF(script_in),
        &cmd_hookup->map, scope_alloc, global_alloc, tmp_out);
    if (istat != 0 ||
        (count_of_FildeshAT(cmds) == 0 || last_FildeshAT(cmds).kind != BarrierCommand))
    {
      close_FildeshX(script_in);
      fildesh_compat_errno_trace();
      script_in = NULL;
    }
    if (istat == 0) {
      fildesh_compat_errno_trace();
      istat = setup_commands(cmds, cmd_hookup);
      fildesh_compat_errno_trace();
    }
    if (exstatus == 0 && istat != 0) {
      exstatus = 65;
    }
    if (false && exstatus == 0) {
      for (i = 0; i < count_of_FildeshAT(cmds); ++i) {
        output_Command (stderr, &(*cmds)[i]);
      }
    }

    if (exstatus == 0) {
      fildesh_compat_errno_trace();
      istat = spawn_commands(fildesh_exe, cmds, alias_map, forkonly);
      fildesh_compat_errno_trace();
    }
    if (exstatus == 0 && istat != 0) {
      exstatus = 126;
    }

    istat = 0;
    for (i = 0; i < count_of_FildeshAT(cmds); ++i) {
      Command* cmd = &(*cmds)[i];
      if (false && exstatus == 0) {
        output_Command(stderr, cmd);
      }
      if ((cmd->kind == RunCommand || cmd->kind == DefCommand) && cmd->pid >= 0) {
        if (cmd->pid == 0) {
          pthread_join(cmd->thread, NULL);
        } else {
          cmd->status = fildesh_compat_sh_wait(cmd->pid);
        }
        cmd->pid = -1;
        if (cmd->status != 0) {
          fputs("FAILED ", stderr);
          output_Command(stderr, cmd);
          if (istat < 127) {
            /* Not sure what to do here. Just accumulate.*/
            istat += 1;
          }
        }
      }

      lose_Command(cmd);
    }
    mpop_FildeshAT(cmds, count_of_FildeshAT(cmds));
    fildesh_compat_errno_trace();
    close_FildeshAlloc(scope_alloc);

    if (exstatus == 0) {
      exstatus = istat;
    }
    cmd_hookup->stdout_fd = stdout_fd;
  }

  close_FildeshX(script_in);  /* Just in case we missed it.*/
  close_FildeshCommandHookup(cmd_hookup);
  return exstatus;
}
