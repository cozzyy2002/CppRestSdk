#include "stdafx.h"
#include "Packet.h"

using namespace testing;
using namespace MQTT;

static log4cplus::Logger logger = log4cplus::Logger::getInstance(_T("MQTT.unittest.CPacketTest"));

class Testee : public CSimplePacket {
public:
	Testee(const Type& type)
		: CPacket(type), CSimplePacket(type) {};
	Testee(const Type& type, const data_t& data, CMqttEvent::Value event)
		: CPacket(type, data), CSimplePacket(type, event) {};

	using CPacketToSend::encodeRemainingLength;
	using CReceivedPacket::decodeRemainingLength;
	using CReceivedPacket::m_data;
};

class CPacketTest : public Test {
public:
	CPacketTest() : testee(CPacket::Type::CONNECT) {};
	Testee testee;

	static const size_t remainingLengthPosition = 1;	// Position of encoded Remaining Length in m_data
};

struct _RemainingLengthTestData {
	size_t decoded;			// Remaining Length to encode or decoded
	size_t size;			// Size of Encoded array
	data_t encoded;			// Remaining Length encoded or to decode
};

static const struct _RemainingLengthTestData RemainingLengthTestData[] = {
	{0, 1, {0, 0, 0, 0}},
	{127, 1, {0x7F, 0, 0, 0}},
	{128, 2, {0x80, 0x01, 0, 0}},
	{16383, 2, {0xFF, 0x7F, 0, 0}},
	{16384, 3, {0x80, 0x80, 0x01, 0}},
	{2097151, 3, {0xFF, 0xFF, 0x7F, 0}},
	{2097152, 4, {0x80, 0x80, 0x80, 0x01}},
	{268435455, 4, {0xFF, 0xFF, 0xFF, 0x7F}},
};

class EncodeRemainingLengthTest
	: public CPacketTest
	, public WithParamInterface<size_t> {};

TEST_P(EncodeRemainingLengthTest, normal)
{
	const _RemainingLengthTestData& data = RemainingLengthTestData[GetParam()];

	byte out[4] = {0};
	size_t pos = testee.encodeRemainingLength(out, data.decoded);
	EXPECT_EQ(data.encoded, data_t(out, out + ARRAYSIZE(out)));
	EXPECT_EQ(data.size, pos);
}

INSTANTIATE_TEST_CASE_P(CPacketToSend, EncodeRemainingLengthTest, Range((size_t)0, ARRAYSIZE(RemainingLengthTestData)));

class DecodeRemainingLengthTest
	: public CPacketTest
	, public WithParamInterface<size_t> {};

TEST_P(DecodeRemainingLengthTest, normal)
{
	const _RemainingLengthTestData& data = RemainingLengthTestData[GetParam()];

	size_t pos = 0;
	testee.m_data = data.encoded;
	size_t decoded = testee.decodeRemainingLength(pos);
	EXPECT_EQ(data.decoded, decoded);
	EXPECT_EQ(data.size, pos);
}

INSTANTIATE_TEST_CASE_P(CReceivedPacket, DecodeRemainingLengthTest, Range((size_t)0, ARRAYSIZE(RemainingLengthTestData)));

TEST_F(CPacketTest, decodeRemainingLength_shortPacket)
{
	testee.m_data.assign({0, 0xFF});
	size_t pos = remainingLengthPosition, decoded;
	ASSERT_THROW(decoded = testee.decodeRemainingLength(pos), std::exception);
}

TEST_F(CPacketTest, decodeRemainingLength_exceedsMax)
{
	testee.m_data.assign({0, 0xFF, 0xFF, 0xFF, 0x80, 0});
	size_t pos = remainingLengthPosition, decoded;
	ASSERT_NO_THROW(decoded = testee.decodeRemainingLength(pos));
	EXPECT_EQ(0, decoded);
	EXPECT_EQ(0, pos);
}
