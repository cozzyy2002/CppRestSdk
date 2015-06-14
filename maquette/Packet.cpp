#include "stdafx.h"
#include "Packet.h"

using namespace MQTT;

static log4cplus::Logger logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("MQTT.CPacket"));

static CReceivedPacket* CAN_NOT_CREATE(const data_t& data)
{
	CPacket::Type type(data[0]);
	LOG4CPLUS_ERROR(logger, "Packet type can't create CReceivedPacket: " << type.toString());
	return (CReceivedPacket*)NULL;
}

static CReceivedPacket* NOT_IMPLEMENTED(const data_t& data)
{
	CPacket::Type type(data[0]);
	LOG4CPLUS_ERROR(logger, "Packet type whose parser is not implemented yet: " << type.toString());
	return (CReceivedPacket*)NULL;
}

const CPacket::Type::Property CPacket::Type::m_properties[Type::_Count] = {
//	flagBit	sendToServer	name				createPacket
	0,		false,			"Reserved(0)",		CAN_NOT_CREATE,
	0,		true,			"CONNECT",			CAN_NOT_CREATE,
	0,		false,			"CONNACK",			[](const data_t& data) { return new CConnAckPacket(data); },
	0,		true,			"PUBLISH",			[](const data_t& data) { return new CPublishPacket(data); },
	0,		true,			"PUBACK",			[](const data_t& data) { return new CPubAckPacket(data); },
	0,		true,			"PUBREC",			[](const data_t& data) { return new CPubRecPacket(data); },
	2,		true,			"PUBREL",			[](const data_t& data) { return new CPubRelPacket(data); },
	0,		true,			"PUBCOMP",			[](const data_t& data) { return new CPubCompPacket(data); },
	2,		true,			"SUBSCRIBE",		CAN_NOT_CREATE,
	0,		false,			"SUBACK",			[](const data_t& data) { return new CSubAckPacket(data); },
	2,		true,			"UNSUBSCRIBE",		CAN_NOT_CREATE,
	0,		false,			"UNSUBACK",			NOT_IMPLEMENTED,
	0,		true,			"PINGREQ",			CAN_NOT_CREATE,
	0,		false,			"PINGRESP",			[](const data_t& data) { return new CPingRespPacket(data); },
	0,		true,			"DISCONNECT",		CAN_NOT_CREATE,
	0,		false,			"Reserved(15)",		CAN_NOT_CREATE,
};

/*static*/ uint16_t CPacketToSend::m_packetIdentifier = 0;

CPacketToSend::CPacketToSend(const Type& type, size_t size /*= 100*/) : CPacket(type)
{
	_ASSERTE(m_type.property.sendToServer);
	_ASSERTE(size <= remainingLengthMax);
	m_remainings.reserve(size);

	if(0 == ++m_packetIdentifier) m_packetIdentifier = 1;
}

void CPacketToSend::add(const void* pData, size_t size)
{
	byte* p = (byte*)pData;
	m_remainings.insert(m_remainings.end(), p, p + size);
}

void CPacketToSend::add(uint32_t num, size_t size /*= sizeof(uint32_t)*/)
{
	static const size_t maxSize = sizeof(num);

	// uint32_t value in network byte order
	// data[0] stores most significant byte
	byte data[maxSize];
	switch(size) {
	case 4: data[0] = HIBYTE(HIWORD(num));
	case 3: data[1] = LOBYTE(HIWORD(num));
	case 2: data[2] = HIBYTE(LOWORD(num));
	case 1: data[3] = LOBYTE(LOWORD(num));
		add(&data[maxSize - size], size);
		break;
	default:
		LOG4CPLUS_FATAL(logger, "Illegal size: " << size);
		_ASSERTE(FALSE);
		break;
	}
}

const data_t& CPacketToSend::data(byte flagBit)
{
	byte remainingLength[4];
	size_t pos = encodeRemainingLength(remainingLength, m_remainings.size());

	// Build MQTT control packet
	m_data.reserve(1 + pos + m_remainings.size());
	m_data.push_back(m_type.encode(flagBit));										// Pcket type and flags
	m_data.insert(m_data.end(), remainingLength, &remainingLength[pos]);			// Remaining Length
	if(!m_remainings.empty()) {
		m_data.insert(m_data.end(), m_remainings.begin(), m_remainings.end());		// Variable header and Payload, if any
	}
	return m_data;
}

// Encode Remaining Length
// See http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_Toc398718023
template<size_t size>
size_t CPacketToSend::encodeRemainingLength(byte(& encoded)[size], size_t lengthToEncode) const
{
	size_t X = lengthToEncode;
	size_t pos = 0;			// Position of remainingLength array
	do {
		byte& encodedByte = encoded[pos++];
		encodedByte = X % 128;
		X = X / 128;
		// if there are more data to encode, set the top bit of this byte
		if(X > 0) {
			encodedByte = encodedByte | 128;
		}
	} while((X > 0) && (pos < ARRAYSIZE(encoded)));
	_ASSERTE(X == 0 /* Remaining Length has been fully encoded */);
	// Now pos equals to the size of Remaining Length
	return pos;
}

const data_t& CSimplePacket::data()
{
	if(m_usePacketIdentifier) add(m_packetIdentifier);
	return CPacketToSend::data(0);
}

const data_t& CConnectPacket::data()
{
	add("MQTT");						// Protocol Name
	add((byte)4);						// Protocol Level
	add((byte)2);						// Connect Flags(Clean Session = 1)
	add((uint16_t)m_params.keepAlive);	// Keep Alive(second)
	add(utility::conversions::to_utf8string(m_params.clientId));	// Client Identifier

	return CPacketToSend::data(0);
}

const data_t& CSubscribePacket::data()
{
	add(m_packetIdentifier);
	for(CSubscribeEvent::Params::const_iterator i = m_params.begin(); i != m_params.end(); i++) {
		add(i->topic);
		add((byte)i->qos);
	}
	return CPacketToSend::data(0);
}

const data_t& CPublishPacket::data()
{
	add(m_params.topic);
	if(QOS_0 < m_params.qos) add(m_packetIdentifier);
	add(m_params.payload);

	byte flagBit = (m_dup ? 0x08 : 0x00) | (m_params.qos << 1) | (m_params.retain ? 0x01 : 0x00);
	return CPacketToSend::data(flagBit);
}

/*static*/ CReceivedPacket* CReceivedPacket::create(const data_t& data)
{
	if(data.size() < 2) {
		LOG4CPLUS_ERROR(logger, "Packet length is too short: " << data.size());
		return NULL;
	}

	byte typeValue = data[0];
	if(!Type::checkValue(typeValue)) {
		LOG4CPLUS_ERROR(logger, "Invalid packet type: " << typeValue);
		return NULL;
	}

	Type type(typeValue);
	CReceivedPacket* packet = type.property.createPacket(data);
	if(packet && !packet->parse()) {
		delete packet;
		packet = NULL;
	}
	return packet;
}

bool CReceivedPacket::parse()
{
	size_t pos = 1;		// Position of Remaining Length in m_data
	remainingLength = decodeRemainingLength(pos);
	if(pos == 0) return false;
	if(remainingLength + pos != m_data.size()) {
		LOG4CPLUS_ERROR(logger, "Wrong Remaining Length. " << remainingLength << ", packet size=" << m_data.size());
		return false;
	}

	return parse(pos);
}

// Decode Remaining Length
// See http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_Toc398718023
size_t CReceivedPacket::decodeRemainingLength(size_t& pos) const
{
	size_t multiplier = 1;
	size_t value = 0;
	byte encodedByte;
	do {
		encodedByte = m_data[pos++];	// 'next byte from stream'
		value += (encodedByte & 127) * multiplier;
		multiplier *= 128;
		if((multiplier > 128 * 128 * 128) || (m_data.size() < pos)) {
			LOG4CPLUS_ERROR(logger, "Malformed Remaing Length. multiplier=" << multiplier << ", pos=" << pos);
			pos = 0;
			return 0;
		}
	} while((encodedByte & 128) != 0);
	return value;
}

uint16_t CReceivedPacket::makeWord(size_t& pos) const
{
	uint16_t word = MAKEWORD(m_data[pos + 1], m_data[pos]);
	pos += sizeof(uint16_t);
	return word;
}

bool CSimplePacket::parse(size_t& pos, bool usePacketIdentifier)
{
	if(m_usePacketIdentifier) {
		packetIdentifire = makeWord(pos);
	}
	return true;
}

bool CConnAckPacket::parse(size_t& pos)
{
	returnCode = m_data[pos];
	isAccepted = (returnCode == CReturnCode::ConnectionAccepted);
	return true;
}

bool CSubAckPacket::parse(size_t& pos)
{
	packetIdentifire = makeWord(pos);
	const byte& returnCode = m_data[pos];
	qos = returnCode & 0x03;
	isAccepted = (returnCode & 0x80) == 0;
	return true;
}

bool CPublishPacket::parse(size_t& pos)
{
	m_params.qos = (QOS)((m_data[0] >> 1) & 0x03);
	m_params.retain = m_data[0] & 0x01;
	size_t size = makeWord(pos);		// Size of Topic string
	m_params.topic.assign((LPCSTR)&m_data[pos], size); pos += size;
	if(QOS_0 < m_params.qos) {
		m_params.packetIdentifier = makeWord(pos);
	}
	m_params.payload.assign(m_data.begin() + pos, m_data.end());
	return true;
}

const LPCSTR CConnAckPacket::CReturnCode::m_valueNames[Value::_Count] = {
	_TO_STRING(ConnectionAccepted),
	_TO_STRING(UnacceptableProtocolVersion),
	_TO_STRING(IdentifierRejected),
	_TO_STRING(ServerUnavailable),
	_TO_STRING(BadUserNameOrPassword),
	_TO_STRING(NotAuthorized),
};
