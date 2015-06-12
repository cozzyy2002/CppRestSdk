#include "stdafx.h"
#include "Timer.h"

using namespace MQTT;

static log4cplus::Logger logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("MQTT.CTimer"));

CTimer::CTimer(bool repeat /*= false*/) : m_repeat(repeat)
{
	m_cancelEvent = ::CreateEvent(NULL, TRUE, FALSE, NULL);
	m_restartEvent = ::CreateEvent(NULL, TRUE, FALSE, NULL);
	if(!m_cancelEvent || !m_restartEvent) {
		LOG4CPLUS_FATAL(logger, __FUNCTION__ " CreateEvent() failed. error=" << ::GetLastError());
	}
}


CTimer::~CTimer()
{
	cancel();
	::CloseHandle(m_cancelEvent);
	::CloseHandle(m_restartEvent);
}

void CTimer::cancel()
{
	if(!::SetEvent(m_cancelEvent)) {
		LOG4CPLUS_FATAL(logger, __FUNCTION__ " SetEvent() failed. error=" << ::GetLastError());
	}
}

void CTimer::restart()
{
	if(!::SetEvent(m_restartEvent)) {
		LOG4CPLUS_FATAL(logger, __FUNCTION__  " SetEvent() failed. error=" << ::GetLastError());
	}
}
