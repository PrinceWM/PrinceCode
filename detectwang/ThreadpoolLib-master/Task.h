#pragma once
#include "MyThread.h"
class CTask
{
public:
	CTask(int id);
	~CTask(void);
public:
	virtual void taskProc(CMyThread* ptr)=0;
	int getID();
private:
	int m_ID;
};

