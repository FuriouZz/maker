#include "internal.h"
#include "maker/utils_str.h"
#include <stdarg.h>
#include <stdio.h>

int mk_str_fmt(char* buf, int buf_size, const char* fmt, ...)
{
    int result = -1;
    MK_ASSERT(buf);
    MK_ASSERT(buf_size);
    if (!buf || !buf_size || !fmt) {
        return 0;
    }

    va_list args;
    va_start(args, fmt);
    result = vsnprintf(buf, buf_size, fmt, args);
    va_end(args);

    return result;
}

char* mk_str_concat(const char* str, char* dst, char* end)
{
    if (str) {
        char c;
        while (((c = *str++) != 0) && (dst < (end - 1))) {
            *dst++ = c;
        }
    }
    *dst = 0;
    return dst;
}

char* mk_str_from_int(uint32_t x, char* buf, size_t buf_size)
{
    const size_t max_digits_and_null = 11;
    if (buf_size < max_digits_and_null) {
        return 0;
    }
    char* p = buf + max_digits_and_null;
    *--p = 0;
    do {
        *--p = '0' + (x % 10);
        x /= 10;
    } while (x != 0);
    return p;
}
