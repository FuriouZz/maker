#include "stdio.h"

#include "maker_internal.h"
#include "maker_util.h"

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
