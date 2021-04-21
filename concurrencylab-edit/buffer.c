#include "buffer.h"

// Creates a buffer with the given capacity
buffer_t* buffer_create(size_t capacity)
{
    buffer_t* buffer = (buffer_t*) malloc(sizeof(buffer_t));
    void** data  = (void**) malloc(capacity * sizeof(void*));
    buffer->size = 0;
    buffer->next = 0;
    buffer->capacity = capacity;
    buffer->data = data;
    return buffer;
}

// Adds the value into the buffer
// Returns BUFFER_SUCCESS if the buffer is not full and value was added
// Returns BUFFER_ERROR otherwise
enum buffer_status buffer_add(buffer_t* buffer, void* data)
{
    if (buffer->size >= buffer->capacity) {
        return BUFFER_ERROR;
    }
    size_t pos = buffer->next + buffer->size;
    if (pos >= buffer->capacity) {
        pos -= buffer->capacity;
    }
    buffer->data[pos] = data;
    buffer->size++;
    return BUFFER_SUCCESS;
}

// Removes the value from the buffer in FIFO order and stores it in data
// Returns BUFFER_SUCCESS if the buffer is not empty and a value was removed
// Returns BUFFER_ERROR otherwise
enum buffer_status buffer_remove(buffer_t* buffer, void **data)
{
    if (buffer->size > 0) {
        *data = buffer->data[buffer->next];
        buffer->size--;
        buffer->next++;
        if (buffer->next >= buffer->capacity) {
            buffer->next -= buffer->capacity;
        }
        return BUFFER_SUCCESS;
    }
    return BUFFER_ERROR;
}

// Frees the memory allocated to the buffer
void buffer_free(buffer_t *buffer)
{
    free(buffer->data);
    free(buffer);
}

// Returns the total capacity of the buffer
size_t buffer_capacity(buffer_t* buffer)
{
    return buffer->capacity;
}

// Returns the current number of elements in the buffer
size_t buffer_current_size(buffer_t* buffer)
{
    return buffer->size;
}

// Peeks at a value in the buffer
// Only used for testing code; you should NOT use this
void* peek_buffer(buffer_t* buffer, size_t index)
{
    return buffer->data[index];
}
