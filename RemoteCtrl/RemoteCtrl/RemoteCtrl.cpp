// RemoteCtrl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include "framework.h"
#include "RemoteCtrl.h"
#include"ServerSocket.h"
#include<direct.h>
#include<list>
#include<atlimage.h>
#include"LockInfoDialog.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 唯一的应用程序对象

CWinApp theApp;

using namespace std;
void Dump(BYTE*pData,size_t nsize) {
    std::string strOut;
    for (size_t i = 0; i < nsize; i++) {
        char buf[8] = "";
        if (i > 0 && (i % 16 == 0)) strOut += "\n";
        snprintf(buf, sizeof(buf), "%02X", pData[i] & 0xFF);
        strOut += buf;
    }
    strOut += "\n";
    OutputDebugStringA(strOut.c_str());
}
int MakeDriverInfo() {//1==>A 2==>B 3==>C 1 2 是软盘，，其中一直可以到26的编号就是Z盘
    std::string result;
    for (int i = 1; i <= 26; i++) {
        if (_chdrive(i) == 0) {//改变当前的驱动,返回0，就是切换成功
            if (result.size() > 0) result += ',';
            result += 'A' + i - 1;
        }  
    }
    CPacket pack(1, (BYTE*)result.c_str(), result.size());//打包用的
    Dump((BYTE*)pack.Data(), pack.Size());
    //CServerSocket::getInstance()->Send(pack);
    return 0;
}
#include<io.h>
typedef struct file_info{
    file_info() {
        IsInvalid = FALSE;
        IsDirectory = -1;
        HasNext = TRUE;
        memset(szFileName, 0, sizeof(szFileName));
    }
    BOOL IsInvalid;//是否无效
    BOOL IsDirectory;//是否为目录，否就是0，是就是1
    BOOL HasNext;//是否还有后续，0没有 1有
    char szFileName[256];//文件名
}FILEINFO,*PFILEINFO;
int MakeDirectoryInfo() {
    std::string strPath;
    //std::list<FILEINFO> lstFileInfos;
    if (CServerSocket::getInstance()->GetFilePath(strPath) == false) {
        OutputDebugString(_T("当前的命令，不是获取文件列表，命令解析错误!!!"));
        return -1;
    }
    if (_chdir(strPath.c_str()) != 0) {
        FILEINFO finfo;
        finfo.IsInvalid = TRUE;
        finfo.IsDirectory = TRUE;
        finfo.HasNext = FALSE;
        memcpy(finfo.szFileName, strPath.c_str(), strPath.size());
        CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
        if (CServerSocket::getInstance()->Send(pack) == FALSE) OutputDebugString(_T("发送失败，客户端连接失败或者发送数据失败!!"));
        //lstFileInfos.push_back(finfo);
        OutputDebugString(_T("没有权限访问目录！"));
        return -2;
    }
    _finddata_t fdata;
    int hfind = 0;
    if ((hfind =(int) _findfirst("*", &fdata)) == -1) {
        OutputDebugString(_T("没有找到任何文件!!"));
        return -3;
    }
    do {
        FILEINFO finfo;
        finfo.IsDirectory = (fdata.attrib & _A_SUBDIR) != 0;
        memcpy(finfo.szFileName, fdata.name, strlen(fdata.name));
        CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
        if(CServerSocket::getInstance()->Send(pack)==FALSE) OutputDebugString(_T("发送失败，客户端连接失败或者发送数据失败!!"));
        //lstFileInfos.push_back(finfo);
    } while (!_findnext(hfind, &fdata));
    FILEINFO finfo;
    finfo.HasNext = FALSE;
    CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
    if (CServerSocket::getInstance()->Send(pack) == FALSE) OutputDebugString(_T("发送失败，客户端连接失败或者发送数据失败!!"));
    return 0;
}
int RunFile() {
    std::string strPath;
    CServerSocket::getInstance()->GetFilePath(strPath);
    ShellExecuteA(NULL, NULL, strPath.c_str(), NULL, NULL, SW_SHOWNORMAL);//执行文件
    CPacket pack(3, NULL, 0);
    if (CServerSocket::getInstance()->Send(pack) == FALSE) OutputDebugString(_T("发送失败，客户端连接失败或者发送数据失败!!"));
    return 0;
}
int DownloadFile() {
    std::string strPath;
    CServerSocket::getInstance()->GetFilePath(strPath);
    long long data = 0;
    FILE* pFile = NULL;
    errno_t err=fopen_s(&pFile,strPath.c_str(), "rb");
    if ( (err!=0) || (pFile==NULL) ) {
        CPacket pack(4, (BYTE*)&data, 8);
        if (CServerSocket::getInstance()->Send(pack) == FALSE) OutputDebugString(_T("发送失败，客户端连接失败或者发送数据失败!!"));
        return -1;
    }
    if (pFile != NULL) {
        fseek(pFile, 0, SEEK_END);//将文件指针指向末尾，
        data = _ftelli64(pFile);//读取文件指针的位置，即因为文件指针在末尾，获取文件内容的长度
        CPacket head(4, (BYTE*)&data, 8);
        fseek(pFile, 0, SEEK_SET);
        char buffer[1024] = "";
        size_t rlen = 0;
        do {
            rlen = fread(buffer, 1, 1024, pFile);
            CPacket pack(4, (BYTE*)buffer, rlen);
            if (CServerSocket::getInstance()->Send(pack) == FALSE) OutputDebugString(_T("发送失败，客户端连接失败或者发送数据失败!!"));
        } while (rlen >= 1024);
        fclose(pFile);
    }
    CPacket pack(4, NULL, 0);
    if (CServerSocket::getInstance()->Send(pack) == FALSE) OutputDebugString(_T("发送失败，客户端连接失败或者发送数据失败!!"));
    return 0;
}
int MouseEvent() {
    MOUSEEV mouse;
    if (CServerSocket::getInstance()->GetMouseEvent(mouse)) {
        DWORD nFlags = 0;
        switch (mouse.nButton) {
        case 0://左键
            nFlags = 1;
            break;
        case 1://右键
            nFlags = 2;
            break;
        case 2://中键
            nFlags = 4;
            break;
        case 4://没有按键，纯移动
            nFlags = 8;
            break;
        }
        if(nFlags!=8)SetCursorPos(mouse.ptXY.x, mouse.ptXY.y);//设置鼠标的位置
        switch (mouse.nAction) {
        case 0://单击
            nFlags |= 0x10;
            break;
        case 1://双击
            nFlags |= 0x20;
            break;
        case 2://按下，拖动东西
            nFlags |= 0x40;
            break;
        case 3://放开
            nFlags |= 0x80;
            break;
        default:
            break;
        
        }

        switch (nFlags) {
        case 0x21://左键双击
            mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
        case 0x11://左键单击
            mouse_event(MOUSEEVENTF_LEFTDOWN,0,0,0,GetMessageExtraInfo()); 
            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x41://左键按下
            mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x81://左键放开
            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x22://右键双击
            mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
        case 0x12://右键单击
            mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x42://右键按下
            mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x82://右键放开
            mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x24://中键双击
            mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
        case 0x14://中键单击
            mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x44://中键按下
            mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x84://中键放开
            mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x08://单纯的鼠标移动
            mouse_event(MOUSEEVENTF_MOVE, mouse.ptXY.x, mouse.ptXY.y, 0, GetMessageExtraInfo());
            break;
        }
        CPacket pack(5, NULL, 0);
        CServerSocket::getInstance()->Send(pack);

    }
    else {
        OutputDebugString(_T("获取鼠标参数失败！！"));
        return -1;
    }
    return 0;
}
int SendScreen() {
    CImage screen;//图形图像 GDI
    HDC hScreen=::GetDC(NULL);//获取设备上下文(屏幕句柄)
    int nBitPerPixel = GetDeviceCaps(hScreen, BITSPIXEL);//获取设备的多个属性,返回的是用多少个bit来表示颜色(位宽)
    int nWidth = GetDeviceCaps(hScreen, HORZRES);//获取设备的宽，水平宽度
    int nHeight = GetDeviceCaps(hScreen, VERTRES);//获取设备的高，垂直高度
    screen.Create(nWidth, nHeight, nBitPerPixel);//创建图像

    //BitBlt函数将hScreen的位图数据按照指定的方式传输到screen上。
    BitBlt(screen.GetDC(), 0, 0, 1920, 1020, hScreen, 0, 0, SRCCOPY);//复制图像
    ReleaseDC(NULL, hScreen);//释放

    //GlobalAlloc 是一个 Windows API 函数，用于在全局堆（Global Heap）中分配指定大小的内存块。它返回一个全局内存块的句柄。
    HGLOBAL hMem= GlobalAlloc(GMEM_MOVEABLE, 0);
    if (hMem == NULL) return -1;
    IStream* pStream = NULL;//流
    HRESULT ret= CreateStreamOnHGlobal(hMem, TRUE, &pStream);
    if (ret == S_OK) {
        screen.Save(pStream, Gdiplus::ImageFormatPNG);//以PNG格式保存到内存流里
        LARGE_INTEGER bg = { 0 };
        pStream->Seek(bg, STREAM_SEEK_SET, NULL);
        PBYTE pData=(PBYTE)GlobalLock(hMem);//锁定内存
        size_t nSize = GlobalSize(hMem);
        CPacket pack(6, pData, nSize);
        CServerSocket::getInstance()->Send(pack);
        GlobalUnlock(hMem);
    }
    pStream->Release();
    GlobalFree(hMem);
    //screen.Save(_T("test2023.png"), Gdiplus::ImageFormatPNG);//以PNG格式保存

   //DWORD tick = GetTickCount64();//获取时间
    //screen.Save(_T("test2023.jpg"), Gdiplus::ImageFormatJPEG);//以JPEG格式保存
    //TRACE("jpg %d\r\n", GetTickCount64() - tick);//获取jpg制作时间

    screen.ReleaseDC();
    return 0;
}
CLockInfoDialog dlg;
unsigned  threadid = 0;
unsigned _stdcall threadLockDlg(void *args) {
    TRACE("%s(%d):%d\r\n", __FUNCTION__,__LINE__,GetCurrentThreadId());//GetCurrentThreadId()获取线程id
    dlg.Create(IDD_DIALOG_INFO, NULL);
    dlg.ShowWindow(SW_SHOW);
    CRect rect;
    rect.left = 0;
    rect.top = 0;
    rect.right = GetSystemMetrics(SM_CXFULLSCREEN);//获取屏幕分辨率xy
    rect.bottom = GetSystemMetrics(SM_CYFULLSCREEN) + 30;//设置窗口大小的参数
    dlg.MoveWindow(rect);
    //窗口置顶
    dlg.SetWindowPos(&dlg.wndTopMost, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);//设置窗口为最前面的，不能修改大小，不能移动
    //限制鼠标显示功能
    ShowCursor(false);
    ::ShowWindow(::FindWindow(_T("Shell_TrayWnd"), NULL), SW_HIDE);//把下面的任务栏隐藏起来
    //限制鼠标的活动范围
    dlg.GetWindowRect(rect);//获取窗口的范围
    rect.left = 0;
    rect.top = 0;
    rect.right = 1;
    rect.bottom = 1;//将鼠标限制在一个点上
    ClipCursor(rect);//把鼠标限制在窗口的位置
    MSG msg;
    //消息循环
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);//传递消息
        DispatchMessage(&msg);//分配消息
        if (msg.wParam == 0x1B || msg.wParam == 0x41) {//按下a或者esc退出
            TRACE("msg:%08X wparam:%08X lparam:%08X\r\n", msg.message, msg.wParam, msg.lParam);
            break;
        }
    }
    
    ShowCursor(true);
    ::ShowWindow(::FindWindow(_T("Shell_TrayWnd"), NULL), SW_SHOW);//把任务栏显示起来
    dlg.DestroyWindow();
    _endthreadex(0);
    return 0;
}
int LockMachine() {
    if ((dlg.m_hWnd == NULL) || (dlg.m_hWnd == INVALID_HANDLE_VALUE)) {
        _beginthreadex(NULL, 0, threadLockDlg, NULL, 0, &threadid);
        TRACE("threadid=%d\r\n", threadid);
    }
    CPacket pack(7, NULL, 0);
    CServerSocket::getInstance()->Send(pack);
    return 0;
}
int UnlockMachine() {
    //dlg.SendMessage(WM_KEYDOWN, 0x41,0x01E0001);
    PostThreadMessage(threadid, WM_KEYDOWN, 0x41, 0);
    CPacket pack(7, NULL, 0);
    CServerSocket::getInstance()->Send(pack);
    return 0;
}
int main()
{
    int nRetCode = 0;

    HMODULE hModule = ::GetModuleHandle(nullptr);

    if (hModule != nullptr)
    {
        // 初始化 MFC 并在失败时显示错误
        if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0))
        {
            // TODO: 在此处为应用程序的行为编写代码。
            wprintf(L"错误: MFC 初始化失败\n");
            nRetCode = 1;
        }
        else
        {
            //// TODO: 在此处为应用程序的行为编写代码。
            ////套接字初始化
            //CServerSocket* pserver = CServerSocket::getInstance();
            //int count = 0;
            //if (pserver->InitSocket() == false) {
            //    MessageBox(NULL, _T("网络初始化异常，未能成功初始化，请检查网络状态！"), _T("网络初始化失败！"), MB_OK | MB_ICONERROR);
            //    exit(0);
            //}
            //while (CServerSocket::getInstance() !=NULL) { 
            //    if (pserver->AcceptClient() == false) {
            //        if (count >= 3) {
            //            MessageBox(NULL, _T("多次无法正常接入用户，结束程序！"), _T("接入用户失败"), MB_OK | MB_ICONERROR);
            //            exit(0);
            //        }
            //        MessageBox(NULL, _T("无法正常接入用户，自动重试"), _T("接入用户失败"), MB_OK | MB_ICONERROR);
            //        count++;
            //    }
            //    int ret = pserver->DealCommand();
            //    //TODO:
            //}
           
            int nCmd = 7;
            switch (nCmd)
            {
            case 1://查看磁盘分区
                MakeDriverInfo();
                break;
            case 2://查看指定目录下的文件
                MakeDirectoryInfo();
                break;
            case 3://打开文件
                RunFile();
                break;
            case 4://下载文件
                DownloadFile();
                break;
            case 5://鼠标操作
                MouseEvent();
                break;
            case 6://发送屏幕内容==>发送屏幕的截图
                SendScreen();
                break;
            case 7://锁机
                LockMachine();
                //Sleep(50);
                //LockMachine();
                break;
            case 8://解锁
                UnlockMachine();
                break;
            }
            Sleep(5000);
            UnlockMachine();
            //while ((dlg.m_hWnd != NULL) && (dlg.m_hWnd != INVALID_HANDLE_VALUE)) Sleep(100);
            while (dlg.m_hWnd != NULL) Sleep(10);

        }
    }
    else
    {
        // TODO: 更改错误代码以符合需要
        wprintf(L"错误: GetModuleHandle 失败\n");
        nRetCode = 1;
    }

    return nRetCode;
}
