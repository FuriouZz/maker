#ifndef MAKER_UTIL_H
#define MAKER_UTIL_H

#include "stdarg.h"
int maker_strfmt(char *buf, int buf_size, const char *fmt, va_list args);

#endif
