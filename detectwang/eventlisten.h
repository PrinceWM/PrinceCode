#pragma once
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include "ThreadpoolLib-master/MyThreadPool.h"
#define  BUFFLEN  512


class eventlisten
{
public:
	eventlisten(CMyThreadPool* threadpool,int port);
	~eventlisten();
	int dealevent();
	int creatlistensock();
private:
	int mlistensock;
	int mport;
	char recvbuff[BUFFLEN];
	int recvbufflen;
	bool exitflag;
	CMyThreadPool* mthreadpool;
};
