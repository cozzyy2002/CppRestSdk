#pragma once

namespace MQTT {

	class CPacket {
	public:
		class Type {
		public:
			typedef enum _Type {
				Reserved_0,			// Reserved
				CONNECT,			// Client request to connect to Server
				CONNACK,			// Connect acknowledgment
				PUBLISH,			// Publish message
				PUBACK,				// Publish acknowledgment
				PUBREC,				// Publish received(assured delivery part 1)
				PUBREL,				// Publish release(assured delivery part 2)
				PUBCOMP,			// Publish complete(assured delivery part 3)
				SUBSCRIBE,			// Client subscribe request
				SUBACK,				// Subscribe acknowledgment
				UNSUBSCRIBE,		// Unsubscribe request
				UNSUBACK,			// Unsubscribe acknowledgment
				PINGREQ,			// PING request
				PINGRESP,			// PING response
				DISCONNECT,			// Client is disconnecting
				Reserved_1,			// Reserved
				_Count,			// Count of element of this enumeration
			} Value;

			Type(Value value) : m_value(value) {};
			operator byte() { return m_value; };

		protected:
			Value m_value;
		};
		static const size_t remainingLengthMax = 268435455;

		CPacket(Type type, size_t size = 100);
		virtual ~CPacket();

		typedef std::vector<byte> data_t;
		const data_t data() const;
		Type type() const { return m_type; };
		size_t remainingLength() const;

	protected:
		Type m_type;
		byte m_flagBit;
		std::vector<data_t> m_data;
	};

	class CPacketToSend : public CPacket {
	public:
	};
}
