#include "MyThread.h"
#include"task.h"
#include "MyThreadPool.h"
#include<cassert>
#define WAIT_TIME 20
CMyThread::CMyThread(CMyThreadPool*threadPool,int port)
{
	m_pTask=NULL;
	//m_bIsActive=false;
	m_pThreadPool=threadPool;
	dataport = port;
	m_hEvent=CreateEvent(NULL,false,false,NULL);
	creatdatasock();
	m_bIsExit=false;
	
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


int CMyThread::creatdatasock()
{
	int ret = 0;
	int theTCPWin = (2*1024*1024);
	int len = sizeof(theTCPWin);
	struct sockaddr_in localadd; 
	WSADATA wsaData;
	int rc = WSAStartup(MAKEWORD(1,1), &wsaData);
	//int rc = WSAStartup( 0x202, &wsaData );	
	if (rc == SOCKET_ERROR)
	{
		return -1;
	}
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
#if 0
int CMyThread::udpclearsession(int index )
{
	if(index >= MAXSESSION)
	{
		printf("clear index is too big \n");
		return -1;
	}
	sessioninfostore[index].udpsessionuse = false;
	sessioninfostore[index].length = 0;
	sessioninfostore[index].transfertime.set(0,0);
	sessioninfostore[index].currenttime.set(0,0);
	sessioninfostore[index].applytime.set(0,0);
	memset(&sessioninfostore[index].clientadd,0,sizeof(sessioninfostore[index].clientadd));
	sessioninfostore[index].clientaddlen = 0;
	//sessionnum--;
	return 0;
}


#define BORDER_TIME 10

//int tmprecvnumpersec[MAX_NUM]={0};
//int tmpseccount = 0;
int CMyThread::udpupdatesession( int mAmount)
{
	int itemp;
	long time = 0;

	long msfeed = 0;
	double speed = 0.0;
	datagram* serverdgmbuff = (datagram*)recvbuff;
	int ret = 0;
	struct timeval currentTime;
	gettimeofday(&currentTime,NULL);
	for(itemp = 0;itemp<MAXSESSION;itemp++)
	{
		if(sessioninfostore[itemp].udpsessionuse ==  true)
		{
			sessioninfostore[itemp].currenttime.set(currentTime.tv_sec,currentTime.tv_usec);
			if(sessioninfostore[itemp].length > 0)
			{//check start transfer session time ,data				

				time = sessioninfostore[itemp].currenttime.subUsec(sessioninfostore[itemp].transfertime);				
				if(time >= (long)tmptime)
				{
					tmptime += 1e6;//1 sec 				
					tmprecvnumpersec[tmpseccount] = (int)((sessioninfostore[itemp].length-lastlen)/recvbufflen);
					lastlen = sessioninfostore[itemp].length;
					printf("time %d tmptime %d pack recv %d\n",time,tmptime,tmprecvnumpersec[tmpseccount]/*(sessioninfostore[itemp].length-lastlen)/serverbufflen*/);
					tmpseccount++;
				}

				if(time >= ((mAmount/100)+BORDER_TIME)*(1e6))//ns
				{
					if(tmpseccount<10)
					{
						printf("******************\n");
						printf("********%d**********\n",tmpseccount);
						printf("******************\n");
					}
					printf("send speed id\n");
					msfeed = sessioninfostore[itemp].transfertime.delta_usec();
					printf("end time %d: %d %d \n",itemp,sessioninfostore[itemp].transfertime.getSecs(),sessioninfostore[itemp].transfertime.getUsecs());
					//speed = (((double)(sessioninfostore[itemp].length*8)/(1024*1024))/((double)msfeed/(1000*1000)));//Mbit/sec
					speed = ((double)(sessioninfostore[itemp].length*8)/(1024*1024))/(mAmount/100);
					printf("speed = %f sessioninfostore[i].length=%d msfeed=%ld recvpack=%d\n",speed,sessioninfostore[itemp].length,msfeed,sessioninfostore[itemp].length/recvbufflen);

					serverdgmbuff->id      = htonl( /*datagramID*/-2 ); 
					serverdgmbuff->send_sec  = htonl( currentTime.tv_sec ); 
					serverdgmbuff->send_sec = htonl( currentTime.tv_usec ); 				
					serverdgmbuff->speed = htonl((int)(speed*1000));


					serverdgmbuff->seccount = htonl( tmpseccount );
					int i;
					for(i = 0;i<tmpseccount;i++)
					{
						serverdgmbuff->recvnumpersec[i] = htonl(tmprecvnumpersec[i]);
					}
					memset(tmprecvnumpersec,0,sizeof(tmprecvnumpersec));
					tmpseccount = 0;
					ret = sendto( datasock, recvbuff,sizeof(datagram)/*serverbufflen*/, 0,(struct sockaddr*) &sessioninfostore[itemp].clientadd, sessioninfostore[itemp].clientaddlen);
					if(ret > 0)
					{
						udpclearsession(itemp);
						tmptime = (((mAmount/100+1))*(1e6));
						lastlen = 0;
						return 1;
					}
					else
					{
						printf("final send to error %d\n",WSAGetLastError());
						return -1;
					}
				}
			}
			else if(sessioninfostore[itemp].length == 0)
			{//check not start transfer session time ,if time out close this session	
				time = sessioninfostore[itemp].currenttime.subUsec(sessioninfostore[itemp].applytime);				
				if(time >= ((mAmount/100))*(1e6))
				{//this session not work in amount time len so clear it
					printf("clear not use session\n");
					udpclearsession(itemp);
				}				
			}
		}
	}
	return 0;
}
#endif
DWORD WINAPI CMyThread::threadProc( LPVOID pParam )
{
	CMyThread *pThread=(CMyThread*)pParam;
	while(!pThread->m_bIsExit)
	{
 		DWORD ret=WaitForSingleObject(pThread->m_hEvent,INFINITE);		
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
	}
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
