#include "stdafx.h"
#include "Packet.h"

using namespace MQTT;

static log4cplus::Logger logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("MQTT.CPacket"));

static CReceivedPacket* ILLEGAL_PARSER(const data_t& data)
{
	LOG4CPLUS_ERROR(logger, "Packet type can't parse: " << data[3]);
	return (CReceivedPacket*)NULL;
}

const CPacket::Type::Property CPacket::Type::m_properties[Type::_Count] = {
//	flagBit	sendToServer	receiveFromServer	name				parser
	0,		false,			false,				"Reserved(0)",		ILLEGAL_PARSER,
	0,		true,			false,				"CONNECT",			ILLEGAL_PARSER,
	0,		false,			true,				"CONNACK",			[](const data_t& data) { return new CConnAckPacket(data); },
	0,		true,			true,				"PUBLISH",			ILLEGAL_PARSER,
	0,		true,			true,				"PUBACK",			ILLEGAL_PARSER,
	0,		true,			true,				"PUBREC",			ILLEGAL_PARSER,
	2,		true,			true,				"PUBREL",			ILLEGAL_PARSER,
	0,		true,			true,				"PUBCOMP",			ILLEGAL_PARSER,
	2,		true,			false,				"SUBSCRIBE",		ILLEGAL_PARSER,
	0,		false,			true,				"SUBACK",			ILLEGAL_PARSER,
	2,		true,			false,				"UNSUBSCRIBE",		ILLEGAL_PARSER,
	0,		false,			true,				"UNSUBACK",			ILLEGAL_PARSER,
	0,		true,			false,				"PINGREQ",			ILLEGAL_PARSER,
	0,		false,			true,				"PINGRESP",			ILLEGAL_PARSER,
	0,		true,			false,				"DISCONNECT",		ILLEGAL_PARSER,
	0,		false,			false,				"Reserved(15)",		ILLEGAL_PARSER,
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

const data_t& CPacketToSend::data()
{
	size_t remainingLength = m_variableData.size();
	m_data.reserve(2 + remainingLength);
	m_data.push_back(m_type.encode());
	m_data.push_back(remainingLength);
	m_data.insert(m_data.end(), m_variableData.begin(), m_variableData.end());
	return m_data;
}

/*static*/ CReceivedPacket* CReceivedPacket::parse(const data_t& data)
{
	byte typeValue = data[0];
	if(!Type::validate(typeValue)) {
		LOG4CPLUS_ERROR(logger, "Invalid packet type: " << typeValue);
		return NULL;
	}

	Type type(typeValue);
	return type.property().parser(data);
}
