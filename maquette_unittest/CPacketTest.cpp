#include "stdafx.h"
#include "Packet.h"

using namespace testing;
using namespace MQTT;

static log4cplus::Logger logger = log4cplus::Logger::getInstance(_T("MQTT.unittest.CPacketTest"));

class Testee : public CSimplePacket {
public:
	Testee(const Type& type, bool usePacketIdentifier)
		: CPacket(type), CSimplePacket(type, usePacketIdentifier) {};
	Testee(const Type& type, const data_t& data, CMqttEvent::Value event, bool usePacketIdentifier)
		: CPacket(type), CSimplePacket(type, data, event, usePacketIdentifier) {};

	using CPacketToSend::encodeRemainingLength;
};

class PacketTest : public Test {
public:
	PacketTest() : testee(CPacket::Type::CONNECT, false) {};
	Testee testee;
};

struct _EncodedRemainingLengthTestData {
	size_t length;		// Length to encode
	size_t pos;			// Expected position returned by encodedRemainingLength()
	byte encoded[4];	// Expected encoded value
} EncodedRemainingLengthTestData[] = {
	{0, 1, {0}},
	{127, 1, {0x7F}},
	{128, 2, {0x80, 0x01}},
	{16383, 2, {0xFF, 0x7F}},
	{16384, 3, {0x80, 0x80, 0x01}},
	{2097151, 3, {0xFF, 0xFF, 0x7F}},
	{2097152, 4, {0x80, 0x80, 0x80, 0x01}},
	{268435455, 4, {0xFF, 0xFF, 0xFF, 0x7F}},
};

class EncodedRemainingLengthTest
	: public PacketTest
	, public WithParamInterface<size_t> {};

TEST_P(EncodedRemainingLengthTest, normal)
{
	const _EncodedRemainingLengthTestData& data = EncodedRemainingLengthTestData[GetParam()];

	byte out[4] = {0};
	size_t pos = testee.encodeRemainingLength(out, data.length);
	for(int i = 0; i < ARRAYSIZE(out); i++) {
		EXPECT_EQ(data.encoded[i], out[i]) << "index=" << i;
	}
	EXPECT_EQ(data.pos, pos);
}

INSTANTIATE_TEST_CASE_P(CPacketToSend, EncodedRemainingLengthTest, Range((size_t)0, ARRAYSIZE(EncodedRemainingLengthTestData)));
