#pragma once
//#include "common.h"
#include "timestamp.h"

//#define WIN32_BANDTEST

//#ifdef WIN32_BANDTEST
#if defined(MOHO_WIN32)

#define close( s )       closesocket( s )
#define read( s, b, l )  recv( s, (char*) b, l, 0 )
#define write( s, b, l ) send( s, (char*) b, l, 0 )

#elif defined(MOHO_X86)

#include <sys/types.h>  //linux
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include<unistd.h>

#define INVALID_SOCKET  (unsigned int)(~0)
#define SOCKET_ERROR            (-1)
#endif

#define MAX_NUM 15
typedef struct datagram
{
	signed   int id      ;
	unsigned int send_sec  ;
	unsigned int send_usec ;
	int speed ;
	int udpid;
	int randid;

	int recvnumpersec[MAX_NUM];
	int seccount;

}datagram;


typedef struct _Settings 
{
	char  mHost[100];                   // -c
	//char*  mLocalhost;              // -B
	int mThreads;                   // -P
	int mTOS;                       // -S
	int mSock;
	int mBufLen;                    // -l
	int mMSS;                       // -M
	int mTCPWin;                    // -w
	int flags; 
	bool isudp;
	bool once;
	long long mUDPRate;            // -b or -u
	int mAmount;             // -n or -t
	double mInterval;               // -i
	unsigned short mListenPort;     // -L
	unsigned short mPort;           // -p
	char   mFormat;                 // -f
	int mTTL;                    // -T
	char pad1[2];
	struct sockaddr_in peer;
	int size_peer;
	struct sockaddr_in local;
	int size_local;	
} SettingPara;


class client
{
public:
	client(SettingPara *inSettings);
	~client( void );
	int run( int sessionid,int randid ); 
	int creatsock( ) ;
	int write_UDP_FIN(void);
	void initiateserver(void);
	int setsock_windowsize( int inSock, int inTCPWin, int inSend ) ;
	void storespeed(int speed);
	int checkconnectack(int *port,int *sessionid,int* randid);
	int setsendtoport(int port);
private:
	int clientsock;
	char* clientbuff;
	int clientbufflen;
	bool isudp;
	struct sockaddr_in sockadd;
#if defined(MOHO_WIN32)
        int sockaddlen ;
#elif defined(MOHO_X86)
        socklen_t sockaddlen ;
#endif
        //int sockaddlen;
	int clientWin;
	Timestamp mEndTime;
	Timestamp lastPacketTime;
	long long udprate;
	int mAmount;
	int speed ;	
	//int clientcheckid;
};