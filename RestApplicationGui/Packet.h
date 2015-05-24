#pragma once

#include "EnumValue.h"

namespace MQTT {
	typedef std::vector<byte> data_t;

	class CReceivedPacket;

	class CPacket {
	public:
		// MQTT Packet type
		class Type : public CEnumValue {
		public:
			typedef enum _Value : byte {
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
				std::function<CReceivedPacket* (const data_t&)> createPacket;
			} Property;

			// Constructor from Value type
			Type(Value value) : CEnumValue(value), property(m_properties[m_value]) {};
			// Constructor from received data
			// NOTE: Validate value using validate() method before create this object
			Type(byte value) : CEnumValue((Value)(value >> 4)), property(m_properties[m_value]) {};

			inline static bool checkValue(byte value) { return ((value >> 4) < Value::_Count); };

			// Encode value to byte to send to server with Flag bits
			// NOTE: To decode received byte, use Type(byte) constructor
			inline byte encode(byte flagBit = 0) const { return (m_value << 4) | property.flagBit | flagBit; };
			inline virtual LPCSTR toString() const { return property.name; };
			const Property& property;

		protected:
			static const Property m_properties[Type::_Count];
		};

		typedef data_t::size_type size_t;
		static const size_t remainingLengthMax = 268435455;

		inline const Type& type() const { return m_type; };
		inline virtual LPCSTR toString() const { return m_type.toString(); };

	protected:
		CPacket(const Type& type) : m_type(type) {};
		virtual ~CPacket() {};

		const Type m_type;
		data_t m_data;
	};

	class CPacketToSend : virtual public CPacket {
	public:
		CPacketToSend(const Type& type, size_t size = 100);

		void add(const void* pData, size_t size);
		void add(const data_t& data) { add(data.data(), data.size()); };
		void add(const std::string& str) { add((uint16_t)str.size()); add(str.c_str(), str.size()); };
		void add(byte num) { add((uint32_t)num, 1); }
		void add(uint16_t num) { add((uint32_t)num, 2); }
		void add(uint32_t num, size_t size = sizeof(uint32_t));
		virtual const data_t& data();

	protected:
		// Returns size of m_variableData encoded using MQTT Remaining Length encoding scheme
		size_t remainingLength() const { return m_variableData.size(); };

		// Variable header and Payload
		data_t m_variableData;
		static uint16_t m_packetIdentifier;
	};

	class CReceivedPacket : virtual public CPacket {
	public:
		static CReceivedPacket* create(const data_t& data);

		// Returns decoded Remaining Length in received data
		size_t remainingLength;

	protected:
		CReceivedPacket(const Type& type, const data_t& data)
			: CPacket(type), m_data(data) {};
		bool parse();
		virtual bool parseInternal() { return true; };

		const data_t m_data;
	};

	class CConnectPacket : public CPacketToSend {
	public:
		CConnectPacket() : CPacket(Type::CONNECT), CPacketToSend(m_type) {};

		virtual const data_t& data() {
			// TODO: Add Variable header and Payload to m_variableData
			add("MQTT");			// Protocol Name
			add((byte)4);			// Protocol Level
			add((byte)2);			// Connect Flags(Clean Session = 1)
			add((uint16_t)60);		// Keep Alive(second)
			add("KYclient");		// Client Identifier

			return CPacketToSend::data();
		};
	};

	class CDisconnectPacket : public CPacketToSend {
	public:
		CDisconnectPacket() : CPacket(Type::DISCONNECT), CPacketToSend(m_type) {};
	};

	class CConnAckPacket : public CReceivedPacket {
	public:
		CConnAckPacket(const data_t& data) : CPacket(Type::CONNACK), CReceivedPacket(m_type, data) {};

		class CReturnCode : public CEnumValue {
		public:
			typedef enum _Value : byte {
				ConnectionAccepted,
				UnacceptableProtocolVersion,
				IdentifierRejected,
				ServerUnavailable,
				BadUserNameOrPassword,
				NotAuthorized,
				_Count,
			} Value;

			CReturnCode(byte b = 0) : CEnumValue((Value)b) {};

		protected:
			static const LPCSTR m_valueNames[];
			virtual const LPCSTR* getValueNames() const { return m_valueNames; };
		};

		CReturnCode returnCode;
		bool isAccepted;

	protected:
		virtual bool parseInternal() {
			returnCode = m_data[3];
			isAccepted = (returnCode == CReturnCode::ConnectionAccepted);
			return true;
		};
	};

	class CSubscribePacket : public CPacketToSend {
	public:
		CSubscribePacket(const std::string& topic)
			: CPacket(Type::SUBSCRIBE), CPacketToSend(m_type), m_topic(topic) {};

		virtual const data_t& data() {
			add(m_packetIdentifier);
			add(m_topic);
			add((byte)0);		// QoS

			return CPacketToSend::data();
		};

		std::string m_topic;
	};

	class CSubAckPacket : public CReceivedPacket {
	public:
		CSubAckPacket(const data_t& data) : CPacket(Type::SUBACK), CReceivedPacket(m_type, data) {};

		uint16_t packetIdentifire;
		byte qos;
		bool isAccepted;

		virtual bool parseInternal() {
			packetIdentifire = MAKEWORD(m_data[3], m_data[2]);
			qos = m_data[4] & 0x03;
			isAccepted = (m_data[4] & 0x80) == 0;
			return true;
		};
	};

	class CPublishPacket : public CReceivedPacket {
	public:
		CPublishPacket(const data_t& data) : CPacket(Type::PUBLISH), CReceivedPacket(m_type, data) {};

		virtual bool parseInternal() {
			size_t size = MAKEWORD(m_data[3], m_data[2]);
			topic.assign((LPCSTR)&m_data[4], size);
			payload.assign(m_data.begin() + 4 + size, m_data.end());
			return true;
		};

		std::string topic;
		data_t payload;
	};
}
