#ifndef MEMORY_H
#define MEMORY_H

#include "common.h"

typedef enum {
    MEMORY_TAG_UNKNOWN = 0,
    MEMORY_TAG_ARRAY,

    MAX_MEMORY_TAGS
} Memory_Tag;

void *memory_alloc(size_t size, Memory_Tag tag);
void memory_zero(void *block, size_t size);
void memory_free(void *block, size_t size, Memory_Tag tag);
void *memory_copy(void *dest, const void *src, size_t size);

size_t get_total_memory_usage();
size_t get_memory_usage_by_tag(Memory_Tag tag);

#endif