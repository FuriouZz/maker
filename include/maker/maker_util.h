#ifndef MAKER_UTIL_H
#define MAKER_UTIL_H

#include <stdarg.h>
#include <stddef.h>

extern int maker_strfmt(char *buf, int buf_size, const char *fmt, va_list args);

extern void *maker_malloc(size_t size);

extern void maker_free(void *ptr);

extern void maker_clear(void *ptr, size_t size);

extern void *maker_malloc_clear(size_t size);
#endif
