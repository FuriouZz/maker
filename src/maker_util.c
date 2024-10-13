#include <maker/maker_util.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "maker_internal.h"

int maker_strfmt(char *buf, int buf_size, const char *fmt, va_list args) {
  int result = -1;
  MAKER_ASSERT(buf);
  MAKER_ASSERT(buf_size);
  if (!buf || !buf_size || !fmt) {
    return 0;
  }
  result = vsnprintf(buf, buf_size, fmt, args);
  return result;
}

void *maker_malloc(size_t size) { return malloc(size); }

void maker_free(void *ptr) { free(ptr); }

void maker_clear(void *ptr, size_t size) {
  MAKER_ASSERT(ptr && (size > 0));
  memset(ptr, 0, size);
}

void *maker_malloc_clear(size_t size) {
  void *ptr = maker_malloc(size);
  maker_clear(ptr, size);
  return ptr;
}
