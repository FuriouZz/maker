#ifndef MK_UTILS_MEM_H
#define MK_UTILS_MEM_H

#include <stddef.h>

extern void* mk_malloc(size_t size);

extern void mk_free(void* ptr);

extern void mk_clear(void* ptr, size_t size);

extern void* mk_malloc_clear(size_t size);

#endif
