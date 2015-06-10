#pragma once

#include "maquette/maquette.h"
#include "EnumValue.h"
#include "Events.h"

namespace MQTT {

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
		// Variable header and Payload
		data_t m_remainings;
		static uint16_t m_packetIdentifier;

		template<size_t size>
		size_t encodeRemainingLength(byte(& encoded)[size], size_t lengthToEncode) const;
	};

	class CReceivedPacket : virtual public CPacket {
	public:
		static CReceivedPacket* create(const data_t& data);
		const CMqttEvent& event() const { return m_event; };

		// Decoded Remaining Length in received data
		size_t remainingLength;

	protected:
		CReceivedPacket(const Type& type) : CPacket(type) {};	// Constructor for class derived from both CPacketToSend and CReceivedPacket
		CReceivedPacket(const Type& type, const data_t& data, CMqttEvent::Value event)
			: CPacket(type), m_data(data), m_event(event) {};
		bool parse();

		// pos = top position of remainings(Variavle header)
		virtual bool parse(size_t pos) { return true; };
		size_t decodeRemainingLength(size_t& pos) const;
		uint16_t makeWord(size_t pos) const { return MAKEWORD(m_data[pos + 1], m_data[pos]); };

		const data_t m_data;
		const CMqttEvent m_event;
	};

	class CConnectPacket : public CPacketToSend {
	public:
		CConnectPacket() : CPacket(Type::CONNECT), CPacketToSend(m_type) {};

		virtual const data_t& data() {
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
		CConnAckPacket(const data_t& data) : CPacket(Type::CONNACK), CReceivedPacket(m_type, data, CMqttEvent::ConnAck) {};

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
		virtual bool parse(size_t pos) {
			returnCode = m_data[pos];
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
		CSubAckPacket(const data_t& data) : CPacket(Type::SUBACK), CReceivedPacket(m_type, data, CMqttEvent::SubAck) {};

		uint16_t packetIdentifire;
		byte qos;
		bool isAccepted;

		virtual bool parse(size_t pos) {
			packetIdentifire = makeWord(pos); pos += 2;
			const byte& returnCode = m_data[pos];
			qos = returnCode & 0x03;
			isAccepted = (returnCode & 0x80) == 0;
			return true;
		};
	};

	class CPublishPacket : public CPacketToSend, public CReceivedPacket {
	public:
		// Constructor for packet to send
		CPublishPacket(const std::string& topic, const data_t& payload)
			: CPacket(Type::PUBLISH)
			, CPacketToSend(m_type), CReceivedPacket(m_type)
			, topic(topic), payload(payload) {};

		virtual const data_t& data()
		{
			add(topic);
			// if(0 < QoS) add(m_packetIdentifier);
			add(payload);
			return CPacketToSend::data();
		};

		// Constructor for received packet
		CPublishPacket(const data_t& data)
			: CPacket(Type::PUBLISH), CPacketToSend(m_type), CReceivedPacket(m_type, data, CMqttEvent::Published) {};

		virtual bool parse(size_t pos) {
			size_t size = makeWord(pos); pos += 2;		// Size of Topic string
			topic.assign((LPCSTR)&m_data[pos], size); pos += size;
			// if(0 < QoS) paketIdentifier = WORD(m_data[pos]); pos += 2;
			payload.assign(m_data.begin() + pos, m_data.end());
			return true;
		};

		std::string topic;
		data_t payload;
	};
}
