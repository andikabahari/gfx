#include "memory.h"

#include <stdlib.h>
#include <string.h>

#include "log.h"

static u64 total_allocated = 0;
static u64 tagged_allocations[MEMORY_TAG_MAX_TAGS] = {0};

// Do malloc and zero out memory
void *memory_alloc(u64 size, Memory_Tag tag)
{
    void *block = malloc(size);
    if (block == NULL) {
        LOG_ERROR("Failed allocating memory, tried to allocate %zu bytes", size);
        exit(1);
    }
    memset(block, 0, size);

    if (tag == MEMORY_TAG_UNKNOWN) {
        LOG_WARNING("Allocating memory with MEMORY_TAG_UNKNOWN\n");
    }

    total_allocated += size;
    tagged_allocations[tag] += size;

    return block;
}

void memory_free(void *block, u64 size, Memory_Tag tag)
{
    if (tag == MEMORY_TAG_UNKNOWN) {
        LOG_WARNING("Freeing memory with MEMORY_TAG_UNKNOWN\n");
    }

    total_allocated -= size;
    tagged_allocations[tag] -= size;

    free(block);
}

void *memory_copy(void *dest, const void *source, u64 size)
{
    return memcpy(dest, source, size);
}

// Get allocated memory usage in bytes
u64 get_total_memory_usage()
{
    return total_allocated;
}

// Get allocated memory usage in bytes (by tag)
u64 get_memory_usage_by_tag(Memory_Tag tag)
{
    return tagged_allocations[tag];
}