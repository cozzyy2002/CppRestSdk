#include "stdafx.h"
#include "Packet.h"

using namespace MQTT;

static log4cplus::Logger logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("MQTT.CPacket"));

static CReceivedPacket* CAN_NOT_CREATE(const data_t& data)
{
	LOG4CPLUS_ERROR(logger, "Packet type can't create CReceivedPacket: " << (data[0] >> 4));
	return (CReceivedPacket*)NULL;
}

const CPacket::Type::Property CPacket::Type::m_properties[Type::_Count] = {
//	flagBit	sendToServer	receiveFromServer	name				createPacket
	0,		false,			false,				"Reserved(0)",		CAN_NOT_CREATE,
	0,		true,			false,				"CONNECT",			CAN_NOT_CREATE,
	0,		false,			true,				"CONNACK",			[](const data_t& data) { return new CConnAckPacket(data); },
	0,		true,			true,				"PUBLISH",			[](const data_t& data) { return new CPublishPacket(data); },
	0,		true,			true,				"PUBACK",			CAN_NOT_CREATE,
	0,		true,			true,				"PUBREC",			CAN_NOT_CREATE,
	2,		true,			true,				"PUBREL",			CAN_NOT_CREATE,
	0,		true,			true,				"PUBCOMP",			CAN_NOT_CREATE,
	2,		true,			false,				"SUBSCRIBE",		CAN_NOT_CREATE,
	0,		false,			true,				"SUBACK",			[](const data_t& data) { return new CSubAckPacket(data); },
	2,		true,			false,				"UNSUBSCRIBE",		CAN_NOT_CREATE,
	0,		false,			true,				"UNSUBACK",			CAN_NOT_CREATE,
	0,		true,			false,				"PINGREQ",			CAN_NOT_CREATE,
	0,		false,			true,				"PINGRESP",			CAN_NOT_CREATE,
	0,		true,			false,				"DISCONNECT",		CAN_NOT_CREATE,
	0,		false,			false,				"Reserved(15)",		CAN_NOT_CREATE,
};

/*static*/ uint16_t CPacketToSend::m_packetIdentifier = 0;

CPacketToSend::CPacketToSend(const Type& type, size_t size /*= 100*/) : CPacket(type)
{
	_ASSERTE(m_type.property.sendToServer);
	_ASSERTE(size <= remainingLengthMax);
	m_variableData.reserve(size);

	if(0 == ++m_packetIdentifier) m_packetIdentifier = 1;
}

void CPacketToSend::add(const void* pData, size_t size)
{
	byte* p = (byte*)pData;
	m_variableData.insert(m_variableData.end(), p, &(p[size]));
}

void CPacketToSend::add(uint32_t num, size_t size /*= sizeof(uint32_t)*/)
{
	static const int maxSize = sizeof(uint32_t);
	_ASSERTE((0 < size) && (size <= maxSize));

	// uint32_t value in network byte order
	// data[0] stores most significant byte
	byte data[maxSize] = {
		HIBYTE(HIWORD(num)), LOBYTE(HIWORD(num)), HIBYTE(LOWORD(num)), LOBYTE(LOWORD(num))
	};

	add(&data[maxSize - size], size);
}

const data_t& CPacketToSend::data()
{
	m_data.reserve(2 + m_variableData.size());
	m_data.push_back(m_type.encode());
	m_data.push_back((byte)remainingLength());
	if(!m_variableData.empty()) {
		m_data.insert(m_data.end(), m_variableData.begin(), m_variableData.end());
	}
	return m_data;
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
	bool ok = (packet != NULL);
	if(ok) {
		ok = packet->parse();
		if(!ok) {
			delete packet;
			packet = NULL;
		}
	}
	return packet;
}

bool CReceivedPacket::parse()
{
	// Decode Remaining Length
	// See http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_Toc398718023
	size_t multiplier = 1;
	size_t value = 0;
	byte encodedByte;
	size_t pos = 1;		// MSB position of Remaining Length in m_data
	do {
		encodedByte = m_data[pos++];	// 'next byte from stream'
		value += (encodedByte & 127) * multiplier;
		multiplier *= 128;
		if((multiplier > 128 * 128 * 128) || (m_data.size() < pos)) {
			LOG4CPLUS_ERROR(logger, "Malformed Remaing Length. multiplier=" << multiplier << ", pos=" << pos);
			return false;
		}
	} while((encodedByte & 128) != 0);
	remainingLength = value;
	if(remainingLength + 2 != m_data.size()) {
		LOG4CPLUS_ERROR(logger, "Wrong Remaining Length. " << remainingLength << ", packet size=" << m_data.size());
		return false;
	}

	return parseInternal();
}

const LPCSTR CConnAckPacket::CReturnCode::m_valueNames[Value::_Count] = {
	_TO_STRING(ConnectionAccepted),
	_TO_STRING(UnacceptableProtocolVersion),
	_TO_STRING(IdentifierRejected),
	_TO_STRING(ServerUnavailable),
	_TO_STRING(BadUserNameOrPassword),
	_TO_STRING(NotAuthorized),
};
