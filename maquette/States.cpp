#include "stdafx.h"
#include "States.h"

using namespace MQTT;

static log4cplus::Logger logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("MQTT.States"));

const LPCSTR CMqttState::m_valueNames[Value::_Count] = {
	_TO_STRING(Initial),
	_TO_STRING(ConnectingSocket),
	_TO_STRING(ConnectingBroker),
	_TO_STRING(Connected),
	_TO_STRING(Disconnecting)
};

const LPCSTR CMqttEvent::m_valueNames[Value::_Count] = {
	_TO_STRING(Connect),
	_TO_STRING(Disconnect),
	_TO_STRING(ConnectedSocket),
	_TO_STRING(ClosedSocket),
	_TO_STRING(ConnAck),
	_TO_STRING(Subscribe),
	_TO_STRING(SubAck),
	_TO_STRING(Publish),
	_TO_STRING(Published),
	_TO_STRING(PingTimer),
};