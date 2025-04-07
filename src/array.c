#include "array.h"

#include "memory.h"
#include "log.h"

void *_array_create(u64 length, u64 stride)
{
    u64 size = SIZE_ARRAY_HEADERS + length * stride;
    u64 *array = memory_alloc(size, MEMORY_TAG_ARRAY);
    array[ARRAY_HEADER_CAPACITY] = length;
    array[ARRAY_HEADER_LENGTH] = 0;
    array[ARRAY_HEADER_STRIDE] = stride;

    return (void *)(array + NUM_ARRAY_HEADERS);
}

void _array_destroy(void *array)
{
    memory_free(array_header(array), array_size(array), MEMORY_TAG_ARRAY);
}

void *_array_resize(void *array)
{
    u64 length = array_length(array);
    u64 stride = array_stride(array);
    void *new_array = _array_create(ARRAY_RESIZE_FACTOR * array_capacity(array), stride);
    memory_copy(new_array, array, length * stride);
    
    array_set_length(new_array, length);
    
    _array_destroy(array);
    return new_array;
}

void *_array_push(void *array, const void *data)
{
    u64 length = array_length(array);
    u64 stride = array_stride(array);
    
    if (length >= array_capacity(array)) {
        array = _array_resize(array);
    }
    
    u64 addr = (u64)array;
    addr += (length * stride);
    memory_copy((void *)addr, data, stride);

    array_set_length(array, length + 1);

    return array;
}

void _array_pop(void *array, void *dest)
{
    u64 length = array_length(array);
    u64 stride = array_stride(array);
    u64 addr = (u64)array;
    addr += ((length - 1) * stride);
    memory_copy(dest, (void *)addr, stride);

    array_set_length(array, length - 1);
}

void *_array_pop_at(void *array, u64 index, void *dest)
{
    u64 length = array_length(array);
    u64 stride = array_stride(array);

    if (index >= length) {
        LOG_ERROR("Array index out of bounds. Length: %llu, index: %llu", length, index);
        return array;
    }

    u64 addr = (u64)array;
    memory_copy(dest, (void *)(addr + (index * stride)), stride);

    // If not on the last element, snip out the entry and copy the rest inward.
    if (index != length - 1) {
        memory_copy(
            (void *)(addr + (index * stride)),
            (void *)(addr + ((index + 1) * stride)),
            stride * (length - index));
    }

    array_set_length(array, length - 1);

    return array;
}

void *_array_insert_at(void *array, u64 index, void *data)
{
    u64 length = array_length(array);
    u64 stride = array_stride(array);

    if (index >= length) {
        LOG_ERROR("Array index out of bounds. Length: %llu, index: %llu", length, index);
        return array;
    }

    if (length >= array_capacity(array)) {
        array = _array_resize(array);
    }

    u64 addr = (u64)array;

    // If not on the last element, copy the rest outward.
    if (index != length - 1) {
        memory_copy(
            (void *)(addr + ((index + 1) * stride)),
            (void *)(addr + (index * stride)),
            stride * (length - index));
    }

    memory_copy((void *)(addr + (index * stride)), data, stride);

    array_set_length(array, length + 1);

    return array;
}