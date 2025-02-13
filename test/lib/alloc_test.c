
#include <fildesh/fildesh.h>

#include <assert.h>
#include <string.h>

static void alloc_test()
{
  FildeshAlloc* alloc = open_FildeshAlloc();
  char* bufs[3];
  size_t size = ((size_t)1 << FILDESH_ALLOC_MIN_BLOCK_LGSIZE);
  bufs[0] = fildesh_allocate(char, size+1, alloc);
  memset(bufs[0], 'a', size+1);

  size *= 2;
  bufs[1] = fildesh_allocate(char, size+1, alloc);
  memset(bufs[1], 'b', size+1);

  size *= 2;
  bufs[2] = fildesh_allocate(char, size+1, alloc);
  memset(bufs[2], 'c', size+1);

  assert(bufs[0][1] == 'a');
  assert(bufs[1][0] == 'b');
  assert(bufs[2][0] == 'c');

  /* Zero-size allocation returns non-NULL but does not reserve anything.*/
  assert(fildesh_allocate(char, 0, alloc));
  assert(fildesh_allocate(char, 0, alloc) == fildesh_allocate(char, 0, alloc));

  close_FildeshAlloc(alloc);
}

static void strdup_test()
{
  const char expect[] = "Hello world I am a string!";
  FildeshAlloc* alloc = open_FildeshAlloc();
  const char* result = strdup_FildeshAlloc(alloc, expect);
  assert(0 == strcmp(expect, result));
  close_FildeshAlloc(alloc);
}

int main() {
  alloc_test();
  strdup_test();
  return 0;
}
