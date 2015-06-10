#pragma once

#include "maquette/maquette.h"
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

}
