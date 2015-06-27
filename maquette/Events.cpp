#include "stdafx.h"
#include "Events.h"
#include "Packet.h"

using namespace MQTT;

static log4cplus::Logger logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("MQTT.Events"));

const LPCSTR CMqttEvent::m_valueNames[Value::_Count] = {
	// Connection events
	_TO_STRING(Connect),
	_TO_STRING(Disconnect),
	_TO_STRING(ConnectedSocket),
	_TO_STRING(ClosedSocket),
	_TO_STRING(ConnAck),
	_TO_STRING(ConnAckTimeout),
	_TO_STRING(KeepAlive),
	_TO_STRING(PingResp),
	_TO_STRING(PingRespTimeout),

	// Session events
	_TO_STRING(Subscribe),
	_TO_STRING(SubAck),
	_TO_STRING(Unsubscribe),
	_TO_STRING(UnsubAck),
	_TO_STRING(Publish),
	_TO_STRING(Published),
	_TO_STRING(PubAck),
	_TO_STRING(PubRec),
	_TO_STRING(PubRel),
	_TO_STRING(PubComp),
	_TO_STRING(SessionTimeout),
};

CReceivedPacketEvent::CReceivedPacketEvent(MQTT::CReceivedPacket* packet)
	: CMqttEvent(packet->event()), m_packet(packet)
{
}

CReceivedPacketEvent::~CReceivedPacketEvent()
{
	delete m_packet;
};
