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
			Type(Value value) : CEnumValue(value) { _ASSERTE(isValid()); };
			// Constructor from received data
			// NOTE: Validate value using validate() method before create this object
			Type(byte value) : CEnumValue((Value)(value >> 4)) { _ASSERTE(isValid()); };

			inline static bool checkValue(byte value) { return ((value >> 4) < Value::_Count); };

			// Encode value to byte to send to server with Flag bits
			// NOTE: To decode received byte, use Type(byte) constructor
			inline byte encode(byte flagBit = 0) const { return (m_value << 4) | property().flagBit | flagBit; };
			inline virtual LPCSTR toString() const { return property().name; };
			inline const Property& property() const { return m_properties[m_value]; };

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
	};

	class CReceivedPacket : virtual public CPacket {
	public:
		static CReceivedPacket* create(const data_t& data);

		// Returns decoded Remaining Length in received data
		size_t remainingLength() const;
		//virtual const data_t& payload() const;

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

	protected:
		virtual bool parseInternal() { returnCode = m_data[3]; return true; };
	};
}
