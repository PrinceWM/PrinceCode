#include "MyThread.h"
#include"task.h"
#include "MyThreadPool.h"
#include<cassert>
#define WAIT_TIME 20
CMyThread::CMyThread(CMyThreadPool*threadPool,int port,int amount)
:msession(amount)
{
	m_pTask=NULL;
	//m_bIsActive=false;
	m_pThreadPool=threadPool;
	dataport = port;
	recvbufflen = BUFFLEN;
	m_hEvent=CreateEvent(NULL,false,false,NULL);
	creatdatasock();
	m_bIsThreadExit=0;
	
}

//bool CMyThread::m_bIsActive=false;
CMyThread::~CMyThread(void)
{
	CloseHandle(m_hEvent);
	CloseHandle(m_hThread);
	if(datasock>=0)
	{
		closesocket(datasock);
	}
	printf("release port %d\n",dataport);
}

bool CMyThread::startThread()
{
	m_hThread=CreateThread(0,0,threadProc,this,0,&m_threadID);
	if(m_hThread==INVALID_HANDLE_VALUE)
	{
		return false;
	}
	return true;
}
bool CMyThread::suspendThread()
{
	ResetEvent(m_hEvent);
	return true;
}
//有任务到来，通知线程继续执行。
bool CMyThread::resumeThread()
{
	SetEvent(m_hEvent);
	return true;
}
#if 0
int CMyThread::getpoolamount()
{
	if(m_pThreadPool != NULL)
	{
		return m_pThreadPool->amount;
	}
	else
	{
		return -1;
	}
	
}
#endif
int CMyThread::creatdatasock()
{
	int ret = 0;
	int theTCPWin = (2*1024*1024);
	int len = sizeof(theTCPWin);
	struct sockaddr_in localadd;
#if 0
	WSADATA wsaData;
	int rc = WSAStartup(MAKEWORD(1,1), &wsaData);
	//int rc = WSAStartup( 0x202, &wsaData );	
	if (rc == SOCKET_ERROR)
	{
		return -1;
	}
#endif
	datasock = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );

	if(datasock == INVALID_SOCKET)
	{
		printf("server sock fail \n");
		return -1;
	}

	ret = setsockopt( datasock, SOL_SOCKET, SO_RCVBUF,(char*) &theTCPWin, sizeof( theTCPWin ));
	ret = getsockopt( datasock, SOL_SOCKET, SO_RCVBUF,(char*) &theTCPWin, &len );
	printf("data recv buff:%d rc=%d error=%d\n",theTCPWin,ret,WSAGetLastError());
	int boolean = 1; 
	len = sizeof(boolean);
	setsockopt( datasock, SOL_SOCKET, SO_REUSEADDR, (char*) &boolean, len );

	// bind socket to server address

	memset(&localadd,0,sizeof(localadd));
	localadd.sin_family = AF_INET;
	localadd.sin_port = htons(dataport);
	localadd.sin_addr.s_addr = INADDR_ANY;


	ret = bind( datasock, (sockaddr*) &localadd, sizeof(localadd) );
	if(ret == SOCKET_ERROR)
	{
		printf("server socket bind error \n");
		return -1;
	}
	return 0;
}

CMyThreadPool* CMyThread::getbelongthreadpool()
{
	return m_pThreadPool;
}
DWORD WINAPI CMyThread::threadProc( LPVOID pParam )
{
	CMyThread *pThread=(CMyThread*)pParam;
	while(pThread->m_bIsThreadExit == 0)
	{
		printf("wait on thread %d \n",pThread->dataport);
 		DWORD ret=WaitForSingleObject(pThread->m_hEvent,INFINITE);		
		printf("wait finish thread %d \n",pThread->dataport);
		if(pThread->m_bIsThreadExit != 0)
		{
			break;
		}
		if(ret==WAIT_OBJECT_0)
		{
			//call a session
			if(pThread->m_pTask)
			{
				pThread->m_pTask->taskProc(pThread);//执行任务。
				//when exit taskProc ,it means,no session transfer at this thread
				//delete pThread->m_pTask;//用户传入的空间交由用户处理，内部不处理。如从CTask从堆栈分配，此处会有问题。
				pThread->m_pTask=NULL;
				//pThread->m_pThreadPool->SwitchActiveThread(pThread);
			}
		}
		else
		{
			printf("WaitForSingleObject ret = %d\n",ret);
		}
	}
	pThread->m_bIsThreadExit = 2 ;
	printf("exit thread port=%d\n",pThread->dataport);
	return 0;
}
//将任务关联到线程类。
bool CMyThread::assignTask( CTask*pTask )
{
	assert(pTask);
	if(!pTask)
		return false;
	m_pTask=pTask;
	
	return true;
}
//开始执行任务。
bool CMyThread::startTask()
{
	resumeThread();
	return true;
}
