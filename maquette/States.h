#pragma once

#include "maquette/maquette.h"
#include "Packet.h"
#include "EnumValue.h"

namespace MQTT {

	class CMqttState : public CEnumValue {
	public:
		typedef enum _Value {
			_Minimum = -1,
			Initial,				// WebSocket is not connected or is disconnected
			ConnectingSocket,		// Waiting for WebSocket to connect
			ConnectingBroker,		// Waiting for CONNACK MQTT control packet
			Connected,				// Connected to MQTT broker but not subscribed
			Disconnecting,			// Disconnecting
			_Count					// Count of enum value for boundary check
		} Value;

		CMqttState() : CEnumValue(Initial) {};
		CMqttState(Value value) : CEnumValue(value) {};

	protected:
		static const LPCSTR m_valueNames[];
		virtual const LPCSTR* getValueNames() const { return m_valueNames; };
	};

	class CMqttEvent : public CEnumValue {
	public:
		typedef enum _Value {
			_Minimum = -1,
			Connect,			// Request to connect to MQTT broker
			Disconnect,			// Request to disconnect MQTT broker
			ConnectedSocket,	// websocket_client::connect() task is completed
			ClosedSocket,		// websocket_client::close_handler is called
			ConnAck,			// MQTT CONNACK is received
			Subscribe,			// Request to subscribe
			SubAck,				// MQTT SUBACK is received
			Publish,			// Requtest to publish message
			Published,			// MQTT PUBLISH is received
			PingTimer,			// Timeout of PING timer
			_Count				// Count of enum value for boundary check
		} Value;

		explicit CMqttEvent(Value value = Value::Connect) : CEnumValue(value) {};

	protected:
		static const LPCSTR m_valueNames[];
		virtual const LPCSTR* getValueNames() const { return m_valueNames; };
	};

	class CConnectEvent : public CMqttEvent {
	public:
		CConnectEvent(LPCTSTR serverUrl)
			: CMqttEvent(Value::Connect), serverUrl(serverUrl)
		{};

		const utility::string_t serverUrl;
	};

	class CSubscribeEvent : public CMqttEvent {
	public:
		CSubscribeEvent(const std::string& topic)
			: CMqttEvent(Value::Subscribe), topic(topic)
		{};

		const std::string topic;
	};

	class CPublishEvent : public CMqttEvent {
	public:
		CPublishEvent(const std::string& topic, const MQTT::data_t& payload)
			: CMqttEvent(Value::Publish), topic(topic), payload(payload)
		{};

		const std::string topic;
		const MQTT::data_t payload;
	};

	class CReceivedPacketEvent : public CMqttEvent {
	public:
		CReceivedPacketEvent(MQTT::CReceivedPacket* packet) : m_packet(packet)
		{
			switch(packet->type()) {
			case MQTT::CPacket::Type::CONNACK:
				m_value = ConnAck;
				break;
			case MQTT::CPacket::Type::SUBACK:
				m_value = SubAck;
				break;
			case MQTT::CPacket::Type::PUBLISH:
				m_value = Published;
				break;
			case MQTT::CPacket::Type::PINGRESP:
				// No associated event.
				break;
			default:
				break;
			}
		};

		virtual ~CReceivedPacketEvent()
		{
			delete m_packet;
		};

		inline MQTT::CReceivedPacket* packet() const { return m_packet; };

	protected:
		MQTT::CReceivedPacket* m_packet;
	};

}
