#ifndef ARRAY_H
#define ARRAY_H

#include "types.h"

typedef enum {
    ARRAY_HEADER_CAPACITY = 0,
    ARRAY_HEADER_LENGTH,
    ARRAY_HEADER_STRIDE,
} Array_Header;

// Based on Array_Header
#define NUM_ARRAY_HEADERS 3

#define SIZE_ARRAY_HEADERS (NUM_ARRAY_HEADERS * sizeof(u64))

// Used in "array" macro
#define ARRAY_DEFAULT_CAPACITY 1

// Used in _array_resize function
#define ARRAY_RESIZE_FACTOR 2

#define array_header(a)         ((u64 *)(a) - NUM_ARRAY_HEADERS)
#define array_capacity(a)       (array_header((a))[ARRAY_HEADER_CAPACITY])
#define array_set_capacity(a,v) (array_header((a))[ARRAY_HEADER_CAPACITY] = (v))
#define array_length(a)         (array_header((a))[ARRAY_HEADER_LENGTH])
#define array_set_length(a,v)   (array_header((a))[ARRAY_HEADER_LENGTH] = (v))
#define array_stride(a)         (array_header((a))[ARRAY_HEADER_STRIDE])
#define array_set_stride(a,v)   (array_header((a))[ARRAY_HEADER_STRIDE] = (v))
#define array_size(a)           (SIZE_ARRAY_HEADERS + array_capacity((a)) * array_stride(a))

void *_array_create(u64 length, u64 stride);
void _array_destroy(void *array);
void *_array_resize(void *array);
void *_array_push(void *array, const void *data);
void _array_pop(void *array, void *dest);
void *_array_pop_at(void *array, u64 index, void *dest);
void *_array_insert_at(void *array, u64 index, void *data);

#define array_create(T)        _array_create(ARRAY_DEFAULT_CAPACITY, sizeof(T))
#define array_reserve(T,n)     _array_create((n), sizeof(T))
#define array_destroy(a)       _array_destroy((a))
#define array_push(a,v)                \
    do {                               \
        typeof(v) temp = (v);          \
        (a) = _array_push((a), &temp); \
    } while(0)
#define array_pop(a,ptr)       _array_pop((a), (ptr))
#define array_insert_at(a,i,v)                   \
    do {                                         \
        typeof(v) temp = (v);                    \
        (a) = _array_insert_at((a), (i), &temp); \
    } while (0)
#define array_pop_at(a,i,ptr)  _array_pop_at((a), (i), (ptr))
#define array_clear(a)         _array_field_set((a), ARRAY_HEADER_LENGTH, 0)

#endif