#include "callback_allocator.h"
#include "x_allocator.h"

#define MAX_32_BLOCKS   20
#define MAX_128_BLOCKS  10

#define BLOCK_32_SIZE     32 + XALLOC_BLOCK_META_DATA_SIZE
#define BLOCK_128_SIZE    128 + XALLOC_BLOCK_META_DATA_SIZE

// Define individual fb_allocators
ALLOC_DEFINE(cbDataAllocator32, BLOCK_32_SIZE, MAX_32_BLOCKS)
ALLOC_DEFINE(cbDataAllocator128, BLOCK_128_SIZE, MAX_128_BLOCKS)

// An array of allocators sorted by smallest block first
static ALLOC_Allocator* allocators[] = {
    &cbDataAllocator32Obj,
    &cbDataAllocator128Obj
};

#define MAX_ALLOCATORS   (sizeof(allocators) / sizeof(allocators[0]))

static XAllocData self = { allocators, MAX_ALLOCATORS };

//----------------------------------------------------------------------------
// CBALLOC_Alloc
//----------------------------------------------------------------------------
void* CBALLOC_Alloc(size_t size)
{
    return XALLOC_Alloc(&self, size);
}

//----------------------------------------------------------------------------
// CBALLOC_Free
//----------------------------------------------------------------------------
void CBALLOC_Free(void* ptr)
{
    XALLOC_Free(ptr);
}

//----------------------------------------------------------------------------
// CBALLOC_Realloc
//----------------------------------------------------------------------------
void* CBALLOC_Realloc(void *ptr, size_t new_size)
{
    return XALLOC_Realloc(&self, ptr, new_size);
}

//----------------------------------------------------------------------------
// CBALLOC_Calloc
//----------------------------------------------------------------------------
void* CBALLOC_Calloc(size_t num, size_t size)
{
    return XALLOC_Calloc(&self, num, size);
}

