#include "internal.h"
#include "maker/utils_mem.h"
#include <stdlib.h>
#include <string.h>

void* mk_malloc(size_t size) { return malloc(size); }

void mk_free(void* ptr) { free(ptr); }

void mk_clear(void* ptr, size_t size)
{
    MK_ASSERT(ptr && (size > 0));
    memset(ptr, 0, size);
}

void* mk_malloc_clear(size_t size)
{
    void* ptr = mk_malloc(size);
    mk_clear(ptr, size);
    return ptr;
}
