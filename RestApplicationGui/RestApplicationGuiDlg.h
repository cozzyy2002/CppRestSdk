
// RestApplicationGuiDlg.h : header file
//

#pragma once
#include "afxwin.h"

class CState {
public:
	typedef enum {
		Initial,
		ConnectingSocket,		// Waiting for WebSocket to connect
		ConnectingBroker,		// Waiting for CONNACK MQTT control packet
		Connected,				// Connected to MQTT broker but not subscribed
		Subscribing,			// Waiting for SUBACK MQTT control packet
		Subscribed,				// Subscribed
		Disconnected,			// Disconnected
		_Count					// Count of enum value for boundary check
	} Value;
};

class CEvent {
public:
	class Type {
	public:
		typedef enum {
			Connect,			// Request to connect to MQTT broker
			Disconnect,			// Request to disconnect MQTT broker
			ConnectedSocket,	// websocket_client::connect() task is completed
			ClosedSocket,		// websocket_client::close_handler is called
			ConnAccepted,		// MQTT CONNACK(Return Code == 0) is received
			ConnRejected,		// MQTT CONNACK(Return Code != 0) is received
			SubscribeSuccess,	// MQTT SUBACK(Return Code != 0x80) is received
			SubscribeFailure,	// MQTT SUBACK(Return Code == 0x80) is received
			Publish,			// Requtest to publish message
			Published,			// MQTT PUBLISH is received
			PingTimer,			// Timeout of PING timer
			_Count				// Count of enum value for boundary check
		};
	};

	explicit CEvent(Type type) : m_Type(type) {};
	virtual ~CEvent() {};

protected:
	Type m_Type;
};

// CRestApplicationGuiDlg dialog
class CRestApplicationGuiDlg : public CDialogEx
{
protected:
	typedef CState::Value(*CRestApplicationGuiDlg::event_handler_t)(CEvent* pEvent);
	static const event_handler_t state_event_table[CEvent::Type::_Count][CState::_Count];

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
