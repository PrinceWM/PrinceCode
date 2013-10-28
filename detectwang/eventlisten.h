#pragma once
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include <winsock2.h>  //windows
#include <windows.h>
#include <ws2tcpip.h>
#define  BUFFLEN  512


class eventlisten
{
public:
	eventlisten(int port);
	~eventlisten();
	int dealevent();
	int creatlistensock();
private:
	int mlistensock;
	int mport;
	char recvbuff[BUFFLEN];
	int recvbufflen;
	bool exitflag;
};
