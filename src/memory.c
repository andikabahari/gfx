#include "memory.h"

#include <stdlib.h>
#include <string.h>

#include "log.h"

static size_t total_allocated = 0;
static size_t tagged_allocations[MEMORY_TAG_MAX_TAGS] = {0};

// Do malloc and zero out memory
void *memory_alloc(size_t size, Memory_Tag tag)
{
    void *block = malloc(size);
    if (block == NULL) {
        LOG_ERROR("Failed allocating memory, tried to allocate %zu bytes", size);
        exit(1);
    }
    memset(block, 0, size);

    if (tag == MEMORY_TAG_UNKNOWN) {
        LOG_WARNING("%s\n", "Allocating memory with MEMORY_TAG_UNKNOWN");
    }

    total_allocated += size;
    tagged_allocations[tag] += size;

    return block;
}

void memory_free(void *block, size_t size, Memory_Tag tag)
{
    if (tag == MEMORY_TAG_UNKNOWN) {
        LOG_WARNING("%s\n", "Freeing memory with MEMORY_TAG_UNKNOWN");
    }

    total_allocated -= size;
    tagged_allocations[tag] -= size;

    free(block);
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