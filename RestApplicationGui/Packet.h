#pragma once

namespace MQTT {

	class CPacket {
	public:
		// MQTT Packet type
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

			// Property of each type
			typedef struct _Property {
				byte flagBit;				// Flags specific to each MQTT Control Packet type
				bool sendToServer;			// true if Packet with this type is to send to server
				bool receiveFromServer;		// true if Packet with this type is to receive from server
				LPCSTR name;				// name string
			} Property;

			// Constructor from Value type
			Type(Value value) : m_value(value) { _ASSERTE(value < Value::_Count); };
			// Constructor from received data
			Type(byte value) : m_value((Value)(value >> 4)) { _ASSERTE(m_value < Value::_Count); };

			// Encode value to byte to send to server with Flag bits
			inline byte encode(byte flagBit = 0) const { return (m_value << 4) | property().flagBit | flagBit; };
			inline operator byte() const { return m_value; };
			inline operator LPCSTR() const { return property().name; };
			inline const Property& property() const { return m_properties[m_value]; };

		protected:
			static const Property m_properties[Type::_Count];
			Value m_value;
		};

		typedef std::vector<byte> data_t;
		typedef data_t::size_type size_t;
		static const size_t remainingLengthMax = 268435455;

		CPacket(Type type);
		virtual ~CPacket();

		inline Type type() const { return m_type; };

	protected:
		Type m_type;
		data_t m_data;
	};

	class CPacketToSend : virtual public CPacket {
	public:
		CPacketToSend(Type type, size_t size = 100);

		void add(const byte* pData, size_t size);
		void add(const std::string& str);
		void add(int num);
		const data_t& data() const;

	protected:
		// Variable header and Payload
		data_t m_variableData;
	};

	class CReceivedPacket : virtual public CPacket {
	public:
		CReceivedPacket(Type type);

		size_t remainingLength() const;
	};
}
