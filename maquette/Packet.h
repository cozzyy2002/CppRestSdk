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
			Type(Value value) : CEnumValue(value), m_property(&m_properties[m_value]) {};
			// Constructor from received data
			// NOTE: Validate value using validate() method before create this object
			Type(byte value) : CEnumValue((Value)(value >> 4)), m_property(&m_properties[m_value]) {};

			inline static bool checkValue(byte value) { return ((value >> 4) < Value::_Count); };

			// Encode value to byte to send to server with Flag bits
			// NOTE: To decode received byte, use Type(byte) constructor
			inline byte encode(byte flagBit) const { return (m_value << 4) | m_property->flagBit | flagBit; };
			inline virtual LPCSTR toString() const { return m_property->name; };
			inline static LPCSTR toString(byte value) { return m_properties[value >> 4].name; }
			const Property& property() const { return *m_property; };

		protected:
			const Property* m_property;
			static const Property m_properties[Type::_Count];
		};

		typedef data_t::size_type size_t;
		static const size_t remainingLengthMax = 268435455;

		inline const data_t& data() const { return m_data; };
		inline const Type& type() const { return m_type; };
		inline uint16_t packetIdentifier() const { return m_packetIdentifier; };
		inline virtual LPCSTR toString() const { return m_type.toString(); };

		static const uint16_t UNUSED_PACKET_IDENTIFIER = 0;

	protected:
		CPacket(const Type& type) : m_type(type) {};
		CPacket(const Type& type, const data_t& data) : m_type(type), m_data(data) {};
		virtual ~CPacket() {};

		const Type m_type;
		data_t m_data;
		uint16_t m_packetIdentifier;
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
		virtual const data_t& encode() { return encode(0); };

	protected:
		// Variable header and Payload
		data_t m_remainings;
		static uint16_t g_packetIdentifier;

		template<size_t size>
		size_t encodeRemainingLength(byte(& encoded)[size], size_t lengthToEncode) const;

		const data_t& encode(byte flagBit);
	};

	class CReceivedPacket : virtual public CPacket {
	public:
		static CReceivedPacket* create(const data_t& data);
		const CMqttEvent& event() const { return m_event; };

		// Decoded Remaining Length in received data
		size_t remainingLength;

	protected:
		CReceivedPacket(const Type& type) : CPacket(type) {};	// Constructor for class derived from both CPacketToSend and CReceivedPacket
		CReceivedPacket(const Type& type, CMqttEvent::Value event)
			: CPacket(type), m_event(event) {};
		bool decode();

		// pos = top position of remainings(Variavle header)
		virtual bool decode(size_t& pos) { return true; };
		size_t decodeRemainingLength(size_t& pos) const;
		uint16_t makeWord(size_t& pos) const;
		void checkLength(size_t pos, size_t size) const;

		const CMqttEvent m_event;
	};

	/**
	 * Base class for packets used to send and receive,
	 * and for packets consist of
	 *   Fixed header,
	 *   Packet Identifier in Variable Header
	 *   and no Paylead.
	 */
	class CSimplePacket : public CPacketToSend, public CReceivedPacket {
	protected:
		// Constructor for packet to send
		CSimplePacket(const Type& type, uint16_t packetIdentifier = UNUSED_PACKET_IDENTIFIER)
			: CPacket(type), CPacketToSend(m_type), CReceivedPacket(m_type)
		{
			m_packetIdentifier = packetIdentifier;
		};

		virtual const data_t& encode();

		// Constructor for received packet
		CSimplePacket(const Type& type, CMqttEvent::Value event)
			: CPacket(type), CPacketToSend(type), CReceivedPacket(m_type, event) {};

		bool decode(size_t& pos, bool usePacketIdentifier);
	};

	class CConnectPacket : public CPacketToSend {
	public:
		CConnectPacket(const CConnectEvent::Params& params)
			: CPacket(Type::CONNECT), CPacketToSend(m_type), m_params(params) {};

		virtual const data_t& encode();

	protected:
		const CConnectEvent::Params m_params;
	};

	class CDisconnectPacket : public CPacketToSend {
	public:
		CDisconnectPacket() : CPacket(Type::DISCONNECT), CPacketToSend(m_type) {};
	};

	class CConnAckPacket : public CReceivedPacket {
	public:
		CConnAckPacket(const data_t& data) : CPacket(Type::CONNACK, data), CReceivedPacket(m_type, CMqttEvent::ConnAck) {};

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
		virtual bool decode(size_t& pos);
	};

	class CSubscribePacket : public CPacketToSend {
	public:
		CSubscribePacket(const CSubscribeEvent::Params& params)
			: CPacket(Type::SUBSCRIBE), CPacketToSend(m_type), m_params(params) {};

		virtual const data_t& encode();

	protected:
		CSubscribeEvent::Params m_params;
	};

	class CSubAckPacket : public CReceivedPacket {
	public:
		CSubAckPacket(const data_t& data) : CPacket(Type::SUBACK, data), CReceivedPacket(m_type, CMqttEvent::SubAck) {};

		byte qos;
		bool isAccepted;

		virtual bool decode(size_t& pos);
	};

	class CUnsubscribePacket : public CPacketToSend {
	public:
		CUnsubscribePacket(const CUnsubscribeEvent::Params& params)
			: CPacket(Type::UNSUBSCRIBE), CPacketToSend(m_type), m_params(params) {};

		virtual const data_t& encode();

	protected:
		CUnsubscribeEvent::Params m_params;
	};

	class CUnsubAckPacket : public CReceivedPacket {
	public:
		CUnsubAckPacket(const data_t& data) : CPacket(Type::UNSUBACK, data), CReceivedPacket(m_type, CMqttEvent::UnsubAck) {};

		virtual bool decode(size_t& pos);
	};

	class CPublishPacket : public CPacketToSend, public CReceivedPacket {
	public:
		// Constructor for packet to send
		CPublishPacket(const CPublishEvent::Params& params)
			: CPacket(Type::PUBLISH)
			, CPacketToSend(m_type), CReceivedPacket(m_type)
			, m_params(params), m_dup(false) {};

		void setDup(bool dup = true);

		virtual const data_t& encode();

		// Constructor for received packet
		CPublishPacket(const data_t& data)
			: CPacket(Type::PUBLISH, data), CPacketToSend(m_type), CReceivedPacket(m_type, CMqttEvent::Published) {};

		virtual bool decode(size_t& pos);

		const CPublishEvent::Params& params() const { return m_params; };

	protected:
		CPublishEvent::Params m_params;
		bool m_dup;
	};

	class CPubAckPacket : public CSimplePacket {
	public:
		CPubAckPacket(uint16_t packetIdentifier) : CPacket(Type::PUBACK), CSimplePacket(m_type, packetIdentifier) {};
		CPubAckPacket(const data_t& data) : CPacket(Type::PUBACK, data), CSimplePacket(m_type, CMqttEvent::PubAck) {};

		virtual bool decode(size_t& pos) { return CSimplePacket::decode(pos, true); };
	};

	class CPubRecPacket : public CSimplePacket {
	public:
		CPubRecPacket(uint16_t packetIdentifier) : CPacket(Type::PUBREC), CSimplePacket(m_type, packetIdentifier) {};
		CPubRecPacket(const data_t& data) : CPacket(Type::PUBREC, data), CSimplePacket(m_type, CMqttEvent::PubRec) {};

		virtual bool decode(size_t& pos) { return CSimplePacket::decode(pos, true); };
	};

	class CPubRelPacket : public CSimplePacket {
	public:
		CPubRelPacket(uint16_t packetIdentifier) : CPacket(Type::PUBREL), CSimplePacket(m_type, packetIdentifier) {};
		CPubRelPacket(const data_t& data) : CPacket(Type::PUBREL, data), CSimplePacket(m_type, CMqttEvent::PubRel) {};

		virtual bool decode(size_t& pos) { return CSimplePacket::decode(pos, true); };
	};

	class CPubCompPacket : public CSimplePacket {
	public:
		CPubCompPacket(uint16_t packetIdentifier) : CPacket(Type::PUBCOMP), CSimplePacket(m_type, packetIdentifier) {};
		CPubCompPacket(const data_t& data) : CPacket(Type::PUBCOMP, data), CSimplePacket(m_type, CMqttEvent::PubComp) {};

		virtual bool decode(size_t& pos) { return CSimplePacket::decode(pos, true); };
	};

	class CPingReqPacket : public CPacketToSend {
	public:
		CPingReqPacket() : CPacket(Type::PINGREQ), CPacketToSend(m_type) {};
	};

	class CPingRespPacket : public CReceivedPacket {
	public:
		CPingRespPacket(const data_t& data) : CPacket(Type::PINGRESP, data), CReceivedPacket(m_type, CMqttEvent::PingResp) {};
	};
}
