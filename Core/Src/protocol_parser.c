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
	//memcpy(package.SFD, sfd_default, sizeof(package.SFD));
	package.type = type;
	package.size = payload_size;
	package.payload = payload;
	package.crc = calculate_crc8(((uint8_t*)&package) + 8, 3 + payload_size, 0);
	return package;
}

void ProtocolParser_AddPackageToBuffer(ProtocolPackageStruct* package, buffer_t* buffer)
{
	buffer_array_add(buffer, sfd_default, 8);
	buffer_add(buffer, package->type);
	buffer_add(buffer, package->size & 0xFF);
	buffer_add(buffer, (package->size >> 8) & 0xFF);
	if (package->size != 0)
	{
		buffer_array_add(buffer, package->payload, package->size);
	}

	buffer_add(buffer, package->crc);
}

int ProtocolParser_GetCommonSize(ProtocolPackageStruct* package)
{
	return 12 + package->size;
}

void ProtocolParser_Update(ProtocolParserStruct* parser, uint8_t data, uint32_t current_time)
{
	if (parser->state == PROTOCOL_PARSER_RECEIVED)
	{
		return;
	}

    if (parser->state != PROTOCOL_PARSER_SFD && parser->state != PROTOCOL_PARSER_CRC)
    {
        parser->real_crc = calculate_crc8(&data, 1, parser->real_crc);
    }

	switch (parser->state) {
		case PROTOCOL_PARSER_SFD:
		{
			parser->SFD[parser->sfd_receive_count] = data;
			parser->sfd_receive_count++;

			if (parser->sfd_receive_count == 8)
			{
				if (memcmp( sfd_default, parser->SFD, 8) == 0)
				{
					parser->current_package.size = 0;
					parser->size_receive_second = false;
					parser->real_crc = 0;
					buffer_init(&parser->receive_buffer, 0);
					parser->state = PROTOCOL_PARSER_TYPE;
				}
				else
				{
					parser->sfd_receive_count--;
					uint8_t tmp_sfd_with_offset[8];
					memmove(tmp_sfd_with_offset, (uint8_t*)(&parser->SFD) + 1, 7);
					tmp_sfd_with_offset[7] = 0;
					memmove(parser->SFD, tmp_sfd_with_offset, 8);
				}
			}

			break;
		}
		case PROTOCOL_PARSER_TYPE:
		{
			parser->current_package.type = data;
			parser->state = PROTOCOL_PARSER_SIZE;
			break;
		}
		case PROTOCOL_PARSER_SIZE:
		{
			if (parser->size_receive_second == false)
			{
				parser->current_package.size = data;
				parser->size_receive_second = true;
			}
			else
			{
				parser->current_package.size = (data << 8) | parser->current_package.size;
				parser->size_receive_second = false;

                if (parser->current_package.size == 0)
                {
                	parser->state = PROTOCOL_PARSER_CRC;
                }
                else
                {
                	parser->state = PROTOCOL_PARSER_PAYLOAD;
                }
			}

			break;
		}
		case PROTOCOL_PARSER_PAYLOAD:
		{
			buffer_add(&parser->receive_buffer, data);

			if (buffer_lenght(&parser->receive_buffer) == parser->current_package.size)
			{
				parser->state = PROTOCOL_PARSER_CRC;
			}

			break;
		}
		case PROTOCOL_PARSER_CRC:
		{
			parser->current_package.crc = data;
			if (parser->real_crc == parser->current_package.crc)
			{
				parser->state = PROTOCOL_PARSER_RECEIVED;
			}

			parser->sfd_receive_count = 0;
			parser->size_receive_second = false;
			memset(parser->SFD, 0, 8);
			break;
		}
		default:
		{
			return;
		}
	}
}

void ProtocolParser_Init(ProtocolParserStruct* parser)
{
	memset(parser, 0, sizeof(ProtocolParserStruct));
	parser->state = PROTOCOL_PARSER_SFD;
}

void ProtocolParser_PopPackage(ProtocolParserStruct* parser, ProtocolPackageStruct* package)
{
	if (parser->state != PROTOCOL_PARSER_RECEIVED)
	{
		return;
	}

	package->type = parser->current_package.type;
	package->size = parser->current_package.size;
	package->crc = parser->current_package.crc;
	package->payload = buffer_get_data_copy(&parser->receive_buffer);

	buffer_destroy(&parser->receive_buffer);
	parser->state = PROTOCOL_PARSER_SFD;
}
