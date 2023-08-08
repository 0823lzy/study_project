// RemoteCtrl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include "framework.h"
#include "RemoteCtrl.h"
#include"ServerSocket.h"
#include<direct.h>
#include<list>
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
            int nCmd = 1;
            switch (nCmd)
            {
            case 1://查看磁盘分区
                MakeDriverInfo();
                break;
            case 2://查看指定目录下的文件
                MakeDirectoryInfo();
                break;
            default:
                break;
            }
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
