
// RestApplicationGuiDlg.h : header file
//

#pragma once
#include "afxwin.h"

class CMqttState {
public:
	typedef enum _VALUE {
		Initial,				// WebSocket is not connected or is disconnected
		ConnectingSocket,		// Waiting for WebSocket to connect
		ConnectingBroker,		// Waiting for CONNACK MQTT control packet
		Connected,				// Connected to MQTT broker but not subscribed
		Subscribing,			// Waiting for SUBACK MQTT control packet
		Subscribed,				// Subscribed
		Disconnecting,			// Disconnecting
		_Count					// Count of enum value for boundary check
	} Value;

	CMqttState() : m_value(Initial) {};
	CMqttState(Value value) : m_value(value) {};
	inline operator int() const { return (int)m_value; };
	inline bool isValid() const { return (0 <= *this) && (*this < Value::_Count); };
	operator LPSTR() const;

protected:
	Value m_value;
};

class CMqttEvent {
public:
	typedef enum _TYPE {
		Connect,			// Request to connect to MQTT broker
		Disconnect,			// Request to disconnect MQTT broker
		ConnectedSocket,	// websocket_client::connect() task is completed
		ClosedSocket,		// websocket_client::close_handler is called
		ConnectAccepted,	// MQTT CONNACK(Return Code == 0) is received
		ConnectRejected,	// MQTT CONNACK(Return Code != 0) is received
		SubscribeSuccess,	// MQTT SUBACK(Return Code != 0x80) is received
		SubscribeFailure,	// MQTT SUBACK(Return Code == 0x80) is received
		Publish,			// Requtest to publish message
		Published,			// MQTT PUBLISH is received
		PingTimer,			// Timeout of PING timer
		_Count				// Count of enum value for boundary check
	} Type;

	explicit CMqttEvent(Type type) : m_type(type) {};
	virtual ~CMqttEvent() {};
	inline operator int() const { return (int)m_type; };
	inline bool isValid() const { return (0 <= *this) && (*this < Type::_Count); };
	operator LPSTR() const;

protected:
	Type m_type;
};

// CRestApplicationGuiDlg dialog
class CRestApplicationGuiDlg : public CDialogEx
{
protected:
	CMqttState m_mqttState;

	typedef CMqttState (CRestApplicationGuiDlg::*event_handler_t)(CMqttEvent* pEvent);
	static const event_handler_t state_event_table[CMqttEvent::Type::_Count][CMqttState::_Count];

	void postEvent(CMqttEvent::Type type);
	void postEvent(CMqttEvent* pEvent);
	CMqttState handleConnect(CMqttEvent* pEvent);
	CMqttState handleDisconnect(CMqttEvent* pEvent);
	CMqttState handleConnectedSocket(CMqttEvent* pEvent);
	CMqttState handleClosedSocket(CMqttEvent* pEvent);
	CMqttState handleConnectAccepted(CMqttEvent* pEvent);
	CMqttState handleConnectRejected(CMqttEvent* pEvent);
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
