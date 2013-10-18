#pragma once
#include "common.h"
class client
{
public:
	client(thread_Settings *inSettings);
	~client( void );
	int run(void);
	int Connect(void);
	int write_UDP_FIN(void);
	void initiateserver(void);
	int setsock_windowsize( int inSock, int inTCPWin, int inSend ) ;
	void storespeed(int speed);
	int checkconnectack();

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
	int speed ;	
};
