#include "maker_internal.h"

void* maker_malloc(usize size) { return malloc(size); }

void maker_free(void* ptr) { free(ptr); }

void maker_clear(void* ptr, usize size)
{
    MAKER_ASSERT(ptr && (size > 0));
    memset(ptr, 0, size);
}

void maker_memset(void* ptr, usize value, usize size)
{
    MAKER_ASSERT(ptr && (size > 0));
    memset(ptr, value, size);
}

void* maker_malloc_clear(usize size)
{
    void* ptr = maker_malloc(size);
    if (ptr != NULL) {
        maker_clear(ptr, size);
    }
    return ptr;
}

void* maker_realloc(void* ptr, usize size)
{
    return realloc(ptr, size);
}
