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
	CMyThread(CMyThreadPool*threadPool,int port,int amount);
	~CMyThread(void);
public:
	bool startThread();
	bool suspendThread();
	bool resumeThread();
	bool assignTask(CTask*pTask);
	bool startTask();
	int creatdatasock();
	//int getpoolamount();
	CMyThreadPool* getbelongthreadpool();
	//int udpupdatesession( int mAmount);
	//int udpclearsession(int index );
	static DWORD WINAPI threadProc(LPVOID pParam);
	DWORD m_threadID;
	HANDLE m_hThread;
	int m_bIsThreadExit;

	int datasock;
	int dataport;	
	char recvbuff[BUFFLEN];
	int recvbufflen;

	/*这个变量是多线程使用的，但是未加锁，因为m_ContrSource这个资源是
	加锁的，从而保护了msession的数据并行性，所以在set session前必须先 
	m_ContrSource getsource	，clear session 先操作，然后再m_ContrSource free操作*/

	CMySession msession;
	
	//SESSIONINFO sessioninfostore[MAXSESSION];
	//long tmptime ;
	//int lastlen ;
private:

	HANDLE m_hEvent;
	CTask*m_pTask;
	CMyThreadPool*m_pThreadPool;	
};
