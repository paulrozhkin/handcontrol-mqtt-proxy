/*
 * buffer.h
 *
 *  Created on: 17 окт. 2020 г.
 *      Author: Paul Rozhkin
 */

#ifndef INC_BUFFER_H_
#define INC_BUFFER_H_

#include <stdio.h>
#include <string.h>
#include "stdbool.h"
#include "stdint.h"

#define BASE_SIZE_BUFFER 16

typedef size_t buffer_size_t; // stores the number of elements

typedef struct {
	buffer_size_t length;
	buffer_size_t capacity;
	uint8_t *data;
} buffer_t;

/**
 * @brief Init buffer.
 * @param buffer_addr pointer to initialization buffer
 * @param default_size default initialization buffer size,
 *  if default_size is 0 then BASE_SIZE_BUFFER is used
 * @retval None
 */
void buffer_init(buffer_t *buffer_addr, buffer_size_t default_size);

/**
 * @brief free allocated memory
 * @param buffer_addr pointer to buffer
 * @retval None
 */
void buffer_destroy(buffer_t *buffer_addr);

/*
 * @brief add new element to buffer
 * @param buffer_addr pointer to buffer
 * @param new_element new item to add
 * @retval None
 */
void buffer_add(buffer_t *buffer_addr, uint8_t new_item);

/*
 * @brief add new element to buffer
 * @param buffer_addr pointer to buffer
 * @param new_element new item to add
 * @retval None
 */
void buffer_add(buffer_t *buffer_addr, uint8_t new_item);

/**
* @brief Writes regions of a byte array to the current buffer
* @param buffer_addr pointer to buffer
* @param array a byte array containing the data to write
* @param array_size the number of bytes to write
* @retval None.
*/
void buffer_array_add(buffer_t* buffer_addr, uint8_t* array, int array_size);

/**
 * @brief get current length of buffer
 * @param buffer_addr pointer to buffer
 * @retval length of buffer
 */
buffer_size_t buffer_lenght(buffer_t *buffer_addr);

/**
 * @brief Internal interface. The function must be called whenever new elements are added to the buffer.
 * Provides an buffer of the required size.
 * If the current buffer size does not meet the required minimum capacity,
 * then a new, upsized buffer is dynamically allocated.
 * The new buffer deeply copies the old buffer and replaces the old buffer in the structure.
 * This clears the old buffer.
 * @param buffer_addr pointer to buffer
 * @param minCapacity the new minimum size for the buffer
 * @retval None.
 */
void buffer_internal_ensure_capacity(buffer_t *buffer_addr, int min_capacity);

/**
 * @brief Internal interface. Expands the buffer to the required length.
 * @param buffer_addr pointer to buffer
 * @param minCapacity the new minimum size for the buffer
 * @retval None.
 */
void buffer_internal_grow_buffer(buffer_t *buffer_addr, int min_capacity);

/**
 * @brief Copy and return buffer data.
 * @param buffer_addr pointer to buffer
 * @retval Data.
 */
uint8_t* buffer_get_data_copy(buffer_t* buffer_addr);

#endif /* INC_BUFFER_H_ */
