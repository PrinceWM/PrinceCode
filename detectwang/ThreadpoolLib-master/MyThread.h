#pragma once
#include "windows.h"
#include "../eventlisten.h"
#include "timestamp.h"
//class CMySession;
#include "CMySession.h"
#define MAX_NUM 15
typedef struct datagram
{
	signed   int id      ;
	unsigned int send_sec  ;
	unsigned int send_usec ;
	int speed ;
	int udpid;
	int randid;

	int recvnumpersec[MAX_NUM];
	int seccount;
	
}datagram;


#define MAXSESSION 3
class CTask;
class CBaseThreadPool;
class CMyThread
{
public:
	CMyThread(CMyThreadPool*threadPool,int port);
	~CMyThread(void);
public:
	bool startThread();
	bool suspendThread();
	bool resumeThread();
	bool assignTask(CTask*pTask);
	bool startTask();
	int creatdatasock();
	CMyThreadPool* getbelongthreadpool();
	//int udpupdatesession( int mAmount);
	//int udpclearsession(int index );
	static DWORD WINAPI threadProc(LPVOID pParam);
	DWORD m_threadID;
	HANDLE m_hThread;
	bool m_bIsExit;

	int datasock;
	int dataport;	
	char recvbuff[BUFFLEN];
	int recvbufflen;

	CMySession msession;
	//SESSIONINFO sessioninfostore[MAXSESSION];
	//long tmptime ;
	//int lastlen ;
private:
	
	HANDLE m_hEvent;
	CTask*m_pTask;
	CMyThreadPool*m_pThreadPool;	
};
