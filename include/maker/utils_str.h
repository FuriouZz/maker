#ifndef MK_UTILS_STRING_H
#define MK_UTILS_STRING_H

#include <stddef.h>
#include <stdint.h>

extern int mk_str_fmt(char* buf, int buf_size, const char* fmt, ...);

extern char* mk_str_concat(const char* str, char* dst, char* end);

extern char* mk_str_from_int(uint32_t x, char* buf, size_t buf_size);

#endif
