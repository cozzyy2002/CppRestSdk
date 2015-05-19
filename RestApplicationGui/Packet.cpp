#include "stdafx.h"
#include "Packet.h"

using namespace MQTT;

const CPacket::Type::Property CPacket::Type::m_properties[Type::_Count] = {
//	flagBit	sendToServer	receiveFromServer	name
	0,		false,			false,				"Reserved(0)",
	0,		true,			false,				"CONNECT",
	0,		false,			true,				"CONNACK",
	0,		true,			true,				"PUBLISH",
	0,		true,			true,				"PUBACK",
	0,		true,			true,				"PUBREC",
	2,		true,			true,				"PUBREL",
	0,		true,			true,				"PUBCOMP",
	2,		true,			false,				"SUBSCRIBE",
	0,		false,			true,				"SUBACK",
	2,		true,			false,				"UNSUBSCRIBE",
	0,		false,			true,				"UNSUBACK",
	0,		true,			false,				"PINGREQ",
	0,		false,			true,				"PINGRESP",
	0,		true,			false,				"DISCONNECT",
	0,		false,			false,				"Reserved(15)",

};

CPacket::CPacket(Type type) : m_type(type)
{}

CPacket::~CPacket()
{}

CPacketToSend::CPacketToSend(Type type, size_t size /*= 100*/) : CPacket(type)
{
	const Type::Property& property = type.property();
	_ASSERTE(property.sendToServer);

	_ASSERTE(size <= remainingLengthMax);
	m_variableData.reserve(size);
}
