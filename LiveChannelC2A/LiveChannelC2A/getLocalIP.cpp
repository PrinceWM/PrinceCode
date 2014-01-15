#include "stdafx.h"
#include "getLocalIP.h"
#include "message.h"
#include <stdio.h>

#ifndef WIN32
#include <sys/types.h>  
#include <sys/socket.h>  
#include <netinet/in.h>  
#include <arpa/inet.h> 
#endif

#include <string.h>



int getLocalIp(SOCKET sock,sockaddr_in conn_addr,sockaddr_in& addrMy)
{

	//sockaddr_in descon_sock_addr;
	///*sockaddr descon_sock_addr;*/
	//descon_sock_addr.sin_addr.S_un.S_addr =  inet_addr(ip);
	//descon_sock_addr.sin_family = AF_INET;
	//descon_sock_addr.sin_port = htons(13709);
	if (connect(sock,
		( sockaddr *)&conn_addr,
		sizeof(conn_addr)) < 0) 
	{
		messager(msgERROR, "getLocalIp   connect :failed");
		return -1;
	} 


	memset(&addrMy,0,sizeof(addrMy));
	int len = sizeof(addrMy);

#ifdef WIN32
	int ret = getsockname(sock,(sockaddr*)&addrMy,&len);
#else
	int ret = getsockname(sock,(sockaddr*)&addrMy,(socklen_t*)&len);
#endif
	if (ret != 0)
	{
		messager(msgERROR, "getLocalIp  Getsockname Error!");
		return -1;

	}

	messager(msgERROR, "getLocalIp  Current Socket IP:%s",inet_ntoa(addrMy.sin_addr));
	fflush(stdout);

	return 0;
}


int getLocalIpByUdp(sockaddr_in& addrMy)
{

	sockaddr_in con_sock_addr;
	char* ip = "112.124.0.75"; 
	int  port = 13709;

	SOCKET sock = socket( AF_INET , SOCK_DGRAM , IPPROTO_UDP ) ;



#ifdef WIN32
	con_sock_addr.sin_addr.S_un.S_addr =  inet_addr(ip);
#else 
	con_sock_addr.sin_addr.s_addr = inet_addr(ip);
#endif
	con_sock_addr.sin_port = htons(port);
	con_sock_addr.sin_family = AF_INET;

	return getLocalIp(sock,con_sock_addr,addrMy);
}