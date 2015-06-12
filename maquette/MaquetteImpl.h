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
		virtual void subscribe(LPCTSTR topic);
		virtual void publish(LPCTSTR topic, const data_t& payload);
		virtual LRESULT onUserEvent(WPARAM wParam, LPARAM lParam);

	protected:
		CMqttState m_state;
		IMaquetteCallback* m_callback;

		typedef CMqttState (CMaquetteImpl::*event_handler_t)(CMqttEvent* pEvent);
		static const event_handler_t state_event_table[CMqttEvent::Value::_Count][CMqttState::Value::_Count];

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

		CMqttState handleConnect(CMqttEvent* pEvent);
		CMqttState handleDisconnect(CMqttEvent* pEvent);
		CMqttState handleDisconnectSocket(CMqttEvent* pEvent);
		CMqttState handleConnectedSocket(CMqttEvent* pEvent);
		CMqttState handleClosedSocket(CMqttEvent* pEvent);
		CMqttState handleConnAck(CMqttEvent* pEvent);
		CMqttState handleSubscribe(CMqttEvent* pEvent);
		CMqttState handleSubAck(CMqttEvent* pEvent);
		CMqttState handlePublish(CMqttEvent* pEvent);
		CMqttState handlePublished(CMqttEvent* pEvent);
		CMqttState handleKeepAlive(CMqttEvent* pEvent);

		CMqttState handleIgnore(CMqttEvent* pEvent);
		CMqttState handleFatal(CMqttEvent* pEvent);

		std::shared_ptr<web::websockets::client::websocket_callback_client> m_client;
		CTimer m_keepAliveTimer;
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
			packet_t* packet = dynamic_cast<packet_t*>(p->packet());
			_ASSERTE(packet);
			return packet;
		}
	};

} // namespace MQTT
