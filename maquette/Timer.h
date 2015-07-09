#pragma once

namespace MQTT {

	class CTimer {
	public:
		explicit CTimer(bool repeat = false);
		virtual ~CTimer();

		template<typename T>
		void start(DWORD ms, T param, std::function<void (T)> timeoutFunc);
		void restart();
		void cancel();
		bool isActive() const { return m_active; };

	protected:
		bool m_repeat;
		bool m_active;
		HANDLE m_cancelEvent;
		HANDLE m_restartEvent;

		static log4cplus::Logger logger;
	};

	template<typename T>
	void CTimer::start(DWORD ms, T param, std::function<void (T)> timeoutFunc)
	{
		m_active = true;
		pplx::task_completion_event<void> tce;
		pplx::task<void> task(tce);
		task.then([this, ms, timeoutFunc, param](pplx::task<void> task) {
			HANDLE events[] = {m_cancelEvent, m_restartEvent};
			::ResetEvent(events[0]);
			::ResetEvent(events[1]);
			bool repeat = m_repeat;
			do {
				DWORD wait = ::WaitForMultipleObjects(ARRAYSIZE(events), events, FALSE, ms);
				switch(wait) {
				case WAIT_TIMEOUT:
					LOG4CPLUS_DEBUG(logger, "Timeout occurred. ms=" << ms);
					timeoutFunc(param);
					break;
				case WAIT_OBJECT_0:
					LOG4CPLUS_DEBUG(logger, "Timer is canceled");
					::ResetEvent(events[0]);
					repeat = false;
					break;
				case WAIT_OBJECT_0 + 1:
					LOG4CPLUS_DEBUG(logger, "Timer is restarted");
					::ResetEvent(events[1]);
					break;
				default:
					LOG4CPLUS_FATAL(logger, "WaitForSingleObject() failed. error=" << ::GetLastError());
					repeat = false;
					break;
				}
			} while(repeat);
			m_active = false;
		});
		tce.set();
	}

} // namespace MQTT
