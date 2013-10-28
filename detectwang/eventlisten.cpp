#include "eventlisten.h"

eventlisten::eventlisten(int port)
{
	mlistensock = -1;
	memset(recvbuff,0,BUFFLEN);
	recvbufflen = BUFFLEN;
	mport = port;
	exitflag = true;
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

	FD_ZERO(&fdsr);  
	FD_SET(mlistensock, &fdsr);

	// timeout setting  
	tv.tv_sec = 0;  
	tv.tv_usec = 20*1000;//20 ms timeout  
	while(exitflag)
	{
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
			ret = (int)recvfrom( mlistensock, recvbuff, recvbufflen, 0,(struct sockaddr*) &clientadd, (socklen_t *)&clientaddlen );		
			if(ret < 0)
			{
				printf("recvform error %d \n",WSAGetLastError());
			}
			if(memcmp(recvbuff,"request",strlen("request")+1)==0)
			{
#if 0
				//get id 
				{
					//ok
					//rand a id
					//send a port
					//insert this session to thread pool
				}
				else
				{
					//fail return
				}
#endif
			}
		}
	}
	return 0;
}
