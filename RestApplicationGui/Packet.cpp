#include "stdafx.h"
#include "Packet.h"

using namespace MQTT;

CPacket::CPacket(Type type, size_t size /*= 100*/) : m_type(type)
{
	_ASSERTE(size <= remainingLengthMax);
	switch(type) {
	case Type::PUBREL:
	case Type::SUBSCRIBE:
	case Type::UNSUBSCRIBE:
		// Packet to send with Flag Bit value 2
		m_flagBit = 2;
		break;
	case Type::CONNECT:
	case Type::PUBLISH:
	case Type::PUBACK:
	case Type::PUBREC:
	case Type::PUBCOMP:
	case Type::PINGREQ:
	case Type::DISCONNECT:
		// Packet to send with Flag Bit value 0
		m_flagBit = 0;
		break;
	default:
		// Other Packet types are not sent
		_ASSERTE(FALSE);
		break;
	}
	m_data.reserve(size);
}

CPacket::~CPacket()
{}
