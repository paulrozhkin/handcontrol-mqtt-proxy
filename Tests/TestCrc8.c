#include "unity.h"
#include "unity_fixture.h"
#include "crc8.h"

TEST_GROUP(CRC8);

TEST_SETUP(CRC8)
{
}

TEST_TEAR_DOWN(CRC8)
{
}

TEST(CRC8, CRC8_Given_SimpleByteArray_When_CalculateCrc8_Then_CorrectCrc8)
{
	// Given
	uint8_t data[] = {0x04, 0x03, 0xFB, 0xAD, 0xFF, 0xBD};
	uint8_t expected_crc8 = 0xBF;

	// When
	uint8_t crc8 = calculate_crc8(data, sizeof(data), 0);

	// Then
	TEST_ASSERT_EQUAL_INT(expected_crc8, crc8);
}