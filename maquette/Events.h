#pragma once

#include "maquette/maquette.h"
#include "EnumValue.h"

namespace MQTT {

	class CMqttEvent : public CEnumValue {
	public:
		typedef enum _Value {
			_ConnectionEvent,
			Connect = _ConnectionEvent,	// Request to connect to MQTT broker
			Disconnect,					// Request to disconnect MQTT broker
			ConnectedSocket,			// websocket_client::connect() task is completed
			ClosedSocket,				// websocket_client::close_handler is called
			ConnAck,					// MQTT CONNACK is received
			ConnAckTimeout,				// MQTT CONNACK is not received before timeout
			KeepAlive,					// Timeout of Keep Alive timer
			PingResp,					// MQTT PINGRESP is received
			PingRespTimeout,			// MQTT PINGRESP is not received before timeout
			_ConnectionEventCount,

			_SessionEvent = _ConnectionEventCount,
			Subscribe = _SessionEvent,	// Request to subscribe
			SubAck,						// MQTT SUBACK is received
			Unsubscribe,				// Request to Unsubscribe
			UnsubAck,					// MQTT UNSUBACK is received
			Publish,					// Requtest to publish message
			Published,					// MQTT PUBLISH is received
			PubAck,						// MQTT PUBACK is received
			PubRec,						// MQTT PUBRec is received
			PubRel,						// MQTT PUBREL is received
			PubComp,					// MQTT PUBCOMP is received
			SessionTimeout,				// MQTT response is not received before timeout

			// Following events are return value of event handler
			// that specifies how modify current session state in CMaquetteImpl::m_sessionStates
			NoMoreEvent,			// Remove current state 
			PreserveCurrentState,	// Do not modify current state

			_SessionEventCount,
			_Count = _SessionEventCount,	// Count of enum value for boundary check
		} Value;

		// NOTE: Default constructor is called as member of CReceivedPacket
		CMqttEvent(Value value = (Value)0) : CEnumValue(value) {};

		inline bool isConnectionEvent() const { return (m_value < _SessionEvent); };
		inline size_t getEventIndex() const { return isConnectionEvent() ? m_value : m_value - _SessionEvent; };

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
			: CMqttEvent(Value::Subscribe)
		{
			m_params.push_back({topic.c_str(), qos});
		};

		const Params& params() const { return m_params; };

	protected:
		Params m_params;
	};

	class CUnsubscribeEvent : public CMqttEvent {
	public:
		typedef std::vector<std::string> Params;

		CUnsubscribeEvent(const std::string& topic)
			: CMqttEvent(Value::Unsubscribe)
		{
			m_params.push_back(topic.c_str());
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

		CPublishEvent(const std::string& topic, const data_t& payload, QOS qos = QOS_0, bool retain = false)
			: CMqttEvent(Value::Publish), m_params({topic, payload, qos, retain}) {};

		const Params& params() const { return m_params; };

	protected:
		Params m_params;
	};

	class CReceivedPacket;
	class CReceivedPacketEvent : public CMqttEvent {
	public:
		CReceivedPacketEvent(CReceivedPacket* packet);
		virtual ~CReceivedPacketEvent();

		inline std::shared_ptr<CReceivedPacket>& packet() { return m_packet; };

	protected:
		std::shared_ptr<CReceivedPacket> m_packet;
	};

	class CSessionTimeoutEvent : public CMqttEvent {
	public:
		CSessionTimeoutEvent(uint16_t packetIdentifire)
			: CMqttEvent(Value::SessionTimeout), packetIdentifire(packetIdentifire) {};

		const uint16_t packetIdentifire;
	};
}
