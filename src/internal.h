#ifndef MK_INTERNAL_H
#define MK_INTERNAL_H

#ifndef _MK_PRIVATE
#if defined(__GNUC__) || defined(__clang__)
#define _MK_PRIVATE __attribute__((unused)) static
#else
#define _MK_PRIVATE static
#endif
#endif

#ifndef MK_DEBUG
#ifndef NDEBUG
#define MK_DEBUG
#endif
#endif

#ifndef MK_ASSERT
#include <assert.h>
#define MK_ASSERT(c) assert(c)
#endif

#ifndef MK_LEN
#define MK_LEN(a) sizeof(a) / sizeof(a[0])
#endif

#endif
