#pragma once
#include "common.h"
class sendevent
{
public:
	sendevent();
	~sendevent();
	int sendpacket();
	virtual int recvpacket();
	int setsendadd(short port ,char* ipadd);
	int setsendadd(struct sockaddr_in add){memcpy(&sendaddr,&add,sizeof(add));return 0;};
	int setpack(SENDPACK tpack);
	int bindport(short port);
	int checkevent(struct timeval tv);
	int getsock(){return sock;};
private:
	SENDPACK pack;
	SOCKADDR_IN sendaddr;
	int sock;
};