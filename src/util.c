#include "maker_internal.h"
#include <unistd.h>

void* mk_malloc(usize size) { return malloc(size); }

void mk_free(void* ptr) { free(ptr); }

void mk_clear(void* ptr, usize size)
{
    MK_ASSERT(ptr && (size > 0));
    memset(ptr, 0, size);
}

void mk_memset(void* ptr, usize value, usize size)
{
    MK_ASSERT(ptr && (size > 0));
    memset(ptr, value, size);
}

void* mk_malloc_clear(usize size)
{
    void* ptr = mk_malloc(size);
    mk_clear(ptr, size);
    return ptr;
}

void* mk_realloc(void* ptr, usize size)
{
    return realloc(ptr, size);
}

i32 mk_file_exists(char* filename) { return access(filename, F_OK); }
