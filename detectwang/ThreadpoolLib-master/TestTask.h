#pragma once
#include "task.h"
#include "MyThread.h"

#include "../eventlisten.h"
//#include "timestamp.h"


class CDataTask :public CTask
{
public:
	CDataTask(int id,int port);
	~CDataTask(void);	
	//int creatdatasock();
	virtual void taskProc(CMyThread* ptr);
	
private:
	
	//int datasock;
	//int dataport;
	//bool datathreadexitflag;
	//char recvbuff[BUFFLEN];
	//int recvbufflen;
};

