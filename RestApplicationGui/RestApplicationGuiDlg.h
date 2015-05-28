
// RestApplicationGuiDlg.h : header file
//

#pragma once
#include "afxwin.h"

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

	template<class event_t>
	event_t* getEvent(CMqttEvent* e)
	{
		event_t* ret = dynamic_cast<event_t*>(e);
		_ASSERTE(ret);
		return ret;
	}

	template<class packet_t>
	packet_t* getReceivedPacket(CMqttEvent* e)
	{
		CReceivedPacketEvent* p = getEvent<CReceivedPacketEvent>(e);
		packet_t* packet = dynamic_cast<packet_t*>(p->packet());
		_ASSERTE(packet);
		return packet;
	}


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
