#pragma once
#include "pch.h"
#include "framework.h"
class CServerSocket
{
public:
	static CServerSocket* getInstance() {
		if (m_instance == NULL) {
			m_instance = new CServerSocket();
		}
		return m_instance;
	}
	bool InitSocket() {
		
		if (m_sock == -1) return false;
		sockaddr_in servaddr;
		memset(&servaddr, 0, sizeof(servaddr));
		servaddr.sin_family = AF_INET;
		servaddr.sin_addr.s_addr = INADDR_ANY;
		servaddr.sin_port = htons(9527);
		//绑定 
		if (bind(m_sock, (sockaddr*)&servaddr, sizeof(servaddr)) == -1) return false;
		if (listen(m_sock, 1) == -1) return false;
		return true;
	} 
	bool AcceptClient() {
		sockaddr_in clntaddr;
		int clntlen = sizeof(clntaddr);
		m_client = accept(m_sock, (sockaddr*)&clntaddr, &clntlen);
		if (m_client == -1)return false;
		return true;
	}
	int  DealCommand() {
		if (m_client == -1)return false;
		char buffer[1024] = "";
		while (true) {
			int len= recv(m_client, buffer, sizeof(buffer), 0);
			if (len <= 0) {
				return -1;
			}
			//TODO:处理命令

		}
	}
	bool Send(const char* pData, int nsize) {
		if (m_client == -1)return false;
		return send(m_client, pData, nsize, 0)>=0;
	}
private:
	SOCKET m_sock;
	SOCKET m_client;
	CServerSocket& operator=(const CServerSocket&ss) {}
	CServerSocket(const CServerSocket&ss) {
		m_sock = ss.m_sock;
		m_client = ss.m_client;
	}
	CServerSocket(){
		if (InitSockEnv() == FALSE) {
			MessageBox(NULL, _T("无法初始化套接字环境,请检查网络设置！"), _T("初始化错误！"),MB_OK|MB_ICONERROR);
			exit(0);
		}
		m_sock = socket(PF_INET, SOCK_STREAM, 0);
	}
	~CServerSocket(){
		closesocket(m_sock);
		WSACleanup();
	}
	BOOL InitSockEnv() {
		WSADATA data;
		if (WSAStartup(MAKEWORD(1, 1), &data) != 0) return FALSE;
		return TRUE;
	}
	static void releaseInstance() {
		if (m_instance != NULL) {
			CServerSocket* tmp = m_instance;
			m_instance = NULL;
			delete tmp;
		}
	}
	static CServerSocket* m_instance;
	class CHelper {
	public:
		CHelper() {
			CServerSocket::getInstance();
		}
		~CHelper() {
			CServerSocket::releaseInstance();
		}
	};

	static CHelper m_helper;
	
};


//extern CServerSocket server;