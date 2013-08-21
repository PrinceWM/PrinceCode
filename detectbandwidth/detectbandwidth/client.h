#pragma once
#include "common.h"
class client
{
public:
	client(thread_Settings *inSettings);
	~client( void );
	void run(void);
	void Connect(void);
	void write_UDP_FIN(void);
	void initiateserver(void);
	

private:
	int clientsock;
	char* clientbuff;
	int clientbufflen;
	bool isudp;
	iperf_sockaddr sockadd;
	int sockaddlen;
	int clientWin;
	Timestamp mEndTime;
	Timestamp lastPacketTime;
	long long udprate;
	long long mAmount;

};
