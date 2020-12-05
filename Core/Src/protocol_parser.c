/*
 * protocol_parser.c
 *
 *  Created on: Dec 5, 2020
 *      Author: Paul Rozhkin
 */

#include "protocol_parser.h"
#include "string.h"
#include "crc8.h"

static uint8_t sfd_default[8] = {0xFD, 0xBA, 0xDC, 0x01, 0x50, 0xB4, 0x11, 0xFF};

ProtocolPackageStruct ProtocolParser_CreatePackage(enum ProtocolCommandType type, uint8_t* payload, uint16_t payload_size)
{
	ProtocolPackageStruct package;
	memcpy(package.SFD, sfd_default, sizeof(package.SFD));
	package.type = type;
	package.size = payload_size;
	package.payload = payload;
	package.crc8 = calculate_crc8(((uint8_t*)&package) + 8, 3 + payload_size);
	return package;
}


int ProtocolParser_GetCommonSize(ProtocolPackageStruct* package)
{
	return sizeof(ProtocolPackageStruct) + package->size - 1;
}
