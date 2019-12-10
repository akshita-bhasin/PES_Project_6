/*
 * circularbuffer.c
 *
 *  Created on: Nov 11, 2019
 *      Author: madhu
 *      https://embeddedartistry.com/blog/2017/05/17/creating-a-circular-buffer-in-c-and-c/
 */

#include "circularbuffer.h"

cbuf_handle_t circular_buf_init(size_t size)
{
	uint8_t* buffer_init;
	cbuf_handle_t cbuf = (circ_bbuf_t*)malloc(sizeof(circ_bbuf_t));
	buffer_init = (uint16_t*)malloc(sizeof(uint16_t)*size);
	cbuf->buffer = buffer_init;
	cbuf->max = size;
	cbuf->head = buffer_init;
	cbuf->tail = buffer_init;
	cbuf->count = 0;
	cbuf->full = false;

	return cbuf;
}

void circular_buf_reset(cbuf_handle_t cbuf)
{

    cbuf->head = cbuf->buffer;
    cbuf->tail = cbuf->buffer;
    cbuf->count = 0;
    cbuf->full = false;
}

buffer_errors circular_buf_free(cbuf_handle_t cbuf)
{
	free(cbuf->buffer);
	free(cbuf);
	return buffer_freed;
}

buffer_errors circular_buf_full(cbuf_handle_t cbuf)
{

	if(cbuf == NULL)
		return buffer_null;
	else if(cbuf->count == cbuf->max)
		return buffer_full;
	else
		return buffer_not_full;
}

buffer_errors circular_buf_empty(cbuf_handle_t cbuf)
{
	if(cbuf == NULL)
		return buffer_null;
	else if((cbuf->count) == 0)
		return buffer_empty;
	else
		return buffer_not_empty;
}

buffer_errors circular_buf_initialized(cbuf_handle_t cbuf)
{
	if(cbuf->buffer && cbuf->head && cbuf->tail && (cbuf->count == 0))
		return buffer_init_success;
	else
		return buffer_init_fail;
}

buffer_errors circular_buf_valid(cbuf_handle_t cbuf)
{
	if(cbuf->buffer == NULL)
		return buffer_invalid;
	else
		return buffer_valid;
}
size_t circular_buf_capacity(cbuf_handle_t cbuf)
{

	return cbuf->max;
}

buffer_errors circular_buf_size(cbuf_handle_t cbuf)
{
    if(cbuf == NULL)
    	return buffer_null;
    else if(circular_buf_full(cbuf) == buffer_full)//cbuf->count == cbuf->max)
    		return buffer_full;
    else if(cbuf->head >= cbuf->tail)
    {
    	cbuf->max = (cbuf->head - cbuf->tail);
    	return buffer_success;
    }
    else
    {
    	cbuf->max = (cbuf->max + cbuf->head - cbuf->tail);
    	return buffer_success;
    }
}

buffer_errors circular_buf_put2(cbuf_handle_t cbuf, uint16_t data) //push
{

    if(cbuf == NULL)
    	return buffer_null;
    else if(circular_buf_full(cbuf) == buffer_full)//cbuf->count == cbuf->max)
    		return buffer_full;
    else if(cbuf->head >= ((cbuf->buffer)+(cbuf->max)))
    {
    	*(cbuf->head) = data;
    	cbuf->head = cbuf->buffer;
    	cbuf->count++;
    	return buffer_success;
    }
    else
    {
    	*(cbuf->head) = data;
    	cbuf->head++;
    	cbuf->count++;
    	return buffer_success;
    }

}

buffer_errors circular_buf_get(cbuf_handle_t cbuf, uint16_t * data) //pop
{

    if(cbuf == NULL)
    	return buffer_null;
    else if(circular_buf_empty(cbuf) == buffer_empty)//cbuf->count == cbuf->max)
    		return buffer_empty;
    else if(cbuf->tail >= ((cbuf->buffer)+(cbuf->max)))
    {
    	*data = *(cbuf->tail);
    	cbuf->tail = cbuf->buffer;
    	cbuf->count--;
    	return buffer_success;
    }
    else
    {
    	*data = *(cbuf->tail);
    	cbuf->tail++;
    	cbuf->count--;
    	return buffer_success;
    }
}

buffer_errors circular_buffer_realloc(cbuf_handle_t cbuf, size_t newSize)
{
	uint8_t data;
	if(circular_buf_full(cbuf) == buffer_full)
	{
		cbuf_handle_t buffer_realloc = circular_buf_init(newSize);
		while(circular_buf_get(cbuf, &data) == buffer_success)
			circular_buf_put2(buffer_realloc, data);

		circular_buf_free(cbuf);
		cbuf = buffer_realloc;

		return buffer_realloc_success;
	}

	log_string("Realloc not done, cause buffer is not full yet");

	return buffer_realloc_fail;

}


