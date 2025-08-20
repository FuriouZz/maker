#ifndef MK_UTIL_H
#define MK_UTIL_H

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

#define MK_CHECK_VALID(ptr)                                                    \
    if (ptr == NULL) {                                                         \
        return -1;                                                             \
    }

#define MK_CHECK_RESULT(res)                                                   \
    if (res != 0) {                                                            \
        return -1;                                                             \
    }

#include <stddef.h>

extern void* mk_malloc(size_t size);

extern void mk_free(void* ptr);

extern void mk_clear(void* ptr, size_t size);

extern void mk_memset(void* ptr, size_t value, size_t size);

extern void* mk_malloc_clear(size_t size);

extern int mk_file_exists(char* filename);

#endif
