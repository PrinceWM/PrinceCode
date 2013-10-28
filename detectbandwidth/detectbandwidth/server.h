#pragma once
#include "common.h"

typedef struct _SESSIONINFO
{
	int fd;//connection fd
	Timestamp transfertime;//this connection start transfer data time
	Timestamp currenttime;//this connection current transfer data time
	Timestamp applytime;//when session apply,then check this session use or not 
	int length;//this connection recv byte 
	bool udpsessionuse;//use for udp transfer
	iperf_sockaddr clientadd;
	int clientaddlen /*= sizeof( iperf_sockaddr )*/;
	int checkid;

}SESSIONINFO;

class server
{
public:
	server(thread_Settings *inSettings);
	~server(void);
	void recvdata(void);
	void creat(void);
	int checkselectfd(fd_set* fdset,int maxsock);
	int checkconnectfd(fd_set* fdset);
	int checkaccept(fd_set* fdset,int* maxsock);
	int udprecvdata( );
	int udpupdatesession( );
	int udpclearsession(int index );
	int clearsessioninfo(int index,fd_set* fdset);
	int getudptransferuid();
	//class listenrequest
	//{
	//public:
	//	listenrequest(int port);
	//	~listenrequest();
	//	listenling();
	//private:
	//	int requestmaxnum ;
	//	int listensock;
	//};
private:
	int serversock;
	int listenedsock;
	char* serverbuff;
	int serverbufflen;
	int serversockwin;
	iperf_sockaddr localadd;
	int localaddlen;
	bool isudp;
	long long mAmount;
	SESSIONINFO* sessioninfostore;
	int sessionnum;//how many session now use;
};
