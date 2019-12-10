/*
 * circularbuffer.h
 *
 *  Created on: Nov 11, 2019
 *      Author: madhu
 */


#ifndef CIRCULARBUFFER_H_
#define CIRCULARBUFFER_H_

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include "logger.h"

typedef struct {
	uint8_t* buffer;
	uint8_t * head;
	uint8_t * tail;
	uint8_t max; //of the buffer
	size_t count;
	bool full;
}circ_bbuf_t;

typedef enum
{
    buffer_init_success=0,
    buffer_init_fail,
    buffer_empty,
    buffer_not_empty,
    buffer_full,
	buffer_not_full,
	buffer_null,
    buffer_success,
	buffer_freed,
	buffer_valid,
	buffer_invalid,
	buffer_realloc_success,
	buffer_realloc_fail
}buffer_errors;

//typedef struct circ_bbuf_t circ_bbuf_t;

typedef circ_bbuf_t* cbuf_handle_t;


/// Pass in a storage buffer and size
/// Returns a circular buffer handle
cbuf_handle_t circular_buf_init(size_t size);

/// Free a circular buffer structure.
/// Does not free data buffer; owner is responsible for that
buffer_errors circular_buf_free(cbuf_handle_t cbuf);

/// Reset the circular buffer to empty, head == tail
void circular_buf_reset(cbuf_handle_t cbuf);

/// Put version 1 continues to add data if the buffer is full
/// Old data is overwritten
//void circular_buf_put(cbuf_handle_t cbuf, uint8_t data);

/// Put Version 2 rejects new data if the buffer is full
/// Returns 0 on success, -1 if buffer is full
buffer_errors circular_buf_put2(cbuf_handle_t cbuf, uint8_t data);

/// Retrieve a value from the buffer
/// Returns 0 on success, -1 if the buffer is empty
buffer_errors circular_buf_get(cbuf_handle_t cbuf, uint8_t * data);

/// Returns true if the buffer is empty
buffer_errors circular_buf_empty(cbuf_handle_t cbuf);

/// Returns true if the buffer is full
buffer_errors circular_buf_full(cbuf_handle_t cbuf);

/// Returns the maximum capacity of the buffer
size_t circular_buf_capacity(cbuf_handle_t cbuf);

/// Returns the current number of elements in the buffer
buffer_errors circular_buf_size(cbuf_handle_t cbuf);

buffer_errors circular_buf_initialized(cbuf_handle_t cbuf);

buffer_errors circular_buf_valid(cbuf_handle_t cbuf);

buffer_errors circular_buffer_realloc(cbuf_handle_t cbuf, size_t newSize);


#endif /* CIRCULARBUFFER_H_ */
