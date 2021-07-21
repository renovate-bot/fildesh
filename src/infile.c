
#include "lace.h"
#include "lace_compat_fd.h"
#include "lace_compat_string.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

typedef struct LaceXF LaceXF;
struct LaceXF {
  LaceX base;
  lace_fd_t fd;
  /* unsigned basename_offset; */
  char* filename;
};


static
  void
read_LaceXF(LaceXF* xf)
{
  static const size_t chunksize = 4096;
  const size_t orig_size = xf->base.size;
  char* buf = grow_LaceX(&xf->base, chunksize);
  xf->base.size = orig_size + lace_compat_fd_read(xf->fd, buf, chunksize);
}

static
  void
close_LaceXF(LaceXF* xf)
{
  if (xf->fd >= 0) {
    lace_compat_fd_close(xf->fd);
    xf->fd = -1;
  }
  if (xf->filename) {
    free(xf->filename);
    xf->filename = NULL;
  }
}

static
  void
free_LaceXF(LaceXF* xf)
{
  free(xf);
}

DEFINE_LaceX_VTable(LaceXF, base);

static inline LaceXF default_LaceXF() {
  LaceXF tmp = {DEFAULT_LaceX, -1, NULL};
  tmp.base.vt = DEFAULT_LaceXF_LaceX_VTable;
  return tmp;
}

static lace_fd_t lace_open_null_readonly() {
  lace_compat_fd_t fd = -1;
  lace_compat_fd_t tmp_fd = -1;
  int istat;
  istat = lace_compat_fd_pipe(&tmp_fd, &fd);
  if (istat != 0) {return -1;}
  istat = lace_compat_fd_close(tmp_fd);
  if (istat != 0) {lace_compat_fd_close(fd); return -1;}
  return fd;
}

static LaceX* open_null_LaceXF() {
  LaceXF* xf;
  lace_compat_fd_t fd = lace_open_null_readonly();
  if (fd < 0) {return NULL;}
  xf = (LaceXF*) malloc(sizeof(LaceXF));
  *xf = default_LaceXF();
  xf->fd = fd;
  xf->filename = lace_compat_string_duplicate("/dev/null");
  return &xf->base;
}

  LaceX*
open_LaceXF(const char* filename)
{
  return open_sibling_LaceXF(NULL, filename);
}

  LaceX*
open_sibling_LaceXF(const char* sibling, const char* filename)
{
  static const char dev_stdin[] = "/dev/stdin";
  static const char dev_null[] = "/dev/null";
  static const char dev_fd_prefix[] = "/dev/fd/";
  static const unsigned dev_fd_prefix_length = sizeof(dev_fd_prefix)-1;
  const size_t filename_length = (filename ? strlen(filename) : 0);
  LaceXF xf[1];

  if (!filename) {return NULL;}

  if (0 == strcmp("-", filename) || 0 == strcmp(dev_stdin, filename)) {
    return open_fd_LaceX(0);
  }
  if (0 == strcmp(dev_null, filename)) {
    return open_null_LaceXF();
  }
  if (0 == strncmp(dev_fd_prefix, filename, dev_fd_prefix_length)) {
    int fd = -1;
    char* s = lace_parse_int(&fd, &filename[dev_fd_prefix_length]);
    if (!s) {return NULL;}
    return open_fd_LaceX(fd);
  }

  *xf = default_LaceXF();
  if (filename[0] != '/' && sibling) {
    size_t sibling_dirlen = 0;
    if (sibling) {
      char* p = strrchr(sibling, '/');
      if (p) {
        sibling_dirlen = (size_t)(p - sibling) + 1;
      }
    }
    xf->filename = malloc(sibling_dirlen + filename_length + 1);
    memcpy(xf->filename, sibling, sibling_dirlen);
    memcpy(&xf->filename[sibling_dirlen], filename, filename_length+1);
  }
  else {
    xf->filename = lace_compat_string_duplicate(filename);
  }

  xf->fd = lace_compat_file_open_readonly(xf->filename);
  if (xf->fd >= 0) {
    LaceXF* p = malloc(sizeof(LaceXF));
    *p = *xf;
    return &p->base;
  }

  if (xf->filename) {free(xf->filename);}
  return NULL;
}

  lace_fd_t
lace_arg_open_readonly(const char* filename)
{
  static const char dev_stdin[] = "/dev/stdin";
  static const char dev_null[] = "/dev/null";
  static const char dev_fd_prefix[] = "/dev/fd/";
  static const unsigned dev_fd_prefix_length = sizeof(dev_fd_prefix)-1;

  if (!filename) {return -1;}

  if (0 == strcmp("-", filename) || 0 == strcmp(dev_stdin, filename)) {
    return lace_compat_fd_claim(0);
  }
  if (0 == strcmp(dev_null, filename)) {
    return lace_open_null_readonly();
  }
  if (0 == strncmp(dev_fd_prefix, filename, dev_fd_prefix_length)) {
    int fd = -1;
    char* s = lace_parse_int(&fd, &filename[dev_fd_prefix_length]);
    if (!s) {return -1;}
    return lace_compat_fd_claim(fd);
  }

  return lace_compat_file_open_readonly(filename);
}

  LaceX*
open_fd_LaceX(lace_fd_t fd)
{
  char filename[LACE_FD_PATH_SIZE_MAX];
  unsigned filename_size;
  LaceXF* xf;
  fd = lace_compat_fd_claim(fd);
  if (fd < 0) {return NULL;}
  xf = (LaceXF*) malloc(sizeof(LaceXF));
  *xf = default_LaceXF();
  /* File descriptor.*/
  xf->fd = fd;
  /* Filename.*/
  filename_size = 1 + lace_encode_fd_path(filename, fd);
  xf->filename = (char*)malloc(filename_size);
  memcpy(xf->filename, filename, filename_size);
  return &xf->base;
}

  LaceX*
open_arg_LaceXF(unsigned argi, char** argv, LaceX** inputv)
{
  if (inputv && inputv[argi]) {
    LaceX* ret = inputv[argi];
    inputv[argi] = NULL;  /* Claim it.*/
    return ret;
  }
  if (!argv[argi]) {return NULL;}
  if (argi == 0 || (argv[argi][0] == '-' && argv[argi][1] == '\0')) {
    if (inputv) {
      if (inputv[0]) {
        LaceX* ret = inputv[0];
        inputv[0] = NULL;  /* Claim it.*/
        return ret;
      } else {
        return NULL; /* Better not steal the real stdin.*/
      }
    }
    return open_fd_LaceX(0);
  }
  return open_LaceXF(argv[argi]);
}

const char* filename_LaceXF(LaceX* in) {
  if (in->vt != DEFAULT_LaceXF_LaceX_VTable) {
    return NULL;
  }
  return lace_castup(LaceXF, base, in)->filename;
}
