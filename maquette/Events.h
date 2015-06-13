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
			PingRespTimeout,	// MQTT PINGRESP is not received before timeout
			_Count				// Count of enum value for boundary check
		} Value;

		explicit CMqttEvent(Value value = Value::Connect) : CEnumValue(value) {};

	protected:
		static const LPCSTR m_valueNames[];
		virtual const LPCSTR* getValueNames() const { return m_valueNames; };
	};

	class CConnectEvent : public CMqttEvent {
	public:
		typedef struct _Params {
			utility::string_t serverUrl;
			utility::string_t clientId;
			int keepAlive;
		} Params;

		CConnectEvent(const utility::string_t& serverUrl, const utility::string_t& clientId, DWORD keepAlive)
			: CMqttEvent(Value::Connect), m_params({serverUrl, clientId, keepAlive}) {};

		const Params& params() const { return m_params; };

	protected:
		const Params m_params;
	};

	class CSubscribeEvent : public CMqttEvent {
	public:
		typedef struct _Topic {
			std::string topic;
			QOS qos;
		} Topic;

		typedef std::vector<Topic> Params;

		CSubscribeEvent(const std::string& topic, QOS qos)
			: CMqttEvent(Value::Subscribe) {
			m_params.push_back({topic.c_str(), qos});
		};

		const Params& params() const { return m_params; };

	protected:
		Params m_params;
	};

	class CPublishEvent : public CMqttEvent {
	public:
		typedef struct _Params {
			std::string topic;
			data_t payload;
			QOS qos;
			bool retain;
			// Used by class derived from CReceivedPacket
			uint16_t packetIdentifier;
		} Params;

		CPublishEvent(const std::string& topic, const data_t& payload, QOS qos, bool retain)
			: CMqttEvent(Value::Publish), m_params({topic, payload, qos, retain}) {};

		const Params& params() const { return m_params; };

	protected:
		const Params m_params;
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
