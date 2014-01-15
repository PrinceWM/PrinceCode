/*

sockaddr_in con_sock_addr;
//char* ip = "112.124.0.75"; int  port = 13709;
//char* ip = "106.87.90.178"; int  port = 80;
char* ip = "112.124.0.76"; int  port = 13709;
//char* ip = "192.168.3.19"; int  port = 13709;    

#if 1 //udp 可以不用填写端口，地址无效也可以。connect返回值正常。
SOCKET sock = socket( AF_INET , SOCK_DGRAM , IPPROTO_UDP ) ;
#else //tcp connect不上，地址取不到。
SOCKET sock = socket( AF_INET , SOCK_STREAM , IPPROTO_TCP) ;
#endif

#ifdef WIN32
con_sock_addr.sin_addr.S_un.S_addr =  inet_addr(ip);
#else 
con_sock_addr.sin_addr.s_addr = inet_addr(ip);
#endif
con_sock_addr.sin_port = htons(port);
con_sock_addr.sin_family = AF_INET;

sockaddr_in local_addr;
getLocalIp(sock,con_sock_addr,local_addr);
*/

#ifndef  __GETLOCALIP_H_
#define  __GETLOCALIP_H_

#ifdef  WIN32
#include <winsock.h> 
#else
#include <pthread.h>
#include <sys/socket.h>  
#include <sys/types.h>  
#include <sys/socket.h>  
#include <netinet/in.h>  
#include <arpa/inet.h> 

#endif
#include "def.h"

//#ifndef SOCKET
//#define SOCKET int 
//#endif




int getLocalIpByUdp(sockaddr_in& addrMy);

int getLocalIp(SOCKET sock,sockaddr_in conn_addr,sockaddr_in& addrMy);





#endif
