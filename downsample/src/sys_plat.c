
#include <stdio.h>
#include <stdlib.h>
//#include <malloc.h>

#include "sys_plat.h"

int32_t wk_total_size = 0;

void* wk_malloc(size_t size, const char *file, int32_t line)
{
    char *ptr = (char*)malloc(size + sizeof(int32_t));
    *((int32_t*)ptr) = size;
    ptr += sizeof(int32_t);

    wk_total_size += size;
    //fprintf(stdout, "malloc %p(%d) in %s:%d total:%d\n", ptr, (int32_t)size, file, line, wk_total_size);    

    return (void*)ptr;
} 

void wk_free(void *ptr, const char *file, int32_t line)
{
    char *ptr_tmp = (char*)ptr - sizeof(int32_t);

    wk_total_size -= *((int32_t*)ptr_tmp);
    //fprintf(stdout, "free %p(%d) in %s:%d total:%d\n", ptr, *((int32_t*)ptr_tmp),file, line, wk_total_size);

    free(ptr_tmp);
}
