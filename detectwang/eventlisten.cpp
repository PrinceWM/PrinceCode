#include "eventlisten.h"
#include "ThreadpoolLib-master/TestTask.h"
#if 0
#include <winsock2.h>  //windows
#include <windows.h>
#include <ws2tcpip.h>
#endif
eventlisten::eventlisten(CMyThreadPool* threadpool,int port)
{
	if(threadpool == NULL)
	{
		printf("threadpool is null \n");
		return;
	}
	mlistensock = -1;
	memset(recvbuff,0,BUFFLEN);
	recvbufflen = BUFFLEN;
	mport = port;
	exitflag = true;
	mthreadpool = threadpool;
}

eventlisten::~eventlisten()
{
	if(mlistensock >= 0)
	{
		closesocket(mlistensock);
		mlistensock = -1;
	}
}


int eventlisten::creatlistensock()
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
	mlistensock = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );

	if(mlistensock == INVALID_SOCKET)
	{
		printf("server sock fail \n");
		return -1;
	}

	ret = setsockopt( mlistensock, SOL_SOCKET, SO_RCVBUF,(char*) &theTCPWin, sizeof( theTCPWin ));
	ret = getsockopt( mlistensock, SOL_SOCKET, SO_RCVBUF,(char*) &theTCPWin, &len );
	printf("server recv buff:%d rc=%d error=%d\n",theTCPWin,ret,WSAGetLastError());
	int boolean = 1;
	len = sizeof(boolean);
	setsockopt( mlistensock, SOL_SOCKET, SO_REUSEADDR, (char*) &boolean, len );

	// bind socket to server address

	memset(&localadd,0,sizeof(localadd));
	localadd.sin_family = AF_INET;
	localadd.sin_port = htons(mport);
	localadd.sin_addr.s_addr = INADDR_ANY;


	ret = bind( mlistensock, (sockaddr*) &localadd, sizeof(localadd) );
	if(ret == SOCKET_ERROR)
	{
		printf("server socket bind error \n");
		return -1;
	}
	return 0;

}

int eventlisten::dealevent()
{
	fd_set fdsr ;
	int ret = -1;
	struct sockaddr_in clientadd;
	int clientaddlen = sizeof(clientadd);
	struct timeval tv;
	int port = -1;
	int sessionid = -1;
	int randid = -1;
	CMyThread* pThread = NULL;
	// timeout setting  
	tv.tv_sec = 0;  
	tv.tv_usec = 20*1000;//20 ms timeout  
	while(exitflag)
	{
		FD_ZERO(&fdsr);  
		FD_SET(mlistensock, &fdsr);

		ret = select(mlistensock + 1, &fdsr, NULL, NULL, &tv);
		if (ret < 0) 
		{  
			printf("select error \n");  
			break;		
		} 
		else if (ret == 0) 
		{  
			continue;  		
		}
		if (FD_ISSET(mlistensock, &fdsr))
		{			
			ret = (int)recvfrom( mlistensock, recvbuff, recvbufflen, 0,(struct sockaddr*) &clientadd, /*(socklen_t *)*/&clientaddlen );		
			if(ret < 0)
			{
				printf("recvform error %d \n",WSAGetLastError());
			}
			if(memcmp(recvbuff,"request",strlen("request")+1)==0)
			{
				ret = mthreadpool->m_ContrSource.getfreesource(&port,&sessionid);
				if(ret == 0)
				{//get source creat transfer
					srand((unsigned)time(NULL));
					randid = rand();
					//get matching port thread
					pThread = mthreadpool->findThread(port);					
					if(pThread == NULL)
					{
						printf("find thread error \n");
					}
					//set thread session info
					pThread->msession.setsession(sessionid,randid);
					CDataTask *p=new CDataTask(0,port);
					//add task to get thread
					mthreadpool->addTask(p,pThread);
										
					((int *)(recvbuff))[0] = ntohl( port );
					((int *)(recvbuff))[1] = ntohl( sessionid );
					((int *)(recvbuff))[2] = ntohl( randid );						
					printf("send id \n");
					ret = sendto( mlistensock, recvbuff,recvbufflen, 0,(struct sockaddr*) &clientadd, clientaddlen);								
				}
				else if(ret == 1)
				{
					srand((unsigned)time(NULL));
					randid = rand();
					//get matching port thread
					pThread = mthreadpool->findThread(port);					
					if(pThread == NULL)
					{
						printf("find thread error \n");
					}
					//set thread session info
					pThread->msession.setsession(sessionid,randid);

					((int *)(recvbuff))[0] = ntohl( port );
					((int *)(recvbuff))[1] = ntohl( sessionid );
					((int *)(recvbuff))[2] = ntohl( randid );						
					printf("send id thread already work \n");
					ret = sendto( mlistensock, recvbuff,recvbufflen, 0,(struct sockaddr*) &clientadd, clientaddlen);								

				}
				else
				{//no source send bye
					memcpy(recvbuff,"bye",strlen("bye")+1);
					printf("send bye \n");
					ret = sendto( mlistensock, recvbuff,recvbufflen, 0,(struct sockaddr*) &clientadd, clientaddlen);
				}
			}
		}
	}
	return 0;
}
