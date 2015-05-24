
// RestApplicationGuiDlg.h : header file
//

#pragma once
#include "afxwin.h"
#include "Packet.h"
#include "EnumValue.h"

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

class CMqttEvent : public CEnumValue {
public:
	typedef enum _Value {
		_Minimum = -1,
		Connect,			// Request to connect to MQTT broker
		Disconnect,			// Request to disconnect MQTT broker
		ConnectedSocket,	// websocket_client::connect() task is completed
		ClosedSocket,		// websocket_client::close_handler is called
		ConnAck,			// MQTT CONNACK is received
		Subscribe,			// Request to subscribe
		SubAck,				// MQTT SUBACK is received
		Publish,			// Requtest to publish message
		Published,			// MQTT PUBLISH is received
		PingTimer,			// Timeout of PING timer
		_Count				// Count of enum value for boundary check
	} Value;

	explicit CMqttEvent(Value value = Value::Connect) : CEnumValue(value) {};

protected:
	static const LPCSTR m_valueNames[];
	virtual const LPCSTR* getValueNames() const { return m_valueNames; };
};

class CSubscribeEvent : public CMqttEvent {
public:
	CSubscribeEvent(const std::string& topic)
		: CMqttEvent(Value::Subscribe), topic(topic) {};

	const std::string topic;
};

class CPublishEvent : public CMqttEvent {
public:
	CPublishEvent(const std::string& topic, const MQTT::data_t& payload)
		: CMqttEvent(Value::Publish), topic(topic), payload(payload) {};

	const std::string topic;
	const MQTT::data_t payload;
};

class CReceivedPacketEvent : public CMqttEvent {
public:
	CReceivedPacketEvent(MQTT::CReceivedPacket* packet) : m_packet(packet) {
		switch(packet->type()) {
		case MQTT::CPacket::Type::CONNACK:
			m_value = ConnAck;
			break;
		case MQTT::CPacket::Type::SUBACK:
			m_value = SubAck;
			break;
		case MQTT::CPacket::Type::PUBLISH:
			m_value = Published;
			break;
		case MQTT::CPacket::Type::PINGRESP:
			// No associated event.
			break;
		default:
			break;
		}
	};

	virtual ~CReceivedPacketEvent() {
		delete m_packet;
	};

	MQTT::CReceivedPacket* m_packet;
};

// CRestApplicationGuiDlg dialog
class CRestApplicationGuiDlg : public CDialogEx
{
protected:
	CMqttState m_mqttState;

	typedef CMqttState (CRestApplicationGuiDlg::*event_handler_t)(CMqttEvent* pEvent);
	static const event_handler_t state_event_table[CMqttEvent::Value::_Count][CMqttState::Value::_Count];

	void postEvent(CMqttEvent::Value value);
	void postEvent(CMqttEvent* pEvent);
	void send(MQTT::CPacketToSend& packet, bool wait = false);
	void receive(const web::websockets::client::websocket_incoming_message& msg);
	CMqttState handleConnect(CMqttEvent* pEvent);
	CMqttState handleDisconnect(CMqttEvent* pEvent);
	CMqttState handleDisconnectSocket(CMqttEvent* pEvent);
	CMqttState handleConnectedSocket(CMqttEvent* pEvent);
	CMqttState handleClosedSocket(CMqttEvent* pEvent);
	CMqttState handleConnAck(CMqttEvent* pEvent);
	CMqttState handleSubscribe(CMqttEvent* pEvent);
	CMqttState handleSubAck(CMqttEvent* pEvent);
	CMqttState handlePublish(CMqttEvent* pEvent);
	CMqttState handlePublished(CMqttEvent* pEvent);
	CMqttState handlePingTimer(CMqttEvent* pEvent);

	CMqttState handleIgnore(CMqttEvent* pEvent);
	CMqttState handleFatal(CMqttEvent* pEvent);

	std::shared_ptr<web::websockets::client::websocket_callback_client> m_client;
	bool canClose();
	void setConnectStatus();
	void log(LPCTSTR format, ...);
	CString m_LogText;
	enum {
		WM_USER_ = WM_USER,
		WM_USER_EVENT,
		WM_USER_SET_TEXT,
	};

	enum ConnectStatus {
		ConnectStatusIdle,
		ConnectStatusClosed,
		ConnectStatusConnecting,
		ConnectStatusConnected,
		ConnectStatusClosing,
	};
	ConnectStatus m_ConnectStatus;


// Construction
public:
	CRestApplicationGuiDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_RESTAPPLICATIONGUI_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnClickedButtonConnect();
	afx_msg void OnClickedButtonDisconnect();
	afx_msg void OnClickedButtonSubscibe();
	afx_msg void OnClickedButtonUnsubscibe();
	afx_msg void OnClickedButtonPublish();
	CString m_ServerUrl;
//	CString m_Log;
	CString m_Payload;
	CString m_Topic;

protected:
	//	afx_msg LRESULT OnUserLog(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnUserSetText(WPARAM wParam, LPARAM lParam);
public:
	afx_msg void OnClose();
	CString m_ConnectStatusText;
	virtual void OnOK();
	virtual void OnCancel();
protected:
	afx_msg LRESULT OnUserEvent(WPARAM wParam, LPARAM lParam);
};
