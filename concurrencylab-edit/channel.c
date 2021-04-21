#include "channel.h"

// Creates a new channel with the provided size and returns it to the caller
// A 0 size indicates an unbuffered channel, whereas a positive size indicates a buffered channel
channel_t* channel_create(size_t size)
{
    channel_t* channel = (channel_t*)malloc(sizeof(channel_t));
	channel->buffer = buffer_create(size);

    // Set the channel to be open
	channel->closed = 1;

    // Initialize the conditional variables aswell as the mutex so we can lock/unlock
	pthread_mutex_init(&channel->MutexLock, NULL);
    pthread_cond_init(&channel->read, NULL);
    pthread_cond_init(&channel->write, NULL);

	return channel;
}

// Writes data to the given channel
// This is a blocking call i.e., the function only returns on a successful completion of send
// In case the channel is full, the function waits till the channel has space to write the new data
// Returns SUCCESS for successfully writing data to the channel,
// CLOSED_ERROR if the channel is closed, and
// GEN_ERROR on encountering any other generic error of any sort
enum channel_status channel_send(channel_t *channel, void* data)
{
    // Lock so memmory cannot overlap
	pthread_mutex_lock(&channel->MutexLock);
	if(channel->closed == 0){
		pthread_mutex_unlock(&channel->MutexLock);
		return CLOSED_ERROR;
	}
	else{
        // Add to the buffer until it has reached its full state
		while(buffer_add(channel->buffer, data) == BUFFER_ERROR){
			pthread_cond_wait(&channel->read, &channel->MutexLock);
			if(channel->closed == 0){
				pthread_mutex_unlock(&channel->MutexLock);
				return CLOSED_ERROR;
			}
		}
        // Signal to recieve that something new is in the buffer, and unlock to allow other threads to continue
		pthread_cond_signal(&channel->write);
		pthread_mutex_unlock(&channel->MutexLock);
		return SUCCESS;
	}
    return GEN_ERROR;
}

// Reads data from the given channel and stores it in the function’s input parameter, data (Note that it is a double pointer).
// This is a blocking call i.e., the function only returns on a successful completion of receive
// In case the channel is empty, the function waits till the channel has some data to read
// Returns SUCCESS for successful retrieval of data,
// CLOSED_ERROR if the channel is closed, and
// GEN_ERROR on encountering any other generic error of any sort
enum channel_status channel_receive(channel_t* channel, void** data)
{
    // Lock so memmory cannot overlap
	pthread_mutex_lock(&channel->MutexLock);
	if(channel->closed == 0){
		pthread_mutex_unlock(&channel->MutexLock);
		return CLOSED_ERROR;
	}
	else{
        // Remove from the buffer until it has reached its empty state
		while(buffer_remove(channel->buffer, data) == BUFFER_ERROR){
			pthread_cond_wait(&channel->write, &channel->MutexLock);
			if(channel->closed == 0){
				pthread_mutex_unlock(&channel->MutexLock);
				return CLOSED_ERROR;
			}
		}
        // Signal to send that something is no longer in the buffer, and unlock to allow other threads to continue
        pthread_cond_signal(&channel->read);
		pthread_mutex_unlock(&channel->MutexLock);
		return SUCCESS;
	}
}

// Writes data to the given channel
// This is a non-blocking call i.e., the function simply returns if the channel is full
// Returns SUCCESS for successfully writing data to the channel,
// CHANNEL_FULL if the channel is full and the data was not added to the buffer,
// CLOSED_ERROR if the channel is closed, and
// GEN_ERROR on encountering any other generic error of any sort
enum channel_status channel_non_blocking_send(channel_t* channel, void* data)
{
    // If the channel does not exist return a GEN_ERROR
    if(!channel){
        // pthread_cond_signal(&channel->write);
		return GEN_ERROR;
	}
    // Lock the memory and check if the channel is closed, and if so unlock and return CLOSED_ERROR
	pthread_mutex_lock(&channel->MutexLock);
	if(channel->closed == 0){
		pthread_mutex_unlock(&channel->MutexLock);	
		return CLOSED_ERROR;
	}
    // If there is no space in the buffer to add, return CHANNEL_FULL
	if(buffer_current_size(channel->buffer) >= buffer_capacity(channel->buffer)){
		pthread_mutex_unlock(&channel->MutexLock);
		return CHANNEL_FULL;
	}
    // Add to the buffer, unlock, and signal to recieve that something was added to buffer
	else{
		buffer_add(channel->buffer, data);
        pthread_cond_signal(&channel->write);
		pthread_mutex_unlock(&channel->MutexLock);
		return SUCCESS;		
	}
    return SUCCESS;
}

// Reads data from the given channel and stores it in the function’s input parameter data (Note that it is a double pointer)
// This is a non-blocking call i.e., the function simply returns if the channel is empty
// Returns SUCCESS for successful retrieval of data,
// CHANNEL_EMPTY if the channel is empty and nothing was stored in data,
// CLOSED_ERROR if the channel is closed, and
// GEN_ERROR on encountering any other generic error of any sort
enum channel_status channel_non_blocking_receive(channel_t* channel, void** data)
{
    // If the channel does not exist return a GEN_ERROR
    if(!channel){
	    return GEN_ERROR;
    }
    // Lock the memory and check if the channel is closed, and if so unlock and return CLOSED_ERROR
	pthread_mutex_lock(&channel->MutexLock);
	if(channel->closed == 0){
		pthread_mutex_unlock(&channel->MutexLock);
		return CLOSED_ERROR;
	}
    // If there is nothing in the buffer to remove, return CHANNEL_EMPTY
	if(buffer_current_size(channel->buffer)  == 0){
		pthread_mutex_unlock(&channel->MutexLock);	
		return CHANNEL_EMPTY;
	}
    // Remove from the buffer, unlock, and signal to send that something was removed from buffer
	else{
		buffer_remove(channel->buffer, data);
        pthread_cond_signal(&channel->read);
		pthread_mutex_unlock(&channel->MutexLock);
		return SUCCESS;
	}
    return SUCCESS;
}

// Closes the channel and informs all the blocking send/receive/select calls to return with CLOSED_ERROR
// Once the channel is closed, send/receive/select operations will cease to function and just return CLOSED_ERROR
// Returns SUCCESS if close is successful,
// CLOSED_ERROR if the channel is already closed, and
// GEN_ERROR in any other error case
enum channel_status channel_close(channel_t* channel)
{
    // If the channel does not exist, return GEN_ERROR
	if(!channel){
		return CLOSED_ERROR;
    }

    // Lock the memory, and check if the channel is already closed
    pthread_mutex_lock(&channel->MutexLock);
    if(channel->closed == 0){
        pthread_mutex_unlock(&channel->MutexLock);
		return CLOSED_ERROR;
	}

    // Close the channel
	channel->closed = 0;
    
    // Wake all sleeping threads and unlock the memory
	pthread_cond_broadcast(&channel->write);
	pthread_cond_broadcast(&channel->read);
    pthread_mutex_unlock(&channel->MutexLock);

	return SUCCESS;
}

// Frees all the memory allocated to the channel
// The caller is responsible for calling channel_close and waiting for all threads to finish their tasks before calling channel_destroy
// Returns SUCCESS if destroy is successful,
// DESTROY_ERROR if channel_destroy is called on an open channel, and
// GEN_ERROR in any other error case
enum channel_status channel_destroy(channel_t* channel)
{
    // If the channel is open return a DESTROY_ERROR
    if(channel->closed == 1){
		return DESTROY_ERROR;
	}

    // Destroy the lock and conditional variables
	pthread_mutex_destroy(&channel->MutexLock);
    pthread_cond_destroy(&channel->read);
    pthread_cond_destroy(&channel->write);

    // Free the buffer and channel from memory
    buffer_free(channel->buffer);
	free(channel);

    return SUCCESS;
}

// Takes an array of channels, channel_list, of type select_t and the array length, channel_count, as inputs
// This API iterates over the provided list and finds the set of possible channels which can be used to invoke the required operation (send or receive) specified in select_t
// If multiple options are available, it selects the first option and performs its corresponding action
// If no channel is available, the call is blocked and waits till it finds a channel which supports its required operation
// Once an operation has been successfully performed, select should set selected_index to the index of the channel that performed the operation and then return SUCCESS
// In the event that a channel is closed or encounters any error, the error should be propagated and returned through select
// Additionally, selected_index is set to the index of the channel that generated the error
enum channel_status channel_select(select_t* channel_list, size_t channel_count, size_t* selected_index)
{
    /* IMPLEMENT THIS */
    return SUCCESS;
}
