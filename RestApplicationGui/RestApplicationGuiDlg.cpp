
// RestApplicationGuiDlg.cpp : implementation file
//

#include "stdafx.h"
#include "RestApplicationGui.h"
#include "RestApplicationGuiDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using namespace MQTT;

static log4cplus::Logger logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("RestApplicationGuiDlg"));

// CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// Dialog Data
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CRestApplicationGuiDlg dialog



CRestApplicationGuiDlg::CRestApplicationGuiDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CRestApplicationGuiDlg::IDD, pParent)
	, m_ServerUrl(_T(""))
	, m_LogText(_T(""))
	, m_Payload(_T(""))
	, m_Topic(_T(""))
	, m_ConnectStatus(ConnectStatusIdle)
	, m_ConnectStatusText(_T("Initial"))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CRestApplicationGuiDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT_SERVER_URL, m_ServerUrl);
	//  DDX_Text(pDX, IDC_EDIT_LOG, m_Log);
	DDX_Text(pDX, IDC_EDIT_PAYLOAD, m_Payload);
	DDX_Text(pDX, IDC_EDIT_TOPIC, m_Topic);
	DDX_Text(pDX, IDC_EDIT_CONNET_STATUS, m_ConnectStatusText);
}

BEGIN_MESSAGE_MAP(CRestApplicationGuiDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_CONNECT, &CRestApplicationGuiDlg::OnClickedButtonConnect)
	ON_BN_CLICKED(IDC_BUTTON_DISCONNECT, &CRestApplicationGuiDlg::OnClickedButtonDisconnect)
	ON_BN_CLICKED(IDC_BUTTON_SUBSCIBE, &CRestApplicationGuiDlg::OnClickedButtonSubscibe)
	ON_BN_CLICKED(IDC_BUTTON_UNSUBSCIBE, &CRestApplicationGuiDlg::OnClickedButtonUnsubscibe)
	ON_BN_CLICKED(IDC_BUTTON_PUBLISH, &CRestApplicationGuiDlg::OnClickedButtonPublish)
//	ON_MESSAGE(WM_USER_Log, &CRestApplicationGuiDlg::OnUserLog)
	ON_MESSAGE(WM_USER_SET_TEXT, &CRestApplicationGuiDlg::OnUserSetText)
	ON_WM_CLOSE()
	ON_MESSAGE(WM_USER_EVENT, &CRestApplicationGuiDlg::OnUserEvent)
END_MESSAGE_MAP()


// CRestApplicationGuiDlg message handlers

BOOL CRestApplicationGuiDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here
	m_maquette.reset(createMaquette(this));

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CRestApplicationGuiDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CRestApplicationGuiDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CRestApplicationGuiDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

// Implementation of IMaquetteCallback
BOOL CRestApplicationGuiDlg::postMessage(WPARAM wParam, LPARAM lParam)
{
	return PostMessage(WM_USER_EVENT, wParam, lParam);
}

void CRestApplicationGuiDlg::onConnAck(bool accepted)
{
	LOG4CPLUS_INFO(logger, __FUNCTIONW__ U(" ") << (accepted ? U("Accepted") : U("Rejected")));
};

void CRestApplicationGuiDlg::onConnectionClosed()
{
	LOG4CPLUS_INFO(logger, __FUNCTIONW__);
};

void CRestApplicationGuiDlg::onSubAck(bool accepted)
{
	LOG4CPLUS_INFO(logger, __FUNCTIONW__ U(" ") << (accepted ? U("Accepted") : U("Rejected")));
};

void CRestApplicationGuiDlg::onPublished(LPCTSTR topic, const data_t& payload)
{
	LOG4CPLUS_INFO(logger, __FUNCTIONW__ U(" Topic=") << topic << U(",") << payload.size() << U(" byte"));
};

afx_msg LRESULT CRestApplicationGuiDlg::OnUserEvent(WPARAM wParam, LPARAM lParam)
{
	return m_maquette->onUserEvent(wParam, lParam);
}

void CRestApplicationGuiDlg::OnClickedButtonConnect()
{
	UpdateData();
	m_maquette->connect(m_ServerUrl);
}


void CRestApplicationGuiDlg::OnClickedButtonDisconnect()
{
	m_maquette->disconnect();
}


void CRestApplicationGuiDlg::OnClickedButtonSubscibe()
{
	UpdateData();
	m_maquette->subscribe(m_Topic);
}


void CRestApplicationGuiDlg::OnClickedButtonUnsubscibe()
{
	// TODO: Add your control notification handler code here
}


void CRestApplicationGuiDlg::OnClickedButtonPublish()
{
	UpdateData();
	ATL::CT2A message(m_Payload);
	data_t payload;
	payload.assign((LPCSTR)message, &((LPCSTR)message)[m_Payload.GetLength()]);
	m_maquette->publish(m_Topic, payload);
}

void CRestApplicationGuiDlg::setConnectStatus()
{
	LPCTSTR status = U("Unknown");
	switch(m_ConnectStatus) {
	case ConnectStatusIdle:
		status = U("Idle"); break;
	case ConnectStatusClosed:
		status = U("Closed"); break;
	case ConnectStatusConnecting:
		status = U("Connecting"); break;
	case ConnectStatusConnected:
		status = U("Connected"); break;
	case ConnectStatusClosing:
		status = U("Closing"); break;
	}
	m_ConnectStatusText = status;
	UpdateData(FALSE);
}

void CRestApplicationGuiDlg::log(LPCTSTR format, ...)
{
	va_list args;
	va_start(args, format);
	CString text;
	text.FormatMessageV(format, &args);
	LOG4CPLUS_INFO(logger, (LPCTSTR)text);
	//text += U("\r\n");
	//m_LogText.Append(text);
	//PostMessage(WM_USER_SET_TEXT, IDC_EDIT_LOG, (LPARAM)&m_LogText);
}


//afx_msg LRESULT CRestApplicationGuiDlg::OnUserLog(WPARAM wParam, LPARAM lParam)
//{
//	return 0;
//}


afx_msg LRESULT CRestApplicationGuiDlg::OnUserSetText(WPARAM wParam, LPARAM lParam)
{
	CWnd* wnd = GetDlgItem(wParam);
	CString* text = (CString*)lParam;
	wnd->SetWindowText(*text);

	setConnectStatus();

	return 0;
}

bool CRestApplicationGuiDlg::canClose()
{
	return true;
	//if(m_mqttState == CMqttState::Initial) {
	//	return true;
	//} else {
	//	AfxMessageBox(U("Connection is not closed."), MB_OK + MB_ICONEXCLAMATION);
	//	return false;
	//}
};

void CRestApplicationGuiDlg::OnClose()
{
	if(canClose()) {
		CDialogEx::OnClose();
	}
}


void CRestApplicationGuiDlg::OnOK()
{
	if(canClose()) {
		CDialogEx::OnOK();
	}
}


void CRestApplicationGuiDlg::OnCancel()
{
	if(canClose()) {
		CDialogEx::OnCancel();
	}
}
