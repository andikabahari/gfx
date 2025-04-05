#ifndef MEMORY_H
#define MEMORY_H

typedef enum {
    MEMORY_TAG_UNKNOWN,
    MEMORY_TAG_ARRAY,

    MEMORY_TAG_MAX_TAGS
} Memory_Tag;

void *memory_alloc(size_t size, Memory_Tag tag);
void memory_free(void *block, size_t size, Memory_Tag tag);

size_t get_total_memory_usage();
size_t get_memory_usage_by_tag(Memory_Tag tag);

#endif