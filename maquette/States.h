#pragma once

#include "maquette/maquette.h"
#include "EnumValue.h"
#include "Packet.h"
#include "Timer.h"

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
		CSessionState()
			: responseType(CPacket::Type::Reserved_0), packetSent(NULL) {};

		CSessionState(CPacket::Type type, CPacketToSend* packet)
			: responseType(type), packetSent(packet), timer(new CTimer()) {};

		virtual ~CSessionState() {
			if(timer.get()) timer->cancel();
		};

		CPacket::Type responseType;
		std::shared_ptr<CPacketToSend> packetSent;
		std::shared_ptr<CTimer> timer;
	};
}
