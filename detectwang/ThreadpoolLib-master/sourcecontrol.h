#pragma once
#include "MyMutex.h"

#define MAXPORT 10
#define MAXSESSION 3

typedef struct _SOURCE
{
	int port;
	int sessionid;
	bool isuse;
}Source;

class CSourcControl
{
public:
	CSourcControl(int startport);
	~CSourcControl(void);
	int getfreesource(int *port,int *sessionid);
	int setfreesource(int port,int sessionid);
private:
	CMyMutex mutext;
	Source source[MAXPORT][MAXSESSION];
};

