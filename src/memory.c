#include "memory.h"

#include <stdlib.h>
#include <string.h>

#include "log.h"

static size_t total_allocated = 0;
static size_t tagged_allocations[MAX_MEMORY_TAGS] = {0};

// Do malloc and zero out memory
void *memory_alloc(size_t size, Memory_Tag tag)
{
    void *block = malloc(size);
    if (block == NULL) {
        LOG_FATAL("Failed allocating memory, tried to allocate %llu bytes", size);
    }

    if (tag == MEMORY_TAG_UNKNOWN) {
        LOG_WARNING("Allocating memory with MEMORY_TAG_UNKNOWN\n");
    }

    total_allocated += size;
    tagged_allocations[tag] += size;

    return block;
}

void memory_zero(void *block, size_t size)
{
    memset(block, 0, size);
}

void memory_free(void *block, size_t size, Memory_Tag tag)
{
    if (tag == MEMORY_TAG_UNKNOWN) {
        LOG_WARNING("Freeing memory with MEMORY_TAG_UNKNOWN\n");
    }

    total_allocated -= size;
    tagged_allocations[tag] -= size;

    free(block);
}

void *memory_copy(void *dest, const void *src, size_t size)
{
    return memcpy(dest, src, size);
}

// Get allocated memory usage in bytes
size_t get_total_memory_usage()
{
    return total_allocated;
}

// Get allocated memory usage in bytes (by tag)
size_t get_memory_usage_by_tag(Memory_Tag tag)
{
    return tagged_allocations[tag];
}