#include "unity.h"
#include "unity_fixture.h"
#include "fff.h"
#include "protocol_parser.h"

DEFINE_FFF_GLOBALS;
// FAKE_VALUE_FUNC(FeedbackUnit, ActuatorController_GetFeedback, ActuatorStruct *);
FAKE_VOID_FUNC(buffer_init, buffer_t *, buffer_size_t);
FAKE_VOID_FUNC(buffer_add, buffer_t *, uint8_t);
FAKE_VOID_FUNC(buffer_destroy, buffer_t *);
FAKE_VOID_FUNC(buffer_array_add, buffer_t *, uint8_t*, int);
FAKE_VALUE_FUNC(buffer_size_t, buffer_lenght, buffer_t *);
FAKE_VALUE_FUNC(uint8_t*, buffer_get_data_copy, buffer_t*);

TEST_GROUP(ProtocolParser);

TEST_SETUP(ProtocolParser)
{
	RESET_FAKE(buffer_init);
	RESET_FAKE(buffer_add);
	RESET_FAKE(buffer_array_add);
	RESET_FAKE(buffer_lenght);
	RESET_FAKE(buffer_get_data_copy);
}

TEST_TEAR_DOWN(ProtocolParser)
{
}

TEST(ProtocolParser, ProtocolParser_Given_CorrectSFD_When_Receive_Then_SfdReceived)
{
	// Given
	uint8_t incoming_bytes[16] = {0x12, 0xDB, 0xFB, 0xAA, 0xF4, 0xCB, 0x34, 0x15, 0xFD, 0xBA, 0xDC, 0x01, 0x50, 0xB4, 0x11, 0xFF};
	ProtocolParserStruct parser;
	ProtocolParser_Init(&parser);

	// When
	for (uint8_t i = 0; i < 16; i++)
	{
		uint8_t data = incoming_bytes[i];
		ProtocolParser_Update(&parser, data, 0);
	}

	// Then
	TEST_ASSERT_EQUAL_INT(8, parser.sfd_receive_count);
	TEST_ASSERT_EQUAL_INT(PROTOCOL_PARSER_TYPE, parser.state);
}

TEST(ProtocolParser, ProtocolParser_Given_IncorrectSFD_When_Receive_Then_SfdNotReceived)
{
	// Given
	uint8_t incoming_bytes[16] = {0x12, 0xDB, 0xFB, 0xAA, 0xF4, 0xCB, 0x34, 0x15, 0xFD, 0xBA, 0xDC, 0x01, 0x50, 0xB4, 0x11, 0xAC};
	ProtocolParserStruct parser;
	ProtocolParser_Init(&parser);

	// When
	for (uint8_t i = 0; i < 16; i++)
	{
		uint8_t data = incoming_bytes[i];
		ProtocolParser_Update(&parser, data, 0);
	}

	// Then
	TEST_ASSERT_EQUAL_INT(7, parser.sfd_receive_count);
	TEST_ASSERT_EQUAL_INT(PROTOCOL_PARSER_SFD, parser.state);
}

TEST(ProtocolParser, ProtocolParser_Given_SfdAndType_When_Receive_Then_TypeReceived)
{
	// Given
	uint8_t incoming_bytes[9] = {0xFD, 0xBA, 0xDC, 0x01, 0x50, 0xB4, 0x11, 0xFF, 0x05};
	ProtocolParserStruct parser;
	ProtocolParser_Init(&parser);

	// When
	for (uint8_t i = 0; i < 9; i++)
	{
		uint8_t data = incoming_bytes[i];
		ProtocolParser_Update(&parser, data, 0);
	}

	// Then
	TEST_ASSERT_EQUAL_INT(PROTOCOL_PARSER_SIZE, parser.state);
	TEST_ASSERT_EQUAL_INT(PROTOCOL_COMMAND_SET_SETTINGS, parser.current_package.type);
}

TEST(ProtocolParser, ProtocolParser_Given_Size_When_Receive_Then_SizeReceived)
{
	// Given
	uint8_t incoming_bytes[11] = {0xFD, 0xBA, 0xDC, 0x01, 0x50, 0xB4, 0x11, 0xFF, 0x05, 0x3B, 0x01};
	ProtocolParserStruct parser;
	ProtocolParser_Init(&parser);

	// When
	for (uint8_t i = 0; i < 11; i++)
	{
		uint8_t data = incoming_bytes[i];
		ProtocolParser_Update(&parser, data, 0);
	}

	// Then
	TEST_ASSERT_EQUAL_INT(PROTOCOL_PARSER_PAYLOAD, parser.state);
	TEST_ASSERT_EQUAL_INT(315, parser.current_package.size);
}

TEST(ProtocolParser, ProtocolParser_Given_ZeroSize_When_Receive_Then_CrcState)
{
	// Given
	uint8_t incoming_bytes[11] = {0xFD, 0xBA, 0xDC, 0x01, 0x50, 0xB4, 0x11, 0xFF, 0x05, 0x00, 0x00};
	ProtocolParserStruct parser;
	ProtocolParser_Init(&parser);

	// When
	for (uint8_t i = 0; i < 11; i++)
	{
		uint8_t data = incoming_bytes[i];
		ProtocolParser_Update(&parser, data, 0);
	}

	// Then
	TEST_ASSERT_EQUAL_INT(PROTOCOL_PARSER_CRC, parser.state);
	TEST_ASSERT_EQUAL_INT(0, parser.current_package.size);
}

TEST(ProtocolParser, ProtocolParser_Given_CRC_When_Receive_Then_CrcCorrectReceiveAndCalculate)
{
	// Given
	uint8_t incoming_bytes[12] = {0xFD, 0xBA, 0xDC, 0x01, 0x50, 0xB4, 0x11, 0xFF, 0x05, 0x00, 0x00, 0xC0};
	ProtocolParserStruct parser;
	ProtocolParser_Init(&parser);

	// When
	for (uint8_t i = 0; i < 12; i++)
	{
		uint8_t data = incoming_bytes[i];
		ProtocolParser_Update(&parser, data, 0);
	}

	// Then
	TEST_ASSERT_EQUAL_INT(PROTOCOL_PARSER_RECEIVED, parser.state);
	TEST_ASSERT_EQUAL_INT(0xC0, parser.current_package.crc);
}

TEST(ProtocolParser, ProtocolParser_Given_SequencePackages_When_Receive_Then_CorrectReceive)
{
	// Given
	uint8_t first_package[13] = {0xFD, 0xBA, 0xDC, 0x01, 0x50, 0xB4, 0x11, 0xFF, 0x05, 0x01, 0x00, 0x32, 0xBB};
	uint8_t second_package[14] = {0xFD, 0xBA, 0xDC, 0x01, 0x50, 0xB4, 0x11, 0xFF, 0x06, 0x02, 0x00, 0xFB, 0xAC, 0xA9};
	uint8_t excepcted_payload_first[1] = {0x32};
	uint8_t excepcted_payload_second[2] = {0xFB, 0xAC};

	ProtocolParserStruct parser;
	ProtocolParser_Init(&parser);

	// When & Then
	// First package receive test
	ProtocolParser_Update(&parser, 5, 0);
	ProtocolParser_Update(&parser, 6, 0);
	ProtocolParser_Update(&parser, 7, 0);
	
	buffer_lenght_fake.return_val = 1;
	buffer_get_data_copy_fake.return_val = excepcted_payload_first;

	for (uint8_t i = 0; i < 13; i++)
	{
		uint8_t data = first_package[i];
		ProtocolParser_Update(&parser, data, 0);
	}

	TEST_ASSERT_EQUAL_INT(PROTOCOL_PARSER_RECEIVED, parser.state);
	TEST_ASSERT_EQUAL_INT(0xBB, parser.real_crc);
	TEST_ASSERT_EQUAL_INT(1, buffer_add_fake.call_count);

	ProtocolPackageStruct package;
	ProtocolParser_PopPackage(&parser, &package);

	TEST_ASSERT_EQUAL_INT(PROTOCOL_COMMAND_SET_SETTINGS, package.type);
	TEST_ASSERT_EQUAL_INT(1, package.size);
	TEST_ASSERT_EQUAL_INT(0xBB, package.crc);
	TEST_ASSERT_EQUAL_UINT8_ARRAY(excepcted_payload_first, package.payload, 1);

	RESET_FAKE(buffer_add);
	RESET_FAKE(buffer_lenght);
	RESET_FAKE(buffer_get_data_copy);

	// Second package receive test
	TEST_ASSERT_EQUAL_INT(0, buffer_add_fake.call_count);
	TEST_ASSERT_EQUAL_INT(0, buffer_lenght_fake.call_count);

	for (uint8_t i = 0; i < 11; i++)
	{
		uint8_t data = second_package[i];
		ProtocolParser_Update(&parser, data, 0);
	}

	buffer_lenght_fake.return_val = 1;

	ProtocolParser_Update(&parser, second_package[11], 0);

	if (buffer_add_fake.call_count == 1)
	{
		buffer_lenght_fake.return_val = 2;
	}

	ProtocolParser_Update(&parser, second_package[12], 0);

	if (buffer_add_fake.call_count == 2)
	{
		buffer_get_data_copy_fake.return_val = excepcted_payload_second;
	}

	ProtocolParser_Update(&parser, second_package[13], 0);

	TEST_ASSERT_EQUAL_INT(PROTOCOL_PARSER_RECEIVED, parser.state);
	TEST_ASSERT_EQUAL_INT(0xA9, parser.real_crc);
	TEST_ASSERT_EQUAL_INT(2, buffer_add_fake.call_count);
	TEST_ASSERT_EQUAL_INT(2, buffer_lenght_fake.call_count);

	ProtocolPackageStruct packageSecond;
	ProtocolParser_PopPackage(&parser, &packageSecond);

	TEST_ASSERT_EQUAL_INT(PROTOCOL_COMMAND_GET_GESTURES, packageSecond.type);
	TEST_ASSERT_EQUAL_INT(2, packageSecond.size);
	TEST_ASSERT_EQUAL_INT(0xA9, packageSecond.crc);
	TEST_ASSERT_EQUAL_UINT8_ARRAY(excepcted_payload_second, packageSecond.payload, 2);

}