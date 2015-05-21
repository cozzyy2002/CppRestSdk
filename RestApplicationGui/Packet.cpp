#include "stdafx.h"
#include "Packet.h"

using namespace MQTT;

static log4cplus::Logger logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("MQTT.CPacket"));

static CReceivedPacket* CAN_NOT_CREATE(const data_t& data)
{
	LOG4CPLUS_ERROR(logger, "Packet type can't create CReceivedPacket: " << data[3]);
	return (CReceivedPacket*)NULL;
}

const CPacket::Type::Property CPacket::Type::m_properties[Type::_Count] = {
//	flagBit	sendToServer	receiveFromServer	name				createPacket
	0,		false,			false,				"Reserved(0)",		CAN_NOT_CREATE,
	0,		true,			false,				"CONNECT",			CAN_NOT_CREATE,
	0,		false,			true,				"CONNACK",			[](const data_t& data) { return new CConnAckPacket(data); },
	0,		true,			true,				"PUBLISH",			CAN_NOT_CREATE,
	0,		true,			true,				"PUBACK",			CAN_NOT_CREATE,
	0,		true,			true,				"PUBREC",			CAN_NOT_CREATE,
	2,		true,			true,				"PUBREL",			CAN_NOT_CREATE,
	0,		true,			true,				"PUBCOMP",			CAN_NOT_CREATE,
	2,		true,			false,				"SUBSCRIBE",		CAN_NOT_CREATE,
	0,		false,			true,				"SUBACK",			CAN_NOT_CREATE,
	2,		true,			false,				"UNSUBSCRIBE",		CAN_NOT_CREATE,
	0,		false,			true,				"UNSUBACK",			CAN_NOT_CREATE,
	0,		true,			false,				"PINGREQ",			CAN_NOT_CREATE,
	0,		false,			true,				"PINGRESP",			CAN_NOT_CREATE,
	0,		true,			false,				"DISCONNECT",		CAN_NOT_CREATE,
	0,		false,			false,				"Reserved(15)",		CAN_NOT_CREATE,
};

CPacketToSend::CPacketToSend(Type::Value type, size_t size /*= 100*/) : CPacket(type)
{
	_ASSERTE(m_type.property().sendToServer);
	_ASSERTE(size <= remainingLengthMax);
	m_variableData.reserve(size);
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
	if(!Type::validate(typeValue)) {
		LOG4CPLUS_ERROR(logger, "Invalid packet type: " << typeValue);
		return NULL;
	}

	Type type(typeValue);
	CReceivedPacket* packet = type.property().createPacket(data);
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
	// TODO: Decode Remaining Length

	return parseInternal();
}
