#include "TestTask.h"
#if 0
#include <winsock2.h>  //windows
#include <windows.h>
#include <ws2tcpip.h>
#endif

CDataTask::CDataTask(int id,int port)
	:CTask(id)	 
{
	//datasock = -1;
	//dataport = port;
	//datathreadexitflag = true;
	//memset(recvbuff,0,BUFFLEN);
	//recvbufflen = BUFFLEN;
}

CDataTask::~CDataTask(void)
{
	//if(datasock >= 0)
	//{
	//	closesocket(datasock);
	//}
}

#if 0
int CDataTask::creatdatasock()
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
#endif







void CDataTask::taskProc(CMyThread* ptr)
{
	int ret = 0;
	fd_set fdsr ;
	struct sockaddr_in clientadd;
	int clientaddlen = sizeof(clientadd);
	struct timeval tv;
	//int amount = 5;
	if(ptr == NULL)
	{
		printf("ptr error \n");
		return;
	}

	// timeout setting  
	tv.tv_sec = 0;  
	tv.tv_usec = 20*1000;//20 ms timeout  
	while(ptr->m_bIsExit)
	{
		ret = ptr->msession.updatesession(AMOUNT,ptr);
		if(ret < 0)
		{
			printf("udpupdatesession eror \n");
		}

		FD_ZERO(&fdsr);  
		FD_SET(ptr->datasock, &fdsr);
		ret = select(ptr->datasock + 1, &fdsr, NULL, NULL, &tv);
		if (ret < 0) 
		{  
			printf("select error \n");  
			break;		
		} 
		else if (ret == 0) 
		{  
			continue;  		
		}
		if (FD_ISSET(ptr->datasock, &fdsr))
		{			
			ret = (int)recvfrom( ptr->datasock, ptr->recvbuff, ptr->recvbufflen, 0,(struct sockaddr*) &clientadd, &clientaddlen );		
			if(ret < 0)
			{
				printf("recvform error %d \n",WSAGetLastError());
			}

			{
				int datagramID = ntohl(((datagram*) ptr->recvbuff)->id ); 
				int transferid = ntohl(((datagram*) ptr->recvbuff)->udpid ); 
				int randid = ntohl(((datagram*) ptr->recvbuff)->randid ); 
				if(ptr->msession.sessioninfostore[transferid].udpsessionuse == false)
				{//check this session allow
					return ;
				}
				if(ptr->msession.sessioninfostore[transferid].randid != randid)
				{//check this session allow
					printf("check id check error\n");
					return;
				}

				//printf("datagramID = %d \n",datagramID);
				if ( datagramID >= 0 ) 
				{
					if(ptr->msession.sessioninfostore[transferid].length == 0)
					{
						ptr->msession.sessioninfostore[transferid].transfertime.setnow();
						memcpy(&ptr->msession.sessioninfostore[transferid].clientadd,&clientadd,sizeof(clientadd));
						ptr->msession.sessioninfostore[transferid].clientaddlen = sizeof(clientadd);
						printf("start time %d :%d %d \n",transferid,ptr->msession.sessioninfostore[transferid].transfertime.getSecs(),ptr->msession.sessioninfostore[transferid].transfertime.getUsecs());
					}
					ptr->msession.sessioninfostore[transferid].length += ret;
					//printf("time =%d abidance =%d\n",time,((mAmount/100)+4)*(1e6));
				}
			}
		}
	}
}