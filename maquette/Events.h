#pragma once

#include "maquette/maquette.h"
#include "EnumValue.h"

namespace MQTT {

	class CReceivedPacket;

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
			KeepAlive,			// Timeout of Keep Alive timer
			PingResp,			// MQTT PINGRESP is received
			PingTimeout,		// MQTT PINGRESP is not received before timeout
			_Count				// Count of enum value for boundary check
		} Value;

		explicit CMqttEvent(Value value = Value::Connect) : CEnumValue(value) {};

	protected:
		static const LPCSTR m_valueNames[];
		virtual const LPCSTR* getValueNames() const { return m_valueNames; };
	};

	class CConnectEvent : public CMqttEvent {
	public:
		CConnectEvent(LPCTSTR serverUrl, int keepAlive)
			: CMqttEvent(Value::Connect)
			, serverUrl(serverUrl), keepAlive(keepAlive)
		{};

		const utility::string_t serverUrl;
		const int keepAlive;
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
		CReceivedPacketEvent(MQTT::CReceivedPacket* packet);
		virtual ~CReceivedPacketEvent();

		inline MQTT::CReceivedPacket* packet() const { return m_packet; };

	protected:
		MQTT::CReceivedPacket* m_packet;
	};

}
