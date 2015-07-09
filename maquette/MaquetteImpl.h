#pragma once
#include "maquette/maquette.h"
#include "maquette/utils.h"
#include "States.h"
#include "Events.h"
#include "Packet.h"
#include "Timer.h"

namespace MQTT {

	class CMaquetteImpl : public CMaquette, CUtils {
	public:
		CMaquetteImpl(IMaquetteCallback* callback);
		virtual ~CMaquetteImpl();

		virtual void connect(LPCTSTR serverUrl, LPCTSTR clientId, DWORD keepAlive);
		virtual void disconnect();
		virtual void subscribe(LPCTSTR topic, QOS qos);
		virtual void unsubscribe(LPCTSTR topic);
		virtual void publish(LPCTSTR topic, const data_t& payload, QOS qos, bool retain);
		virtual LRESULT onUserEvent(WPARAM wParam, LPARAM lParam);

	protected:
		CConnectionState m_state;
		IMaquetteCallback* m_callback;

		typedef CConnectionState (CMaquetteImpl::*connection_event_handler_t)(CMqttEvent* pEvent);
		static const connection_event_handler_t state_event_table[CMqttEvent::_ConnectionEventCount][CConnectionState::_Count];

		void send(MQTT::CPacketToSend& packet, bool wait = false);
		void receive(const web::websockets::client::websocket_incoming_message& msg);

		void postEvent(CMqttEvent::Value value);
		void postEvent(CMqttEvent* pEvent);

		void startEventTimer(CTimer& timer, DWORD ms, CMqttEvent::Value event)
		{
			timer.start<CMqttEvent::Value>(ms, event, [this](CMqttEvent::Value event) {
				postEvent(event);
			});
		};

		void startSessionTimer(CSessionState& state, uint16_t packetIdentifier)
		{
			state.timer->start<uint16_t>(m_sessionResponseTimeout, packetIdentifier, [this](uint16_t packetIdentifier) {
				postEvent(new CSessionTimeoutEvent(packetIdentifier));
			});
		};

		static const DWORD m_sessionResponseTimeout = 2000;

		CConnectionState handleConnect(CMqttEvent* pEvent);
		CConnectionState handleDisconnect(CMqttEvent* pEvent);
		CConnectionState handleDisconnectSocket(CMqttEvent* pEvent);
		CConnectionState handleConnectedSocket(CMqttEvent* pEvent);
		CConnectionState handleClosedSocket(CMqttEvent* pEvent);
		CConnectionState handleConnAck(CMqttEvent* pEvent);
		CConnectionState handleKeepAlive(CMqttEvent* pEvent);
		CConnectionState handlePingResp(CMqttEvent* pEvent);
		CConnectionState handlePingRespTimeout(CMqttEvent* pEvent);
		CConnectionState handleIgnore(CMqttEvent* pEvent);
		CConnectionState handleFatal(CMqttEvent* pEvent);

		typedef std::map<uint16_t, CSessionState> session_states_t;
		session_states_t m_sessionStates;

		typedef void (CMaquetteImpl::*session_request_handler_t)(CMqttEvent* pEvent);
		typedef void (CMaquetteImpl::*session_response_handler_t)(CMqttEvent* pEvent, session_states_t::iterator it);
		typedef void (CMaquetteImpl::*session_timeout_handler_t)(CMqttEvent* pEvent, session_states_t::iterator it);
		typedef uint16_t (*packet_identifier_getter)(CMqttEvent* pEvent);

		typedef struct _SesstionEvent {
			session_request_handler_t requestHandler;
			session_response_handler_t responseHandler;
			session_timeout_handler_t timeoutHandler;
			packet_identifier_getter getPacketIdentifier;
		} SessionEvent;
		static const SessionEvent sesstion_event_table[CMqttEvent::_SessionEventCount];

		void handleSubscribe(CMqttEvent* pEvent);
		void handleSubAck(CMqttEvent* pEvent, session_states_t::iterator it);
		void handleSubAckTimeout(CMqttEvent* pEvent, session_states_t::iterator it);
		void handleUnsubscribe(CMqttEvent* pEvent);
		void handleUnsubAck(CMqttEvent* pEvent, session_states_t::iterator it);
		void handleUnsubAckTimeout(CMqttEvent* pEvent, session_states_t::iterator it);
		void handlePublish(CMqttEvent* pEvent);
		void handlePublished(CMqttEvent* pEvent);
		void handlePubAck(CMqttEvent* pEvent, session_states_t::iterator it);
		void handlePubAckTimeout(CMqttEvent* pEvent, session_states_t::iterator it);
		void handlePubRec(CMqttEvent* pEvent, session_states_t::iterator it);
		void handlePubRecTimeout(CMqttEvent* pEvent, session_states_t::iterator it);
		void handlePubRel(CMqttEvent* pEvent, session_states_t::iterator it);
		void handlePubRelTimeout(CMqttEvent* pEvent, session_states_t::iterator it);
		void handlePubComp(CMqttEvent* pEvent, session_states_t::iterator it);
		void handlePubCompTimeout(CMqttEvent* pEvent, session_states_t::iterator it);

		std::shared_ptr<web::websockets::client::websocket_callback_client> m_client;
		CTimer m_keepAliveTimer;
		CTimer m_pingRespTimer;
		static const DWORD m_responseTime_ms = 10000;
		CConnectEvent::Params m_connectParams;
	};

	template<class event_t>
	event_t* getEvent(CMqttEvent* e)
	{
		event_t* ret = dynamic_cast<event_t*>(e);
		_ASSERTE(ret);
		return ret;
	}

	template<class packet_t>
	packet_t* getReceivedPacket(CMqttEvent* e)
	{
		CReceivedPacketEvent* p = getEvent<CReceivedPacketEvent>(e);
		packet_t* packet = dynamic_cast<packet_t*>(p->packet().get());
		_ASSERTE(packet);
		return packet;
	}

} // namespace MQTT
