#include "unity.h"
#include "unity_fixture.h"

TEST_GROUP_RUNNER(ProtocolParser)
{
	RUN_TEST_CASE(ProtocolParser, ProtocolParser_Given_CorrectSFD_When_Receive_Then_SfdReceived);
	RUN_TEST_CASE(ProtocolParser, ProtocolParser_Given_IncorrectSFD_When_Receive_Then_SfdNotReceived);
	RUN_TEST_CASE(ProtocolParser, ProtocolParser_Given_SfdAndType_When_Receive_Then_TypeReceived);
	RUN_TEST_CASE(ProtocolParser, ProtocolParser_Given_Size_When_Receive_Then_SizeReceived);
	RUN_TEST_CASE(ProtocolParser, ProtocolParser_Given_ZeroSize_When_Receive_Then_CrcState);
	RUN_TEST_CASE(ProtocolParser, ProtocolParser_Given_CRC_When_Receive_Then_CrcCorrectReceiveAndCalculate);
	RUN_TEST_CASE(ProtocolParser, ProtocolParser_Given_SequencePackages_When_Receive_Then_CorrectReceive);
}