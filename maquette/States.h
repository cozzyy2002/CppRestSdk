#pragma once

#include "maquette/maquette.h"
#include "EnumValue.h"
#include "Packet.h"

namespace MQTT {

	class CConnectionState : public CEnumValue {
	public:
		typedef enum _Value {
			Initial = 0,			// WebSocket is not connected or is disconnected
			ConnectingSocket,		// Waiting for WebSocket to connect
			ConnectingBroker,		// Waiting for CONNACK MQTT control packet
			Connected,				// Connected
			Disconnecting,			// Disconnecting
			_Count					// Count of enum value for boundary check
		} Value;

		CConnectionState(Value value) : CEnumValue(value) {};

	protected:
		static const LPCSTR m_valueNames[];
		virtual const LPCSTR* getValueNames() const { return m_valueNames; };
	};

	class CSessionState {
	public:

		CSessionState(CPacket::Type type, CPacketToSend* packet)
			: responseType(type), packet(packet) {};

		virtual ~CSessionState() {};

		CSessionState handleResponse(CMqttEvent* pEvent);
		CSessionState handleTimeout(CMqttEvent* pEvent);

		const CPacket::Type responseType;
		const std::shared_ptr<CPacketToSend> packet;
	};

	class CNoMoreEventState : public CSessionState {
	public:
		CNoMoreEventState() : CSessionState(CMqttEvent::NoMoreEvent, NULL) {};
	};

	class CPreserveCurrentState : public CSessionState {
	public:
		CPreserveCurrentState() : CSessionState(CMqttEvent::PreserveCurrentState, NULL) {};
	};
}
