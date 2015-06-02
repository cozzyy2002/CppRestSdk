
// RestApplicationGuiDlg.h : header file
//

#pragma once
#include "afxwin.h"
#include "maquette/maquette.h"

// CRestApplicationGuiDlg dialog
class CRestApplicationGuiDlg : public CDialogEx, public MQTT::IMaquetteCallback
{
protected:
	typedef enum _ConnectStatus {
		ConnectStatusIdle,
		ConnectStatusClosed,
		ConnectStatusConnecting,
		ConnectStatusConnected,
		ConnectStatusClosing,
	} ConnectStatus;

	std::shared_ptr<MQTT::CMaquette> m_maquette;
	bool canClose();
	void setConnectStatus(ConnectStatus status);
	void setConnectStatus();
	void log(LPCTSTR format, ...);
	CString m_LogText;

	enum {
		WM_USER_ = WM_USER,
		WM_USER_EVENT,
		WM_USER_SET_TEXT,
	};

	// Implementation of IMaquetteCallback
	virtual BOOL postMessage(WPARAM wParam, LPARAM lParam);

	virtual void onConnAck(bool accepted);
	virtual void onConnectionClosed();
	virtual void onSubAck(bool accepted);
	virtual void onPublished(LPCTSTR topic, const MQTT::data_t& payload);

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
