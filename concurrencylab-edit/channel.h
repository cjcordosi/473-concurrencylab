#ifndef CHANNEL_H
#define CHANNEL_H

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include "buffer.h"
#include <stddef.h> 
#include <string.h>
#include <stdbool.h>
#include "linked_list.h"

// Defines possible return values from channel functions
enum channel_status {
    CHANNEL_EMPTY = 0,
    CHANNEL_FULL = 0,
    SUCCESS = 1,
    CLOSED_ERROR = -2,
    GEN_ERROR = -1,
    DESTROY_ERROR = -3
};

// Defines channel object
typedef struct {
    // DO NOT REMOVE buffer (OR CHANGE ITS NAME) FROM THE STRUCT
    // YOU MUST USE buffer TO STORE YOUR BUFFERED CHANNEL MESSAGES
    buffer_t* buffer;

    /* ADD ANY STRUCT ENTRIES YOU NEED HERE */
    /* IMPLEMENT THIS */
    pthread_mutex_t MutexLock;
    pthread_cond_t read;
    pthread_cond_t write;
    void* data;
    unsigned long int closed;
} channel_t;

// Defines channel list structure for channel_select function
enum direction {
    SEND,
    RECV,
};
typedef struct {
    // Channel on which we want to perform operation
    channel_t* channel;
    // Specifies whether we want to receive (RECV) or send (SEND) on the channel
    enum direction dir;
    // If dir is RECV, then the message received from the channel is stored as an output in this parameter, data
    // If dir is SEND, then the message that needs to be sent is given as input in this parameter, data
    void* data;
} select_t;

// Creates a new channel with the provided size and returns it to the caller
// A 0 size indicates an unbuffered channel, whereas a positive size indicates a buffered channel
channel_t* channel_create(size_t size);

// Writes data to the given channel
// This is a blocking call i.e., the function only returns on a successful completion of send
// In case the channel is full, the function waits till the channel has space to write the new data
// Returns SUCCESS for successfully writing data to the channel,
// CLOSED_ERROR if the channel is closed, and
// GEN_ERROR on encountering any other generic error of any sort
enum channel_status channel_send(channel_t* channel, void* data);

// Reads data from the given channel and stores it in the function’s input parameter, data (Note that it is a double pointer).
// This is a blocking call i.e., the function only returns on a successful completion of receive
// In case the channel is empty, the function waits till the channel has some data to read
// Returns SUCCESS for successful retrieval of data,
// CLOSED_ERROR if the channel is closed, and
// GEN_ERROR on encountering any other generic error of any sort
enum channel_status channel_receive(channel_t* channel, void** data);

// Writes data to the given channel
// This is a non-blocking call i.e., the function simply returns if the channel is full
// Returns SUCCESS for successfully writing data to the channel,
// CHANNEL_FULL if the channel is full and the data was not added to the buffer,
// CLOSED_ERROR if the channel is closed, and
// GEN_ERROR on encountering any other generic error of any sort
enum channel_status channel_non_blocking_send(channel_t* channel, void* data);

// Reads data from the given channel and stores it in the function’s input parameter data (Note that it is a double pointer)
// This is a non-blocking call i.e., the function simply returns if the channel is empty
// Returns SUCCESS for successful retrieval of data,
// CHANNEL_EMPTY if the channel is empty and nothing was stored in data,
// CLOSED_ERROR if the channel is closed, and
// GEN_ERROR on encountering any other generic error of any sort
enum channel_status channel_non_blocking_receive(channel_t* channel, void** data);

// Closes the channel and informs all the blocking send/receive/select calls to return with CLOSED_ERROR
// Once the channel is closed, send/receive/select operations will cease to function and just return CLOSED_ERROR
// Returns SUCCESS if close is successful,
// CLOSED_ERROR if the channel is already closed, and
// GEN_ERROR in any other error case
enum channel_status channel_close(channel_t* channel);

// Frees all the memory allocated to the channel
// The caller is responsible for calling channel_close and waiting for all threads to finish their tasks before calling channel_destroy
// Returns SUCCESS if destroy is successful,
// DESTROY_ERROR if channel_destroy is called on an open channel, and
// GEN_ERROR in any other error case
enum channel_status channel_destroy(channel_t* channel);

// Takes an array of channels, channel_list, of type select_t and the array length, channel_count, as inputs
// This API iterates over the provided list and finds the set of possible channels which can be used to invoke the required operation (send or receive) specified in select_t
// If multiple options are available, it selects the first option and performs its corresponding action
// If no channel is available, the call is blocked and waits till it finds a channel which supports its required operation
// Once an operation has been successfully performed, select should set selected_index to the index of the channel that performed the operation and then return SUCCESS
// In the event that a channel is closed or encounters any error, the error should be propagated and returned through select
// Additionally, selected_index is set to the index of the channel that generated the error
enum channel_status channel_select(select_t* channel_list, size_t channel_count, size_t* selected_index);

#endif // CHANNEL_H
