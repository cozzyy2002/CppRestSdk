#include "stdafx.h"
#include "Timer.h"

using namespace MQTT;

log4cplus::Logger CTimer::logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("MQTT.CTimer"));

CTimer::CTimer(bool repeat /*= false*/) : m_repeat(repeat), m_active(false)
{
	LOG4CPLUS_DEBUG(logger, "Creating CTimer: this=" << (void*)this);
	m_cancelEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	m_restartEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	if(!m_cancelEvent || !m_restartEvent) {
		LOG4CPLUS_FATAL(logger, __FUNCTION__ " CreateEvent() failed. error=" << ::GetLastError());
	}
}


CTimer::~CTimer()
{
	LOG4CPLUS_DEBUG(logger, "Deleting CTimer: this=" << (void*)this);
	cancel();
	try {
		m_task.wait();
	} catch(const std::exception&) { /* Ignore */ }

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
