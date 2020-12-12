/*
 * protocol_parser.h
 *
 *  Created on: Dec 5, 2020
 *      Author: Paul Rozhkin
 */

#ifndef INC_PROTOCOL_PARSER_H_
#define INC_PROTOCOL_PARSER_H_

#include "stdint.h"
#include "buffer.h"
#include "crc8.h"
#include "stdbool.h"
#include "string.h"

enum ProtocolCommandType {
	PROTOCOL_COMMAND_EMPTY = 0,
	PROTOCOL_COMMAND_ACK = 1,
	PROTOCOL_COMMAND_ERR = 2,
	PROTOCOL_COMMAND_TELEMETRY = 3,
	PROTOCOL_COMMAND_GET_SETTINGS = 4,
	PROTOCOL_COMMAND_SET_SETTINGS = 5,
	PROTOCOL_COMMAND_GET_GESTURES = 6,
	PROTOCOL_COMMAND_SAVE_GESTURES = 7,
	PROTOCOL_COMMAND_DELETE_GESTURES = 8,
	PROTOCOL_COMMAND_PERFORM_GESTURE_ID = 9,
	PROTOCOL_COMMAND_PERFORM_GESTURE_RAW = 10,
	PROTOCOL_COMMAND_SET_POSITIONS = 11
};

enum ProtocolParserState {
	PROTOCOL_PARSER_SFD,
	PROTOCOL_PARSER_TYPE,
	PROTOCOL_PARSER_SIZE,
	PROTOCOL_PARSER_PAYLOAD,
	PROTOCOL_PARSER_CRC,
	PROTOCOL_PARSER_RECEIVED
};

typedef struct {
	enum ProtocolCommandType type;
	uint16_t size;
	uint8_t* payload;
	uint8_t crc;
} ProtocolPackageStruct;

typedef struct {
	uint8_t SFD[8];
	uint8_t real_crc;
	ProtocolPackageStruct current_package;
	enum ProtocolParserState state;
	buffer_t receive_buffer;
	uint32_t last_receive_timestamp;
	int sfd_receive_count;
	bool size_receive_second;
} ProtocolParserStruct;

ProtocolPackageStruct ProtocolParser_CreatePackage(enum ProtocolCommandType type, uint8_t* payload, uint16_t payload_size);

void ProtocolParser_Init(ProtocolParserStruct* parser);

int ProtocolParser_GetCommonSize(ProtocolPackageStruct* package);

void ProtocolParser_AddPackageToBuffer(ProtocolPackageStruct* package, buffer_t* buffer);

void ProtocolParser_Update(ProtocolParserStruct* parser, uint8_t data, uint32_t current_time);

void ProtocolParser_PopPackage(ProtocolParserStruct* parser, ProtocolPackageStruct* package);

#endif /* INC_PROTOCOL_PARSER_H_ */
