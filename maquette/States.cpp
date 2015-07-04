#include "stdafx.h"
#include "States.h"

using namespace MQTT;

static log4cplus::Logger logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("MQTT.States"));

const LPCSTR CConnectionState::m_valueNames[Value::_Count] = {
	_TO_STRING(Initial),
	_TO_STRING(ConnectingSocket),
	_TO_STRING(ConnectingBroker),
	_TO_STRING(Connected),
	_TO_STRING(Disconnecting)
};

