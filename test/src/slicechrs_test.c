
#include <fildesh/fildesh.h>

#include <assert.h>
#include <string.h>

static
  void
slicechrs_easy_test()
{
  FildeshX in[1] = { DEFAULT_FildeshX };
  FildeshX slice;
  char content[] = "(i.am some|content.)";
  const char delims[] = " ().|";

  in->at = content;
  in->size = strlen(content);

  slice = until_chars_FildeshX(in, delims);
  assert(slice.size == 0);
  assert(peek_char_FildeshX(in, '('));

  while_chars_FildeshX(in, delims);
  assert(peek_char_FildeshX(in, 'i'));
  slice = until_chars_FildeshX(in, delims);
  assert(slice.size == 1);
  assert(0 == memcmp(slice.at, "i", 1));

  while_chars_FildeshX(in, delims);
  assert(peek_char_FildeshX(in, 'a'));
  slice = until_chars_FildeshX(in, delims);
  assert(slice.size == 2);
  assert(0 == memcmp(slice.at, "am", 2));

  while_chars_FildeshX(in, delims);
  slice = until_chars_FildeshX(in, delims);
  assert(slice.size == 4);
  assert(0 == memcmp(slice.at, "some", 4));

  while_chars_FildeshX(in, delims);
  slice = until_chars_FildeshX(in, delims);
  assert(slice.size == 7);
  assert(0 == memcmp(slice.at, "content", 7));

  while_chars_FildeshX(in, delims);
  slice = until_chars_FildeshX(in, delims);
  assert(slice.size == 0);
  assert(!slice.at);
  assert(!peek_char_FildeshX(in, ')'));
  assert(!peek_char_FildeshX(in, '\0'));
  assert(!peek_byte_FildeshX(in, 0));
}

int main() {
  slicechrs_easy_test();
  return 0;
}
