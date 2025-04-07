#ifndef MEMORY_H
#define MEMORY_H

#include "types.h"

typedef enum {
    MEMORY_TAG_UNKNOWN,
    MEMORY_TAG_ARRAY,

    MEMORY_TAG_MAX_TAGS
} Memory_Tag;

void *memory_alloc(u64 size, Memory_Tag tag);
void memory_free(void *block, u64 size, Memory_Tag tag);
void *memory_copy(void *dest, const void *source, u64 size);

u64 get_total_memory_usage();
u64 get_memory_usage_by_tag(Memory_Tag tag);

#endif