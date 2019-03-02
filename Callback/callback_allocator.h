// The callback_allocator module is a fixed block memory allocator that 
// allocates/deallocates memory for callback data to travel through an 
// OS task queue. 

#ifndef _CALLBACK_ALLOCATOR_H
#define _CALLBACK_ALLOCATOR_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void* CBALLOC_Alloc(size_t size);
void CBALLOC_Free(void* ptr);
void* CBALLOC_Realloc(void *ptr, size_t new_size);
void* CBALLOC_Calloc(size_t num, size_t size);

#ifdef __cplusplus
}
#endif

#endif
