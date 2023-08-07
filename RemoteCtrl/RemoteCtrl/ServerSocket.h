#pragma once
#include "pch.h"
#include "framework.h"

class CPacket {
public:
	CPacket():sHead(0),nLength(0),sCmd(0),sSum(0){}
	CPacket(const CPacket& packet) {
		sHead = packet.sHead;
		nLength = packet.nLength;
		sCmd = packet.sCmd;
		strData = packet.strData;
		sSum = packet.sSum;
	}
	CPacket(const BYTE* pData, size_t& nsize):sHead(0), nLength(0), sCmd(0), sSum(0) {
		size_t i = 0;
		for (; i < nsize; i++) {
			if (*(WORD*)(pData + i) == 0xFEFF) {
				sHead = *(WORD*)(pData + i);
				i += 2;
				break;
			}
		}
		if (i +4+2+2> nsize) {//�����ݿ��ܲ�ȫ�����߰�ͷδ��ȫ�����յ�
			nsize = 0;
			return;
		}
		nLength = *(DWORD*)(pData + i); i += 4;
		if (nLength + i > nsize) {//��δ��ȫ���յ����ͷ��أ�����ʧ��
			nsize = 0;
			return;
		}
		sCmd = *(WORD*)(pData + i); i += 2;
		if (nLength > 4) {
			strData.resize(nLength - 2 - 2);
			memcpy((void*)strData.c_str(), pData + i, nLength - 4);
			i += nLength - 4;
		}
		sSum = *(DWORD*)(pData + i); i += 2;
		WORD sum;
		for (size_t j = 0; j < strData.size(); j++) {
			sum += BYTE(strData[j]) & 0XFF;
		}
		if (sum == sSum) {
			nsize = i;
			return;
		}
		nsize = 0;
	}
	~CPacket(){}
	CPacket operator=(const CPacket& packet) {
		if (this!=&packet) {
			sHead = packet.sHead;
			nLength = packet.nLength;
			sCmd = packet.sCmd;
			strData = packet.strData;
			sSum = packet.sSum;
		}
		return *this;
	}
public:
	WORD sHead;//��ͷ���̶�λFEFF
	DWORD nLength;//�����ȣ��ӿ������ʼ����У�������
	WORD sCmd;//��������
	std::string strData;//������
	WORD sSum;//��У��
};
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
		//�� 
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
#define BUFFER_SIZE 4096
	int  DealCommand() {
		if (m_client == -1)return -1;
		char* buffer = new char[BUFFER_SIZE];
		memset(buffer, 0, BUFFER_SIZE);
		size_t index = 0;
		while (true) {
			size_t len= recv(m_client, buffer+index, BUFFER_SIZE -index, 0);
			if (len <= 0) {
				return -1;
			}
			index += len;
			len = index;
			m_packet=CPacket((BYTE*)buffer, len);
			if (len > 0) {
				memmove(buffer, buffer + len, BUFFER_SIZE - len);
				index -= len;
				return m_packet.sCmd;
			}
		}
		return -1;
	}
	bool Send(const char* pData, int nsize) {
		if (m_client == -1)return false;
		return send(m_client, pData, nsize, 0)>=0;
	}
private:
	SOCKET m_sock;
	SOCKET m_client;
	CPacket m_packet;
	CServerSocket& operator=(const CServerSocket&ss) {}
	CServerSocket(const CServerSocket&ss) {
		m_sock = ss.m_sock;
		m_client = ss.m_client;
	}
	CServerSocket(){
		if (InitSockEnv() == FALSE) {
			MessageBox(NULL, _T("�޷���ʼ���׽��ֻ���,�����������ã�"), _T("��ʼ������"),MB_OK|MB_ICONERROR);
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