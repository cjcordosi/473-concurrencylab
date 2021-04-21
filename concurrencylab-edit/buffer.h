#ifndef BUFFER_H
#define BUFFER_H

#include <stdlib.h>

typedef struct {
    size_t size;
    size_t next;
    size_t capacity;
    void** data;
} buffer_t;

enum buffer_status {
    BUFFER_SUCCESS = 1,
    BUFFER_ERROR = -1
};

// Creates a buffer with the given capacity
buffer_t* buffer_create(size_t capacity);

// Adds the value into the buffer
// Returns BUFFER_SUCCESS if the buffer is not full and value was added
// Returns BUFFER_ERROR otherwise
enum buffer_status buffer_add(buffer_t* buffer, void* data);

// Removes the value from the buffer in FIFO order and stores it in data
// Returns BUFFER_SUCCESS if the buffer is not empty and a value was removed
// Returns BUFFER_ERROR otherwise
enum buffer_status buffer_remove(buffer_t* buffer, void** data);

// Frees the memory allocated to the buffer
void buffer_free(buffer_t* buffer);

// Returns the total capacity of the buffer
size_t buffer_capacity(buffer_t* buffer);

// Returns the current number of elements in the buffer
size_t buffer_current_size(buffer_t* buffer);

// Peeks at a value in the buffer
// Only used for testing code; you should NOT use this
void* peek_buffer(buffer_t* buffer, size_t index);

#endif // BUFFER_H
