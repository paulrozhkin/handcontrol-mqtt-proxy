/*
 * buffer.c
 *
 *  Created on: 17 окт. 2020 г.
 *      Author: Paul Rozhkin
 */

#include "buffer.h"

void buffer_init(buffer_t *buffer_addr, buffer_size_t default_size) {
	buffer_size_t alloc_size =
			default_size == 0 ? BASE_SIZE_BUFFER : default_size;

	buffer_addr->capacity = alloc_size;
	buffer_addr->length = 0;
	buffer_addr->data = pvPortMalloc(alloc_size);
}

void buffer_destroy(buffer_t *buffer_addr) {
	buffer_addr->capacity = 0;
	buffer_addr->length = 0;
	vPortFree(buffer_addr->data);
}

void buffer_add(buffer_t *buffer_addr, uint8_t new_item) {
	buffer_internal_ensure_capacity(buffer_addr, buffer_addr->length + 1);
	buffer_addr->data[buffer_addr->length] = new_item;
	buffer_addr->length += 1;
}

void buffer_array_add(buffer_t* buffer_addr, uint8_t* array, int array_size)
{
	buffer_internal_ensure_capacity(buffer_addr,
			buffer_addr->length + array_size);

	uint8_t* next_last_item_ptr = buffer_addr->data+buffer_addr->length;
	memcpy(next_last_item_ptr, array, array_size);
	buffer_addr->length += array_size;
}

buffer_size_t buffer_lenght(buffer_t *buffer_addr)
{
	return buffer_addr->length;
}

void buffer_internal_ensure_capacity(buffer_t *buffer_addr, int min_capacity) {
	configASSERT(buffer_addr != NULL);
	configASSERT(buffer_addr->data != NULL);

	if (buffer_addr->capacity < min_capacity) {
		buffer_internal_grow_buffer(buffer_addr, min_capacity);
	}
}

void buffer_internal_grow_buffer(buffer_t *buffer_addr, int min_capacity) {
	// overflow-conscious code
	int newCapacity = buffer_addr->capacity;

	while (1) {
		newCapacity = newCapacity + (newCapacity >> 1);

		if (newCapacity - min_capacity >= 0) {
			break;
		}
	}

	// minCapacity is usually close to size, so this is a win:
	uint8_t *oldBuffer = buffer_addr->data;
	uint8_t *newBuffer = pvPortMalloc(sizeof(uint8_t) * newCapacity);
	memcpy(newBuffer, oldBuffer, buffer_addr->length);
	buffer_addr->data = newBuffer;
	buffer_addr->capacity = newCapacity;
	vPortFree(oldBuffer);
}
