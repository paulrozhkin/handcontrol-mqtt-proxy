#include "unity.h"
#include "unity_fixture.h"

TEST_GROUP_RUNNER(CRC8)
{
	RUN_TEST_CASE(CRC8, CRC8_Given_SimpleByteArray_When_CalculateCrc8_Then_CorrectCrc8);
}