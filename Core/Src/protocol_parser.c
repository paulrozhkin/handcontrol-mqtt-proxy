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

void ProtocolParser_AddPackageToBuffer(ProtocolPackageStruct* package, buffer_t* buffer)
{
	buffer_array_add(buffer, package->SFD, sizeof(package->SFD));
	buffer_add(buffer, package->type);
	buffer_add(buffer, package->size & 0xFF);
	buffer_add(buffer, (package->size >> 8) & 0xFF);
	buffer_array_add(buffer, package->payload, package->size);
	buffer_add(buffer, package->crc8);
}

int ProtocolParser_GetCommonSize(ProtocolPackageStruct* package)
{
	return 12 + package->size;
}
