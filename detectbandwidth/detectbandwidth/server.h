#pragma once
#include "common.h"
class server
{
public:
	server(thread_Settings *inSettings);
	~server(void);
	void UDPSingleServer(void);
	void creat(void);
private:
	int serversock;
	char* serverbuff;
	int serverbufflen;
	int serversockwin;
	iperf_sockaddr localadd;
	int localaddlen;

};
