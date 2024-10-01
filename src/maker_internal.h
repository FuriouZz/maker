#ifndef MAKER_INTERNAL_H
#define MAKER_INTERNAL_H

#ifndef _MAKER_PRIVATE
#if defined(__GNUC__) || defined(__clang__)
#define _MAKER_PRIVATE __attribute__((unused)) static
#else
#define _MAKER_PRIVATE static
#endif
#endif

#ifndef MAKER_DEBUG
#ifndef NDEBUG
#define MAKER_DEBUG
#endif
#endif

#ifndef MAKER_ASSERT
#include <assert.h>
#define MAKER_ASSERT(c) assert(c)
#endif

#ifndef MAKER_LEN
#define MAKER_LEN(a) sizeof(a) / sizeof(a[0])
#endif

#endif
