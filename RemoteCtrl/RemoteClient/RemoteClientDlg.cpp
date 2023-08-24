
// RemoteClientDlg.cpp: 实现文件
//

#include "pch.h"
#include "framework.h"
#include "RemoteClient.h"
#include "RemoteClientDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif
#include "CWatchDialog.h"


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CRemoteClientDlg 对话框



CRemoteClientDlg::CRemoteClientDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_REMOTECLIENT_DIALOG, pParent)
	, m_server_address(0)
	, m_nPort(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CRemoteClientDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_IPAddress(pDX, IDC_IPADDRESS_SERV, m_server_address);
	DDX_Text(pDX, IDC_EDIT_PORT, m_nPort);
	DDX_Control(pDX, IDC_TREE_DIR, m_Tree);
	DDX_Control(pDX, IDC_LIST_FILE, m_list);
}

int CRemoteClientDlg::SendCommandPacket(int nCmd, bool bAutoClose,BYTE* pData, size_t nLenght)
{
	UpdateData();
	CClientSocket* pClient = CClientSocket::getInstance();
	bool ret = pClient->InitSocket(m_server_address, atoi((LPCTSTR)m_nPort));
	if (ret == false) {
		AfxMessageBox("网络初始化失败！");
		return -1;
	}
	CPacket pack(nCmd, pData, nLenght);
	bool flag = pClient->Send(pack);
	TRACE("flag:%d\r\n", flag);
	int cmd = pClient->DealCommand();
	TRACE("ack:%d\r\n", cmd);
	if (bAutoClose) {
		pClient->CloseSocket();
	}
	return cmd;
}

BEGIN_MESSAGE_MAP(CRemoteClientDlg, CDialogEx)//消息映射表
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BTN_TEST, &CRemoteClientDlg::OnBnClickedBtnTest)
	ON_BN_CLICKED(IDC_BUTTON_FILEINFO, &CRemoteClientDlg::OnBnClickedButtonFileinfo)
	ON_NOTIFY(NM_DBLCLK, IDC_TREE_DIR, &CRemoteClientDlg::OnNMDblclkTreeDir)
	ON_NOTIFY(NM_CLICK, IDC_TREE_DIR, &CRemoteClientDlg::OnNMClickTreeDir)
	ON_NOTIFY(NM_RCLICK, IDC_LIST_FILE, &CRemoteClientDlg::OnNMRClickListFile)
	ON_COMMAND(ID_DOWNLOAD_FILE, &CRemoteClientDlg::OnDownloadFile)
	ON_COMMAND(ID_DELETE_FILE, &CRemoteClientDlg::OnDeleteFile)
	ON_COMMAND(ID_RUN_FILE, &CRemoteClientDlg::OnRunFile)
	ON_MESSAGE(WM_SEND_PACKET, &CRemoteClientDlg::OnSendPacket)//注册消息，告诉系统消息id对应的响应函数，第三步
	ON_BN_CLICKED(IDC_BTN_START_WATCH, &CRemoteClientDlg::OnBnClickedBtnStartWatch)
	ON_WM_TIMER()
END_MESSAGE_MAP()


// CRemoteClientDlg 消息处理程序

BOOL CRemoteClientDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
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

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码
	UpdateData();
	m_server_address = 0x7F000001;
	m_nPort = _T("9527");
	UpdateData(false);
	m_dlgStatus.Create(IDD_DIALOG_STATUS,this);
	m_dlgStatus.ShowWindow(SW_HIDE);
	m_isFull = false;//将缓存设置为false
	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CRemoteClientDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CRemoteClientDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CRemoteClientDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CRemoteClientDlg::OnBnClickedBtnTest()
{
	//UpdateData();//默认为true，将控件的值更新到代码，如果为false，则相反。
	////m_server_address;
	////atoi((LPCTSTR)m_nPort);//将port转为整数，（端口)
	//CClientSocket* pClient = CClientSocket::getInstance();
	//bool ret=pClient->InitSocket(m_server_address, atoi((LPCTSTR)m_nPort));
	//if (ret == false) {
	//	AfxMessageBox("网络初始化失败！");
	//	return;
	//}
	//CPacket pack(1981,NULL,0);
	//bool flag=pClient->Send(pack);
	//TRACE("flag:%d\r\n", flag);
	//int cmd=pClient->DealCommand();
	//TRACE("ack:%d\r\n",cmd);
	//pClient->CloseSocket();
	SendCommandPacket(1998);
}


void CRemoteClientDlg::OnBnClickedButtonFileinfo()
{
	// TODO: 在此添加控件通知处理程序代码
	int ret=SendCommandPacket(1);
	if (ret == -1) {
		AfxMessageBox(_T("命令处理失败！！！"));
		return;
	}
	CClientSocket* pClient = CClientSocket::getInstance();
	std::string drivers=pClient->GetPacket().strData;
	std::string dr;
	m_Tree.DeleteAllItems();
	for (size_t i = 0; i < drivers.size(); i++) {
		if (drivers[i] == ',') {
			dr += ":";
			HTREEITEM hTemp= m_Tree.InsertItem(dr.c_str(),TVI_ROOT,TVI_LAST);//把dr追加到根目录下面
			m_Tree.InsertItem(NULL, hTemp, TVI_LAST);
			dr.clear();
			continue;
		}
		dr += drivers[i];
		if (drivers[i] == 'F') {
			dr += ":";
			HTREEITEM hTemp = m_Tree.InsertItem(dr.c_str(), TVI_ROOT, TVI_LAST);//把dr追加到根目录下面
			m_Tree.InsertItem(0, hTemp, TVI_LAST);
		}
	}
}



CString CRemoteClientDlg::GetPath(HTREEITEM hTree) {
	CString strRet, strTmp;
	do {
		strTmp = m_Tree.GetItemText(hTree);
		strRet = strTmp + '\\' + strRet;
		hTree = m_Tree.GetParentItem(hTree);//拿到父节点
	} while (hTree != NULL);
	return strRet;
}
void CRemoteClientDlg::LoadFileCurrent()
{
	HTREEITEM hTree = m_Tree.GetSelectedItem();
	CString strPath = GetPath(hTree);
	m_list.DeleteAllItems();
	int nCmd = SendCommandPacket(2, false, (BYTE*)(LPCTSTR)strPath, strPath.GetLength());
	PFILEINFO pInfo = (PFILEINFO)CClientSocket::getInstance()->GetPacket().strData.c_str();
	CClientSocket* pClient = CClientSocket::getInstance();
	while (pInfo->HasNext) {
		if (!pInfo->IsDirectory) {
			m_list.InsertItem(0, pInfo->szFileName);
		}
		int cmd = pClient->DealCommand();
		TRACE("ack:%d\r\n", cmd);
		if (cmd < 0) break;
		PFILEINFO pInfo = (PFILEINFO)CClientSocket::getInstance()->GetPacket().strData.c_str();
	}

	pClient->CloseSocket();
}
void CRemoteClientDlg::LoadFileInfo()
{
	CPoint ptMouse;//拿到鼠标指针,全局坐标
	GetCursorPos(&ptMouse);//拿到指针位置，全局坐标
	m_Tree.ScreenToClient(&ptMouse);//转成客户端坐标
	HTREEITEM hTreeSelected = m_Tree.HitTest(ptMouse, 0);//拿到双击点击的地方
	if (hTreeSelected == NULL) {//点不中节点位置，就退出
		return;
	}
	if (m_Tree.GetChildItem(hTreeSelected) == NULL) return;
	DeleteTreeChildrenItem(hTreeSelected);
	m_list.DeleteAllItems();
	CString strPath = GetPath(hTreeSelected);
	int nCmd = SendCommandPacket(2, false, (BYTE*)(LPCTSTR)strPath, strPath.GetLength());
	PFILEINFO pInfo = (PFILEINFO)CClientSocket::getInstance()->GetPacket().strData.c_str();
	CClientSocket* pClient = CClientSocket::getInstance();
	while (pInfo->HasNext) {
		if (pInfo->IsDirectory) {
			if (CString(pInfo->szFileName) == "." || CString(pInfo->szFileName) == "..") {
				int cmd = pClient->DealCommand();
				TRACE("ack:%d\r\n", cmd);
				if (cmd < 0) break;
				PFILEINFO pInfo = (PFILEINFO)CClientSocket::getInstance()->GetPacket().strData.c_str();
				continue;
			}
			HTREEITEM hTemp = m_Tree.InsertItem(pInfo->szFileName, hTreeSelected, TVI_LAST);
			m_Tree.InsertItem(NULL, hTemp, TVI_LAST);
		}
		else {
			m_list.InsertItem(0, pInfo->szFileName);
		}
		
		int cmd = pClient->DealCommand();
		TRACE("ack:%d\r\n", cmd);
		if (cmd < 0) break;
		PFILEINFO pInfo = (PFILEINFO)CClientSocket::getInstance()->GetPacket().strData.c_str();
	}

	pClient->CloseSocket();
}
void CRemoteClientDlg::threadEntryForWatchData(void* arg)
{
	CRemoteClientDlg* thiz = (CRemoteClientDlg*)arg;
	thiz->threadWatchData();
	_endthread();
}
void CRemoteClientDlg::threadWatchData()
{
	Sleep(50);
	CClientSocket* pClient = NULL;
	do {
		pClient = CClientSocket::getInstance();
	} while (pClient == NULL);
	//用于获取自系统启动以来经过的毫秒数。它返回一个无符号32位整数，
	//表示从系统启动到当前时刻所经过的毫秒数。
	//ULONGLONG tick = GetTickCount64();
	for (;;) {
		if (m_isFull == false) {//更新数据到缓存
			LRESULT ret = SendMessage(WM_SEND_PACKET, 6 << 1 | 1);
			if (ret == 6) {
				BYTE* pData = (BYTE*)pClient->GetPacket().strData.c_str();
				//GlobalAlloc函数用于从全局堆中分配一块内存
				HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, 0);//分配一个全局的内存
				if (hMem == NULL) {
					TRACE("内存不足了！");
					Sleep(1);
					continue;
				}
				IStream* pStream = NULL;//创建一个流
				//CreateStreamOnHGlobal 是一个函数，它可以将全局内存块转换为可用于操作数据的流接口。
				HRESULT hRet = CreateStreamOnHGlobal(hMem, true, &pStream);//将全局内存转换为流
				if (hRet == S_OK) {
					ULONG length = 0;
					//Write写入流中，pdata指向要写入流中的数据缓冲区的指针，size是写入字节数，length实际写入字节数
					pStream->Write(pData, pClient->GetPacket().strData.size(), &length);
					LARGE_INTEGER bg = { 0 };
					pStream->Seek(bg, STREAM_SEEK_SET, NULL);//seek到流的开头
					m_image.Load(pStream);//把流load到缓存中
					m_isFull = true;
				}
			}
			else Sleep(1);
		}
		else Sleep(1);
			
	}
}
void CRemoteClientDlg::threadEntryDownFile(void* arg)
{
	CRemoteClientDlg* thiz = (CRemoteClientDlg*)arg;
	thiz->threadDownFile();
	_endthread();
}
void CRemoteClientDlg::threadDownFile()
{
	int nListSelected = m_list.GetSelectionMark();//获取选择的标记
	CString strFile = m_list.GetItemText(nListSelected, 0);//拿到文件名
	CFileDialog dlg(FALSE, NULL, strFile, OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY, NULL, this);
	if (dlg.DoModal() == IDOK) {
		FILE* pFile = fopen(dlg.GetPathName(), "wb+");
		if (pFile == NULL) {
			AfxMessageBox(_T("没有权限保存该文件或者文件无法创建！！！"));
			m_dlgStatus.ShowWindow(SW_HIDE);
			EndWaitCursor();
			return;
		}
		HTREEITEM hSelected = m_Tree.GetSelectedItem();//获取文件夹的名字
		strFile = GetPath(hSelected) + strFile;
		TRACE("%s\r\n", LPCSTR(strFile));
		CClientSocket* pClient = CClientSocket::getInstance();
		do {
			//int ret = SendCommandPacket(4, false, (BYTE*)LPCSTR(strFile), strFile.GetLength());
			LRESULT ret = SendMessage(WM_SEND_PACKET, 4 << 1 | 0, (LPARAM)(LPCSTR)strFile);//子线程发信息给主线程
			if (ret < 0) {
				AfxMessageBox("执行下载命令失败！！");
				TRACE("执行下载失败，ret=%d\r\n", ret);
				break;
			}
			long long nLength = *(long long*)pClient->GetPacket().strData.c_str();
			if (nLength == 0) {
				AfxMessageBox("文件长度为0或者无法读取文件！");
				break;
			}
			long long nCount = 0;

			while (nCount < nLength) {
				ret = pClient->DealCommand();
				if (ret < 0) {
					AfxMessageBox("传输失败");
					TRACE("传输失败,ret=%d\r\n", ret);
					break;
				}
				fwrite(pClient->GetPacket().strData.c_str(), 1, pClient->GetPacket().strData.size(), pFile);
				nCount += pClient->GetPacket().strData.size();
			}
		} while (false);
		fclose(pFile);
		pClient->CloseSocket();
	}
	m_dlgStatus.ShowWindow(SW_HIDE);
	EndWaitCursor();
	MessageBox(_T("下载完成!!"),_T("完成"));
}
void CRemoteClientDlg::DeleteTreeChildrenItem(HTREEITEM hTree)
{
	HTREEITEM hSub = NULL;
	do {
		hSub = m_Tree.GetChildItem(hTree);
		if (hSub != NULL)m_Tree.DeleteItem(hSub);
	} while (hSub != NULL);
}
void CRemoteClientDlg::OnNMDblclkTreeDir(NMHDR* pNMHDR, LRESULT* pResult)//双击节点事件
{
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;
	LoadFileInfo();
}


void CRemoteClientDlg::OnNMClickTreeDir(NMHDR* pNMHDR, LRESULT* pResult)
{
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;
	LoadFileInfo();
}


void CRemoteClientDlg::OnNMRClickListFile(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;
	CPoint ptMouse,ptList;
	GetCursorPos(&ptMouse);
	ptList = ptMouse;
	m_list.ScreenToClient(&ptList);
	int listSelected = m_list.HitTest(ptList);
	if (listSelected < 0)return;
	CMenu menu;
	menu.LoadMenu(IDR_MENU_RCLICK);//把资源菜单加载上来
	CMenu*pPupup=menu.GetSubMenu(0);//取资源菜单的第一个子菜单
	if (pPupup != NULL) {
		pPupup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, ptMouse.x, ptMouse.y, this);//弹出菜单
	}
}


void CRemoteClientDlg::OnDownloadFile()
{
	_beginthread(CRemoteClientDlg::threadEntryDownFile, 0, this);//开始线程
	BeginWaitCursor();//将鼠标光标设置为等待状态
	m_dlgStatus.m_info.SetWindowText(_T("命令正在执行中！"));
	m_dlgStatus.ShowWindow(SW_SHOW);
	m_dlgStatus.CenterWindow(this);
	m_dlgStatus.SetActiveWindow();

}


void CRemoteClientDlg::OnDeleteFile()
{
	HTREEITEM hSelected = m_Tree.GetSelectedItem();//获取当前选中的文件夹
	CString strPath = GetPath(hSelected);//获取当前文件夹路径信息
	int nSelected = m_list.GetSelectionMark();//获取当前选中的文件
	CString strFile = m_list.GetItemText(nSelected, 0);//获取文件名
	strFile = strPath + strFile;
	int ret = SendCommandPacket(9, true, (BYTE*)(LPCSTR)strFile, strFile.GetLength());
	if (ret < 0) {
		AfxMessageBox("删除文件命令执行失败！！!");
	}
	LoadFileCurrent();
}


void CRemoteClientDlg::OnRunFile()
{
	HTREEITEM hSelected = m_Tree.GetSelectedItem();//获取当前选中的文件夹
	CString strPath = GetPath(hSelected);//获取当前文件夹路径信息
	int nSelected = m_list.GetSelectionMark();//获取当前选中的文件
	CString strFile = m_list.GetItemText(nSelected, 0);//获取文件名
	strFile = strPath + strFile;
	int ret=SendCommandPacket(3, true, (BYTE*)(LPCSTR)strFile, strFile.GetLength());
	if (ret < 0) {
		AfxMessageBox("打开文件命令执行失败！！!");
	}
}

LRESULT CRemoteClientDlg::OnSendPacket(WPARAM wParam, LPARAM lParam)
{//实现消息响应函数 第四步
	int ret = 0;
	int cmd = wParam >> 1;
	switch(cmd) {
	case 4:
		{
			CString strFile = (LPCSTR)lParam;
			ret = SendCommandPacket(cmd, wParam & 1, (BYTE*)(LPCSTR)strFile, strFile.GetLength());
		}
		break;
	case 6:
		{
			ret = SendCommandPacket(cmd, wParam & 1);
		}
		break;
	default:
		ret = -1;
	}

	
	return ret;
}


void CRemoteClientDlg::OnBnClickedBtnStartWatch()
{
	CWatchDialog dlg(this);//传递this
	_beginthread(CRemoteClientDlg::threadEntryForWatchData, 0, this);
	dlg.DoModal();//模态对话框
}


void CRemoteClientDlg::OnTimer(UINT_PTR nIDEvent)//定时器
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值

	CDialogEx::OnTimer(nIDEvent);
}
