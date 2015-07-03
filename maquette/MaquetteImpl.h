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
		virtual void publish(LPCTSTR topic, const data_t& payload, QOS qos, bool retain);
		virtual LRESULT onUserEvent(WPARAM wParam, LPARAM lParam);

	protected:
		CConnectionState m_state;
		IMaquetteCallback* m_callback;

		typedef CConnectionState (CMaquetteImpl::*event_handler_t)(CMqttEvent* pEvent);
		static const event_handler_t state_event_table[CMqttEvent::Value::_Count][CConnectionState::Value::_Count];

		typedef std::map<uint16_t, CSessionState> session_states_t;
		session_states_t m_sessionStates;

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

		CSessionState handleSubscribe(CMqttEvent* pEvent);
		CSessionState handleSubAck(CMqttEvent* pEvent);
		CSessionState handlePublish(CMqttEvent* pEvent);
		CSessionState handlePublished(CMqttEvent* pEvent);

		std::shared_ptr<web::websockets::client::websocket_callback_client> m_client;
		CTimer m_keepAliveTimer;
		CTimer m_pingRespTimer;
		static const DWORD m_responseTime_ms = 10000;
		CConnectEvent::Params m_connectParams;

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
	};

} // namespace MQTT
