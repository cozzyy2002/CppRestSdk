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

TEST_F(PacketTest, encodeRemainingLength)
{
	byte out[4] = {0};
	const byte expected[] = {0xff, 0xff, 0x7f, 0};
	size_t pos = testee.encodeRemainingLength(out, 2097151);
	for(int i = 0; i < ARRAYSIZE(out); i++) {
		EXPECT_EQ(expected[i], out[i]) << "index=" << i;
	}
	EXPECT_EQ(3, pos);
}
