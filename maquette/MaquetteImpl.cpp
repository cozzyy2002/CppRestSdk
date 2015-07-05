#include "stdafx.h"
#include "MaquetteImpl.h"

using namespace MQTT;
using namespace web::websockets::client;
using namespace concurrency::streams;
using namespace utility;
using namespace utility::conversions;

static log4cplus::Logger logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("MQTT.CMaquetteImpl"));

CMaquette* createMaquette(IMaquetteCallback* callback)
{
	return new CMaquetteImpl(callback);
}

CMaquetteImpl::CMaquetteImpl(IMaquetteCallback* callback)
	: m_callback(callback), m_state(CConnectionState::Initial), m_keepAliveTimer(true)
{
	_ASSERTE(m_callback);
}


CMaquetteImpl::~CMaquetteImpl()
{}

void CMaquetteImpl::connect(LPCTSTR serverUrl, LPCTSTR clientId, DWORD keepAlive)
{
	postEvent(new CConnectEvent(serverUrl, clientId, keepAlive));
}

void CMaquetteImpl::disconnect()
{
	postEvent(CMqttEvent::Disconnect);
}

void CMaquetteImpl::subscribe(LPCTSTR topic, MQTT::QOS qos)
{
	postEvent(new CSubscribeEvent(to_utf8string(topic), qos));
}

void CMaquetteImpl::publish(LPCTSTR topic, const data_t& payload, MQTT::QOS qos, bool retain)
{
	postEvent(new CPublishEvent(to_utf8string(topic), payload, qos, retain));
}

void CMaquetteImpl::postEvent(CMqttEvent::Value value)
{
	postEvent(new CMqttEvent(value));
}

void CMaquetteImpl::postEvent(CMqttEvent* pEvent)
{
	if(!m_callback->postMessage(0, (LPARAM)pEvent)) {
		LOG4CPLUS_FATAL(logger, "postMessage() failed. error=" << ::GetLastError());
		_ASSERTE(!"postMessage() failed");
	}
}

LRESULT CMaquetteImpl::onUserEvent(WPARAM wParam, LPARAM lParam)
{
	std::shared_ptr<CMqttEvent> pEvent((CMqttEvent*)lParam);
	LOG4CPLUS_TRACE(logger, "OnUserEvent(): state=" << m_state.toString() << ", event=" << pEvent->toString());

	if(!pEvent->isValid()) {
		LOG4CPLUS_FATAL(logger, "CMqttEvent is out of range: " << pEvent->value<CMqttEvent::Value>());
		return 0;
	}

	if(pEvent->isConnectionEvent()) {
		if(!m_state.isValid()) {
			LOG4CPLUS_FATAL(logger, "CConnectionState is out of range: " << (byte)m_state);
			return 0;
		}

		connection_event_handler_t handler = state_event_table[*pEvent][m_state];
		m_state = (this->*handler)(pEvent.get());
		LOG4CPLUS_TRACE(logger, "OnUserEvent(): new state=" << m_state.toString());
	} else {
		const SessionEvent& event = sesstion_event_table[pEvent->getEventIndex()];
		if(event.requestHandler) {
			// Send PUBLISH, SUBSCRIBE, UNSUBSCRIBE
			LOG4CPLUS_TRACE(logger, "Handling request: " << pEvent->toString());
			(this->*event.requestHandler)(pEvent.get());
		} else {
			session_states_t::iterator it = m_sessionStates.end();
			if(event.getPacketIdentifier) {
				uint16_t packetIdentifier = event.getPacketIdentifier(pEvent.get());
				it = m_sessionStates.find(packetIdentifier);
			}
			if(pEvent->value<CMqttEvent::Value>() != CMqttEvent::SessionTimeout) {
				// TODO:
				//   Check if type of received packet equals to CSessionState::responseType

				if(event.responseHandler) {
					// Received SUBACK, UNSUBACK, PUBLISH, PUBACK, PUBREC, PUBREL, PUBCOMP
					LOG4CPLUS_TRACE(logger, "Handling response: " << pEvent->toString());
					(this->*event.responseHandler)(pEvent.get(), it);
				} else {
					LOG4CPLUS_WARN(logger, "No response handler: event=" << pEvent->toString());
					_ASSERTE(!"No response handler");
				}
			} else {
				// Response timeout
				if(event.timeoutHandler) {
					LOG4CPLUS_INFO(logger, "Handling timeout: " << pEvent->toString()
											<< ", expect " << ((it != m_sessionStates.end()) ? it->second.responseType.toString() : ""));
					(this->*event.timeoutHandler)(pEvent.get(), it);
				} else {
					LOG4CPLUS_WARN(logger, "No timeout handler: event=" << pEvent->toString());
					_ASSERTE(!"No timeout handler");
				}
			}
		}
	}

	return 0;
}

#define H(x) &CMaquetteImpl::handle##x
#define _IGNORE H(Ignore)
#define _FATAL H(Fatal)
#define _NOT_IMPL _FATAL

const CMaquetteImpl::connection_event_handler_t CMaquetteImpl::state_event_table[CMqttEvent::_ConnectionEventCount][CConnectionState::_Count] =
{
	//	Initial					ConnectingSocket		ConnectingBroker		Connected				Disconnecting
	{	H(Connect),				_IGNORE,				_IGNORE,				_IGNORE,				_IGNORE		},		// Connect
	{	_IGNORE,				H(DisconnectSocket),	H(DisconnectSocket),	H(Disconnect),			_IGNORE		},		// Disconnect
	{	_FATAL,					H(ConnectedSocket),		_FATAL,					_FATAL,					_FATAL		},		// ConnectedSocket
	{	_IGNORE,				H(ClosedSocket),		H(ClosedSocket),		H(ClosedSocket),		H(ClosedSocket)	},	// ClosedSocket
	{	_FATAL,					_FATAL,					H(ConnAck),				_FATAL,					_IGNORE		},		// ConnAck
	{	_IGNORE,				_IGNORE,				_NOT_IMPL,				_IGNORE,				_IGNORE		},		// ConnAckTimeout
	{	_IGNORE,				_IGNORE,				_IGNORE,				H(KeepAlive),			_IGNORE		},		// KeepAlive
	{	_IGNORE,				_IGNORE,				_IGNORE,				H(PingResp),			_IGNORE		},		// PingResp
	{	_IGNORE,				_IGNORE,				_IGNORE,				H(PingRespTimeout),		_IGNORE		},		// PingRespTimeout
};

const CMaquetteImpl::SessionEvent CMaquetteImpl::sesstion_event_table[CMqttEvent::_SessionEventCount] = {
	//	Request handler
	//			Response handler, Timeout handler,	Getter packet identifier from received packet
	{	H(Subscribe)		},
	{	NULL,	H(SubAck),		H(SubAckTimeout),	[](CMqttEvent* e) { return getReceivedPacket<CSubAckPacket>(e)->packetIdentifier(); }	},
	{	H(Unsubscribe)		},
	{	NULL,	H(UnsubAck),	H(UnsubAckTimeout),	[](CMqttEvent* e) { return getReceivedPacket<CUnsubAckPacket>(e)->packetIdentifier(); }	},
	{	H(Publish)			},
	{	NULL,	H(Published)	},	// Published event(Received PUBLISH packet) has no timeout handler
	{	NULL,	H(PubAck),		H(PubAckTimeout),	[](CMqttEvent* e) { return getReceivedPacket<CPubAckPacket>(e)->packetIdentifier(); }	},
	{	NULL,	H(PubRec),		H(PubRecTimeout),	[](CMqttEvent* e) { return getReceivedPacket<CPubRecPacket>(e)->packetIdentifier(); }	},
	{	NULL,	H(PubRel),		H(PubRelTimeout),	[](CMqttEvent* e) { return getReceivedPacket<CPubRelPacket>(e)->packetIdentifier(); }	},
	{	NULL,	H(PubComp),		H(PubCompTimeout),	[](CMqttEvent* e) { return getReceivedPacket<CPubCompPacket>(e)->packetIdentifier(); }	},
};

void CMaquetteImpl::send(CPacketToSend& packet, bool wait /*= false*/)
{
	// Copy data to buffer.
	// And wait for buffer to cmplete copying to prevent data from being deleted.
	producer_consumer_buffer<byte> buf;
	const data_t& data = packet.data();
	LOG4CPLUS_DEBUG(logger, "Sending " << CPacket::Type::toString(data[0]) << ": " << data.size() << " byte\n" << CUtils::dump(data).c_str());
	size_t size = buf.putn(data.data(), data.size()).get();

	// Send message to the server
	// See https://casablanca.codeplex.com/wikipage?title=Web%20Socket&referringTitle=Documentation
	websocket_outgoing_message msg;
	msg.set_binary_message(buf.create_istream(), size);
	auto task = m_client->send(msg)
		.then([this, packet](pplx::task<void> task) {
			try {
				task.get();

				if(packet.type() != CPacket::Type::PINGREQ) {
					m_keepAliveTimer.restart();
				}
			} catch(const websocket_exception& e) {
				LOG4CPLUS_ERROR(logger, "Exception while sending WebSocket message: " << e.what());
			}
		});
	if(wait) task.wait();
}

void CMaquetteImpl::receive(const web::websockets::client::websocket_incoming_message& msg)
{
	size_t size = msg.length();
	auto data = std::make_shared<data_t>();
	data->resize(size);
	msg.body().streambuf().getn(&data->at(0), size)
		.then([this, data](size_t size) {
			LOG4CPLUS_DEBUG(logger, "Received "
				<< (data->size() ? CPacket::Type::toString(data->at(0)) : "") << ": "
				<< data->size() << " byte\n" << CUtils::dump(*data).c_str());
			CReceivedPacket* packet = CReceivedPacket::create(*data);
			if(packet) {
				LOG4CPLUS_DEBUG(logger, "Received: " << typeid(*packet).name() << ", Remaining Length=" << packet->remainingLength);
				postEvent(new CReceivedPacketEvent(packet));
			}
		});
}

CConnectionState CMaquetteImpl::handleConnect(CMqttEvent* pEvent)
{
	CConnectEvent* p = getEvent<CConnectEvent>(pEvent);
	m_connectParams = p->params();
	const string_t& serverUrl = m_connectParams.serverUrl;
	LOG4CPLUS_INFO(logger, U("Connecting: '") << serverUrl.c_str() << U("'"));

	websocket_client_config config;
	config.add_subprotocol(U("mqttv3.1"));
	m_client.reset(new websocket_callback_client(config));
	m_client->set_message_handler(std::bind(&CMaquetteImpl::receive, this, std::placeholders::_1));
	m_client->set_close_handler([this](websocket_close_status status, const string_t& reason, const std::error_code& error) {
		LOG4CPLUS_INFO(logger, 
			U("close_handler: status=") << (int)status
			<< U(", reason='") << reason.c_str()
			<< U("', error='") << to_string_t(error.message().c_str()).c_str() << U("'"));
		postEvent(CMqttEvent::ClosedSocket);
	});

	m_client->connect(serverUrl)
		.then([this]() {
			LOG4CPLUS_INFO(logger, U("Connected: '") << m_connectParams.serverUrl.c_str() << U("'"));
			postEvent(CMqttEvent::ConnectedSocket);
		});

	return CConnectionState::ConnectingSocket;
}

CConnectionState CMaquetteImpl::handleDisconnect(CMqttEvent* pEvent)
{
	// Send Disconnect MQTT message then disconnect socket
	CDisconnectPacket packet;
	send(packet, true);

	return handleDisconnectSocket(pEvent);
}

CConnectionState CMaquetteImpl::handleDisconnectSocket(CMqttEvent* pEvent)
{
	m_keepAliveTimer.cancel();

	// Disconnect socket
	m_client->close();
	return CConnectionState::Disconnecting;
}

CConnectionState CMaquetteImpl::handleConnectedSocket(CMqttEvent* pEvent)
{
	CConnectPacket packet(m_connectParams);
	send(packet);
	return CConnectionState::ConnectingBroker;
}

CConnectionState CMaquetteImpl::handleClosedSocket(CMqttEvent* pEvent)
{
	m_keepAliveTimer.cancel();
	m_client->close();
	m_callback->onConnectionClosed();
	return CConnectionState::Initial;
}

CConnectionState CMaquetteImpl::handleConnAck(CMqttEvent* pEvent)
{
	CConnAckPacket* packet = getReceivedPacket<CConnAckPacket>(pEvent);

	if(packet->isAccepted) {
		LOG4CPLUS_INFO(logger, "MQTT CONNECT accepted.");
		m_callback->onConnAck(true);

		startEventTimer(m_keepAliveTimer, m_connectParams.keepAlive * 1000, CMqttEvent::KeepAlive);
		return CConnectionState::Connected;
	} else {
		LOG4CPLUS_ERROR(logger, "MQTT CONNECT rejected: Return code=" << packet->returnCode.toString());
		m_client->close();
		m_callback->onConnAck(false);
		return CConnectionState::Initial;
	}
}

void CMaquetteImpl::handleSubscribe(CMqttEvent* pEvent)
{
	CSubscribeEvent* p = getEvent<CSubscribeEvent>(pEvent);
	CSubscribePacket* packet = new CSubscribePacket(p->params());
	send(*packet);

	m_sessionStates[packet->packetIdentifier()] = CSessionState(CPacket::Type::SUBACK, packet);
}

void CMaquetteImpl::handleSubAck(CMqttEvent* pEvent, session_states_t::iterator it)
{
	CSubAckPacket* packet = getReceivedPacket<CSubAckPacket>(pEvent);

	if(packet->isAccepted) {
		LOG4CPLUS_INFO(logger, "MQTT SUBSCRIBE accepted.");
		m_callback->onSubAck(true);
	} else {
		LOG4CPLUS_ERROR(logger, "MQTT SUBSCRIBE rejected");
		m_callback->onSubAck(false);
	}

	m_sessionStates.erase(it);
}

#define _SESSION_HANDLER_NOT_IMPL LOG4CPLUS_WARN(logger, "Not implemented: " __FUNCTION__)

void CMaquetteImpl::handleSubAckTimeout(CMqttEvent* pEvent, session_states_t::iterator it)
{
	_SESSION_HANDLER_NOT_IMPL;
}

void CMaquetteImpl::handleUnsubscribe(CMqttEvent* pEvent)
{
	_SESSION_HANDLER_NOT_IMPL;
}

void CMaquetteImpl::handleUnsubAck(CMqttEvent* pEvent, session_states_t::iterator it)
{
	_SESSION_HANDLER_NOT_IMPL;
}

void CMaquetteImpl::handleUnsubAckTimeout(CMqttEvent* pEvent, session_states_t::iterator it)
{
	_SESSION_HANDLER_NOT_IMPL;
}

void CMaquetteImpl::handlePublish(CMqttEvent* pEvent)
{
	CPublishEvent* p = getEvent<CPublishEvent>(pEvent);
	CPublishPacket* packet = new CPublishPacket(p->params());
	send(*packet);

	switch(p->params().qos) {
	default:
		LOG4CPLUS_FATAL(logger, "Unknown QoS: " << p->params().qos);
		_ASSERTE(!"Unknown QoS");
		// go through
	case QOS_0:
		delete packet;
		break;
	case QOS_1:
		m_sessionStates[packet->packetIdentifier()] = CSessionState(CPacket::Type::PUBACK, packet);
		break;
	case QOS_2:
		m_sessionStates[packet->packetIdentifier()] = CSessionState(CPacket::Type::PUBREC, packet);
		break;
	}
}

void CMaquetteImpl::handlePublished(CMqttEvent* pEvent, session_states_t::iterator /* not used */)
{
	CPublishPacket* packet = getReceivedPacket<CPublishPacket>(pEvent);
	const CPublishEvent::Params& params = packet->params();

	switch(packet->params().qos) {
	default:
		LOG4CPLUS_FATAL(logger, "Unknown QoS: " << packet->params().qos);
		_ASSERTE(!"Unknown QoS");
		// go through
	case QOS_0:
		break;
	case QOS_1:
		{
			CPubAckPacket p(packet->packetIdentifier());
			send(p);
		}
	case QOS_2:
		{
			CPubRecPacket* p = new CPubRecPacket(packet->packetIdentifier());
			send(*p);
			m_sessionStates[packet->packetIdentifier()] = CSessionState(CPacket::Type::PUBREL, p);
		}
	}

	m_callback->onPublished(to_utf16string(params.topic).c_str(), params.payload);
}

void CMaquetteImpl::handlePubAck(CMqttEvent* pEvent, session_states_t::iterator it)
{
	_SESSION_HANDLER_NOT_IMPL;
}

void CMaquetteImpl::handlePubAckTimeout(CMqttEvent* pEvent, session_states_t::iterator it)
{
	_SESSION_HANDLER_NOT_IMPL;
}

void CMaquetteImpl::handlePubRec(CMqttEvent* pEvent, session_states_t::iterator it)
{
	_SESSION_HANDLER_NOT_IMPL;
}

void CMaquetteImpl::handlePubRecTimeout(CMqttEvent* pEvent, session_states_t::iterator it)
{
	_SESSION_HANDLER_NOT_IMPL;
}

void CMaquetteImpl::handlePubRel(CMqttEvent* pEvent, session_states_t::iterator it)
{
	_SESSION_HANDLER_NOT_IMPL;
}

void CMaquetteImpl::handlePubRelTimeout(CMqttEvent* pEvent, session_states_t::iterator it)
{
	_SESSION_HANDLER_NOT_IMPL;
}

void CMaquetteImpl::handlePubComp(CMqttEvent* pEvent, session_states_t::iterator it)
{
	_SESSION_HANDLER_NOT_IMPL;
}

void CMaquetteImpl::handlePubCompTimeout(CMqttEvent* pEvent, session_states_t::iterator it)
{
	_SESSION_HANDLER_NOT_IMPL;
}

CConnectionState CMaquetteImpl::handleKeepAlive(CMqttEvent* pEvent)
{
	CPingReqPacket packet;
	send(packet);
	startEventTimer(m_pingRespTimer, m_responseTime_ms, CMqttEvent::PingRespTimeout);
	return m_state;
}

CConnectionState CMaquetteImpl::handlePingResp(CMqttEvent* pEvent)
{
	if(m_pingRespTimer.isActive()) {
		m_pingRespTimer.cancel();
	} else {
		handleIgnore(pEvent);
	}
	return m_state;
}

CConnectionState CMaquetteImpl::handlePingRespTimeout(CMqttEvent* pEvent)
{
	LOG4CPLUS_ERROR(logger, "PingResp timeout.");
	return handleDisconnectSocket(pEvent);
}

CConnectionState CMaquetteImpl::handleIgnore(CMqttEvent* pEvent)
{
	LOG4CPLUS_TRACE(logger, "handleIgnore(): event=" << pEvent->toString());
	return m_state;
}

CConnectionState CMaquetteImpl::handleFatal(CMqttEvent* pEvent)
{
	LOG4CPLUS_FATAL(logger, "handleFatal(): event=" << pEvent->toString());
	return m_state;
}
