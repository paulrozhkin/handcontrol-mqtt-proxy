/*
 * crc8.h
 *
 *  Created on: Nov 21, 2020
 *      Author: Pavel Rozhkin
 */

#ifndef INC_CRC8_H_
#define INC_CRC8_H_

#include <stdint.h>
#include <stddef.h>

uint8_t calculate_crc8(const uint8_t* data, size_t length);

#endif /* INC_CRC8_H_ */
