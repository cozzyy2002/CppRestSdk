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
	: m_callback(callback), m_state(CMqttState::Initial)
{
	_ASSERTE(m_callback);
}


CMaquetteImpl::~CMaquetteImpl()
{}

void CMaquetteImpl::connect(LPCTSTR serverUrl)
{
	postEvent(new CConnectEvent(serverUrl));
}

void CMaquetteImpl::disconnect()
{
	postEvent(CMqttEvent::Disconnect);
}

void CMaquetteImpl::subscribe(LPCTSTR topic)
{
	postEvent(new CSubscribeEvent(to_utf8string(topic)));
}

void CMaquetteImpl::publish(LPCTSTR topic, const data_t& payload)
{
	postEvent(new CPublishEvent(to_utf8string(topic), payload));
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
	CMqttEvent* pEvent = (CMqttEvent*)lParam;
	LOG4CPLUS_TRACE(logger, "OnUserEvent(): state=" << m_state.toString() << ", event=" << pEvent->toString());

	if(!m_state.isValid()) {
		LOG4CPLUS_FATAL(logger, "CMqttState is out of range: " << (byte)m_state);
		return 0;
	}
	if(!pEvent->isValid()) {
		LOG4CPLUS_FATAL(logger, "CMqttEvent is out of range: " << (byte)pEvent);
		return 0;
	}

	event_handler_t handler = state_event_table[*pEvent][m_state];
	m_state = (this->*handler)(pEvent);
	delete pEvent;

	LOG4CPLUS_TRACE(logger, "OnUserEvent(): new state=" << m_state.toString());

	return 0;
}

#define H(x) &CMaquetteImpl::handle##x
#define _IGNORE H(Ignore)
#define _FATAL H(Fatal)
#define _NOT_IMPL _FATAL

const CMaquetteImpl::event_handler_t CMaquetteImpl::state_event_table[CMqttEvent::Value::_Count][CMqttState::_Count] =
{
	//	Initial					ConnectingSocket		ConnectingBroker		Connected				Disconnecting
	{	H(Connect),				_IGNORE,				_IGNORE,				_IGNORE,				H(Connect)	},		// Connect
	{	_IGNORE,				H(DisconnectSocket),	H(DisconnectSocket),	H(Disconnect),			_IGNORE		},		// Disconnect
	{	_FATAL,					H(ConnectedSocket),		_FATAL,					_FATAL,					_FATAL		},		// ConnectedSocket
	{	_IGNORE,				H(ClosedSocket),		H(ClosedSocket),		H(ClosedSocket),		H(ClosedSocket)	},	// ClosedSocket
	{	_FATAL,					_FATAL,					H(ConnAck),				_FATAL,					_IGNORE		},		// ConnAck
	{	_IGNORE,				_IGNORE,				_IGNORE,				H(Subscribe),			_IGNORE		},		// Subscribe
	{	_IGNORE,				_IGNORE,				_IGNORE,				H(SubAck),				_IGNORE		},		// SubAck
	{	_IGNORE,				_IGNORE,				_IGNORE,				H(Publish),				_IGNORE		},		// Publish
	{	_IGNORE,				_IGNORE,				_IGNORE,				H(Published),			_IGNORE		},		// Published
	{	_NOT_IMPL,				_NOT_IMPL,				_NOT_IMPL,				_NOT_IMPL,				_NOT_IMPL	},		// PingTimer
};

void CMaquetteImpl::send(CPacketToSend& packet, bool wait /*= false*/)
{
	// Copy data to buffer.
	// And wait for buffer to cmplete copying to prevent data from being deleted.
	producer_consumer_buffer<byte> buf;
	const data_t& data = packet.data();
	size_t size = buf.putn(data.data(), data.size()).get();

	// Send message to the server
	// See https://casablanca.codeplex.com/wikipage?title=Web%20Socket&referringTitle=Documentation
	websocket_outgoing_message msg;
	msg.set_binary_message(buf.create_istream(), size);
	auto task = m_client->send(msg)
		.then([](pplx::task<void> task) {
			try {
				task.get();
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
			LOG4CPLUS_DEBUG(logger, "Received " << size << "bytes");
			CReceivedPacket* packet = CReceivedPacket::create(*data);
			if(packet) {
				LOG4CPLUS_DEBUG(logger, "Received: " << typeid(*packet).name() << ", Remaining Length=" << packet->remainingLength);
				postEvent(new CReceivedPacketEvent(packet));
			}
		});
}

CMqttState CMaquetteImpl::handleConnect(CMqttEvent* pEvent)
{
	CConnectEvent* p = getEvent<CConnectEvent>(pEvent);
	string_t serverUrl = p->serverUrl;
	LOG4CPLUS_INFO(logger, U("Connecting: '") << serverUrl.c_str() << U("'"));

	websocket_client_config config;
	config.add_subprotocol(U("mqtt"));
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
		.then([this, serverUrl]() {
			LOG4CPLUS_INFO(logger, U("Connected: '") << serverUrl.c_str() << U("'"));
			postEvent(CMqttEvent::ConnectedSocket);
		});

	return CMqttState::ConnectingSocket;
}

CMqttState CMaquetteImpl::handleDisconnect(CMqttEvent* pEvent)
{
	// Send Disconnect MQTT message then disconnect socket
	CDisconnectPacket packet;
	send(packet, true);

	return handleDisconnectSocket(pEvent);
}

CMqttState CMaquetteImpl::handleDisconnectSocket(CMqttEvent* pEvent)
{
	// Disconnect socket
	m_client->close();
	return CMqttState::Disconnecting;
}

CMqttState CMaquetteImpl::handleConnectedSocket(CMqttEvent* pEvent)
{
	CConnectPacket packet;
	send(packet);
	return CMqttState::ConnectingBroker;
}

CMqttState CMaquetteImpl::handleClosedSocket(CMqttEvent* pEvent)
{
	m_client->close();
	m_callback->onConnectionClosed();
	return CMqttState::Initial;
}

CMqttState CMaquetteImpl::handleConnAck(CMqttEvent* pEvent)
{
	CConnAckPacket* packet = getReceivedPacket<CConnAckPacket>(pEvent);

	if(packet->isAccepted) {
		LOG4CPLUS_INFO(logger, "MQTT CONNECT accepted.");
		m_callback->onConnAck(true);
		return CMqttState::Connected;
	} else {
		LOG4CPLUS_ERROR(logger, "MQTT CONNECT rejected: Return code=" << packet->returnCode.toString());
		m_client->close();
		m_callback->onConnAck(false);
		return CMqttState::Initial;
	}
}

CMqttState CMaquetteImpl::handleSubscribe(CMqttEvent* pEvent)
{
	CSubscribeEvent* p = getEvent<CSubscribeEvent>(pEvent);
	CSubscribePacket packet(p->topic);
	send(packet);
	return m_state;
}

CMqttState CMaquetteImpl::handleSubAck(CMqttEvent* pEvent)
{
	CSubAckPacket* packet = getReceivedPacket<CSubAckPacket>(pEvent);

	if(packet->isAccepted) {
		LOG4CPLUS_INFO(logger, "MQTT SUBSCRIBE accepted.");
		m_callback->onSubAck(true);
	} else {
		LOG4CPLUS_ERROR(logger, "MQTT SUBSCRIBE rejected");
		m_callback->onSubAck(false);
	}
	return m_state;
}

CMqttState CMaquetteImpl::handlePublish(CMqttEvent* pEvent)
{
	CPublishEvent* p = getEvent<CPublishEvent>(pEvent);
	CPublishPacket packet(p->topic, p->payload);
	send(packet);
	return m_state;
}

CMqttState CMaquetteImpl::handlePublished(CMqttEvent* pEvent)
{
	CPublishPacket* packet = getReceivedPacket<CPublishPacket>(pEvent);

	std::string text;
	text.assign((LPCSTR)packet->payload.data(), packet->payload.size());
	LOG4CPLUS_INFO(logger, "MQTT PUBLISH topic='" << packet->topic.c_str() << "', payload='" << text.c_str() << "'");

	m_callback->onPublished(to_utf16string(packet->topic).c_str(), packet->payload);
	return m_state;
}

CMqttState CMaquetteImpl::handlePingTimer(CMqttEvent* pEvent)
{
	return m_state;
}

CMqttState CMaquetteImpl::handleIgnore(CMqttEvent* pEvent)
{
	LOG4CPLUS_TRACE(logger, "handleIgnore(): event=" << pEvent->toString());
	return m_state;
}

CMqttState CMaquetteImpl::handleFatal(CMqttEvent* pEvent)
{
	LOG4CPLUS_FATAL(logger, "handleFatal(): event=" << pEvent->toString());
	return m_state;
}
