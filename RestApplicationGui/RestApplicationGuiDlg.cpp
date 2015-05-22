
// RestApplicationGuiDlg.cpp : implementation file
//

#include "stdafx.h"
#include "RestApplicationGui.h"
#include "RestApplicationGuiDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using namespace web::websockets::client;
using namespace concurrency::streams;
using namespace utility;
using namespace utility::conversions;
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



void CRestApplicationGuiDlg::OnClickedButtonConnect()
{
	postEvent(CMqttEvent::Connect);
}


void CRestApplicationGuiDlg::OnClickedButtonDisconnect()
{
	postEvent(CMqttEvent::Disconnect);
}


void CRestApplicationGuiDlg::OnClickedButtonSubscibe()
{
	// TODO: Add your control notification handler code here
	CConnectPacket conn;
	const byte* pConn = conn.data().data();
	LOG4CPLUS_DEBUG(logger, "CConnectPacket: " << conn.type());

	CDisconnectPacket disc;
	const byte* pDisc = disc.data().data();
	LOG4CPLUS_DEBUG(logger, "CDisconnectPacket: " << disc.type());

	data_t test(4);
	test[0] = 0x20;
	test[1] = 0x02;
	test[2] = 0x01;
	test[3] = 0x05;
	CReceivedPacket* rec = CReceivedPacket::create(test);
	_ASSERTE(rec);
	CConnAckPacket* pConnAck = dynamic_cast<CConnAckPacket*>(rec);
	LOG4CPLUS_DEBUG(logger, "CConnAckPacket: " << typeid(*rec).name() << "," << rec->type() << "," << pConnAck->returnCode);
	delete rec;
}


void CRestApplicationGuiDlg::OnClickedButtonUnsubscibe()
{
	// TODO: Add your control notification handler code here
}


void CRestApplicationGuiDlg::OnClickedButtonPublish()
{
	// TODO: Add your control notification handler code here
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
	if(m_mqttState == CMqttState::Initial) {
		return true;
	} else {
		AfxMessageBox(U("Connection is not closed."), MB_OK + MB_ICONEXCLAMATION);
		return false;
	}
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

void CRestApplicationGuiDlg::postEvent(CMqttEvent::Value value)
{
	postEvent(new CMqttEvent(value));
}

void CRestApplicationGuiDlg::postEvent(CMqttEvent* pEvent)
{
	VERIFY(PostMessage(WM_USER_EVENT, 0, (LPARAM)pEvent));
}

afx_msg LRESULT CRestApplicationGuiDlg::OnUserEvent(WPARAM wParam, LPARAM lParam)
{
	CMqttEvent* pEvent = (CMqttEvent*)lParam;
	LOG4CPLUS_TRACE(logger, "OnUserEvent(): state=" << m_mqttState.toString() << ", event=" << pEvent->toString());

	if(!m_mqttState.isValid()) {
		LOG4CPLUS_FATAL(logger, "m_mqttState is out of range: " << (byte)m_mqttState);
		return 0;
	}
	if(!pEvent->isValid()) {
		LOG4CPLUS_FATAL(logger, "m_mqttState is out of range: " << (byte)m_mqttState);
		return 0;
	}

	event_handler_t handler = state_event_table[*pEvent][m_mqttState];
	m_mqttState = (this->*handler)(pEvent);
	delete pEvent;

	m_ConnectStatusText = m_mqttState;
	UpdateData(FALSE);
	return 0;
}

#define H(x) &CRestApplicationGuiDlg::handle##x
#define _IGNORE H(Ignore)
#define _FATAL H(Fatal)
#define _NOT_IMPL _FATAL

const CRestApplicationGuiDlg::event_handler_t CRestApplicationGuiDlg::state_event_table[CMqttEvent::Value::_Count][CMqttState::_Count] =
{
	//	Initial				ConnectingSocket	ConnectingBroker	Connected			Subscribing			Subscribed			Disconnecting
	{	H(Connect),			_IGNORE,			_IGNORE,			_IGNORE,			_IGNORE,			_IGNORE,			H(Connect)	},		// Connect
	{	_IGNORE,			H(Disconnect),		H(Disconnect),		H(Disconnect),		H(Disconnect),		H(Disconnect),		_IGNORE		},		// Disconnect
	{	_FATAL,				H(ConnectedSocket),	_FATAL,				_FATAL,				_FATAL,				_FATAL,				_FATAL		},		// ConnectedSocket
	{	_NOT_IMPL,			H(ClosedSocket),	H(ClosedSocket),	H(ClosedSocket),	H(ClosedSocket),	H(ClosedSocket),	H(ClosedSocket)	},	// ClosedSocket
	{	_NOT_IMPL,			H(ConnAck),			_NOT_IMPL,			_NOT_IMPL,			_NOT_IMPL,			_NOT_IMPL,			_NOT_IMPL	},		// ConnAck
	{	_NOT_IMPL,			_NOT_IMPL,			_NOT_IMPL,			_NOT_IMPL,			_NOT_IMPL,			_NOT_IMPL,			_NOT_IMPL	},		// SubAck
	{	_NOT_IMPL,			_NOT_IMPL,			_NOT_IMPL,			_NOT_IMPL,			_NOT_IMPL,			_NOT_IMPL,			_NOT_IMPL	},		// Publish
	{	_NOT_IMPL,			_NOT_IMPL,			_NOT_IMPL,			_NOT_IMPL,			_NOT_IMPL,			_NOT_IMPL,			_NOT_IMPL	},		// Published
	{	_NOT_IMPL,			_NOT_IMPL,			_NOT_IMPL,			_NOT_IMPL,			_NOT_IMPL,			_NOT_IMPL,			_NOT_IMPL	},		// PingTimer
};

void CRestApplicationGuiDlg::send(const data_t& data)
{
	auto buf = std::make_shared<streambuf<byte>>();
	buf->putn(data.data(), data.size())
		.then([this, buf](pplx::task<size_t> task) {
			task.wait();
			websocket_outgoing_message msg;
			msg.set_binary_message(buf->create_istream());
			m_client->send(msg);
		});
}

CMqttState CRestApplicationGuiDlg::handleConnect(CMqttEvent* pEvent)
{
	UpdateData();
	log(U("Connecting: '%1!s!'"), m_ServerUrl);

	websocket_client_config config;
	config.add_subprotocol(U("mqtt"));
	m_client.reset(new websocket_callback_client(config));
	m_client->set_message_handler([this](const websocket_incoming_message& msg) {
		auto in = std::make_shared<istream>();
		msg.body().read_to_end(in->streambuf())
			.then([this, in](size_t size) {
				LOG4CPLUS_DEBUG(logger, "Received " << size << "bytes");
				data_t data(size);
				in->streambuf().scopy(data.data(), size);
				CReceivedPacket* packet = CReceivedPacket::create(data);
				if(packet) {
					LOG4CPLUS_DEBUG(logger, "Received: " << typeid(*packet).name());
					postEvent(new CReceivedPacketEvent(packet));
				}
			});
	});
	m_client->set_close_handler([this](websocket_close_status status, const string_t& reason, const std::error_code& error) {
		log(U("close_handler: status=%1!d!, reason='%2', error='%3'"),
			status, reason.c_str(), to_string_t(error.message().c_str()).c_str());
		postEvent(CMqttEvent::ClosedSocket);
	});

	m_client->connect((LPCTSTR)m_ServerUrl)
		.then([this]() {
			log(U("Connected: '%1'"), m_ServerUrl);
			postEvent(CMqttEvent::ConnectedSocket);
		});

	return CMqttState::ConnectingSocket;
}

CMqttState CRestApplicationGuiDlg::handleDisconnect(CMqttEvent* pEvent)
{
	m_client->close();
	return CMqttState::Disconnecting;
}

CMqttState CRestApplicationGuiDlg::handleConnectedSocket(CMqttEvent* pEvent)
{
	CConnectPacket packet;
	//send(packet.data());
	return CMqttState::ConnectingBroker;
}

CMqttState CRestApplicationGuiDlg::handleClosedSocket(CMqttEvent* pEvent)
{
	return CMqttState::Initial;
}

CMqttState CRestApplicationGuiDlg::handleConnAck(CMqttEvent* pEvent)
{
	CReceivedPacketEvent* p = dynamic_cast<CReceivedPacketEvent*>(pEvent);
	CConnAckPacket* packet = dynamic_cast<CConnAckPacket*>(p->m_packet);

	if(packet->returnCode == CConnAckPacket::ReturnCode::ConnectionAccepted) {
		LOG4CPLUS_INFO(logger, "MQTT CONNECT accepted.");
		return CMqttState::Connected;
	} else {
		LOG4CPLUS_ERROR(logger, "MQTT CONNECT rejected: Return code=" << packet->returnCode);
		m_client->close();
		return CMqttState::Initial;
	}
}

CMqttState CRestApplicationGuiDlg::handlePingTimer(CMqttEvent* pEvent)
{
	return m_mqttState;
}

CMqttState CRestApplicationGuiDlg::handleIgnore(CMqttEvent* pEvent)
{
	LOG4CPLUS_TRACE(logger, "handleIgnore(): event=" << typeid(*pEvent).name());
	return m_mqttState;
}

CMqttState CRestApplicationGuiDlg::handleFatal(CMqttEvent* pEvent)
{
	LOG4CPLUS_FATAL(logger, "handleFatal(): event=" << typeid(*pEvent).name());
	return m_mqttState;
}

#define _TO_STRING(x) #x

const LPCSTR CMqttState::m_valueNames[Value::_Count] = {
	_TO_STRING(Initial),
	_TO_STRING(ConnectingSocket),
	_TO_STRING(ConnectingBroker),
	_TO_STRING(Connected),
	_TO_STRING(Subscribing),
	_TO_STRING(Subscribed),
	_TO_STRING(Disconnecting)
};

const LPCSTR CMqttEvent::m_valueNames[Value::_Count] = {
	_TO_STRING(Connect),
	_TO_STRING(Disconnect),
	_TO_STRING(ConnectedSocket),
	_TO_STRING(ClosedSocket),
	_TO_STRING(ConnAck),
	_TO_STRING(SubAck),
	_TO_STRING(Publish),
	_TO_STRING(Published),
	_TO_STRING(PingTimer),
};
