#pragma once

#define WIN32_BANDTEST

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <math.h>

#ifdef WIN32_BANDTEST
//#include <winsock.h>
#include <winsock2.h>  //windows
#include <windows.h>
#include <ws2tcpip.h>
#define close( s )       closesocket( s )
#define read( s, b, l )  recv( s, (char*) b, l, 0 )
#define write( s, b, l ) send( s, (char*) b, l, 0 )

#else
#include <sys/types.h>  //linux
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include<unistd.h>

#define INVALID_SOCKET  (unsigned int)(~0)
#define SOCKET_ERROR            (-1)
#endif

#include "timestamp.h"


const long kDefault_UDPRate = 1024 * 1024; // -u  if set, 1 Mbit/sec


typedef struct datagram
{
	signed   int id      ;
	unsigned int send_sec  ;
	unsigned int send_usec ;
	int speed ;
	int udpid;
}datagram;


typedef struct sockaddr_in iperf_sockaddr;
typedef struct thread_Settings {
	// Pointers
	char*  mHost;                   // -c
	char*  mLocalhost;              // -B
	int mThreads;                   // -P
	int mTOS;                       // -S
	int mSock;
	int mBufLen;                    // -l
	int mMSS;                       // -M
	int mTCPWin;                    // -w
	int flags; 
	bool isudp;
	// enums (which should be special int's)
	//ThreadMode mThreadMode;         // -s or -c
	//ReportMode mReportMode;
	//TestMode mMode;                 // -r or -d
	// Hopefully int64_t's
	long long mUDPRate;            // -b or -u
	long long mAmount;             // -n or -t
	// doubles
	double mInterval;               // -i
	// shorts
	unsigned short mListenPort;     // -L
	unsigned short mPort;           // -p
	// chars
	char   mFormat;                 // -f
	int mTTL;                    // -T
	char pad1[2];
	// structs or miscellaneous
	iperf_sockaddr peer;
	int size_peer;
	iperf_sockaddr local;
	int size_local;
	//nthread_t mTID;
	char* mCongestion;
} thread_Settings;


#if 0
int ptr_count = 0;

inline void *ptr_malloc(unsigned int bytes)
{
	ptr_count++;
	return (void*)malloc(bytes);
}

inline void ptr_free(void* ptr)
{
	if(ptr != NULL)
	{
		ptr_count--;
		free(ptr);
	}
	else
	{
		printf("ptr is null\n");
	}
}
#endif
extern int sInterupted;
