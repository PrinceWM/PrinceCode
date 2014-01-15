#pragma once
#include<list>
#include "MyMutex.h"
#include "MyStack.h"
#include "MyList.h"
#include"MyQueue.h"
#include "sourcecontrol.h"
class CMyThread;
class CTask;
enum PRIORITY
{
	NORMAL,
	HIGH
};
class CBaseThreadPool
{
public:
	virtual bool SwitchActiveThread(CMyThread*)=0;
};
class CMyThreadPool:public CBaseThreadPool
{
public:
	CMyThreadPool(int num,int portstart,int paraamount);
	~CMyThreadPool(void);

public:
	virtual CMyThread* PopIdleThread();
	virtual bool SwitchActiveThread(CMyThread*);
	virtual CTask*GetNewTask();
public:
	//priorityΪ���ȼ��������ȼ������񽫱����뵽���ס�
	bool addTask( CTask*t,CMyThread*pThread );
	bool start();//��ʼ���ȡ�
	bool destroyThreadPool();
	CMyThread* findThread(int port);
	
	CSourcControl m_ContrSource;
	//int amount;
private:
	int m_nThreadNum;
	bool m_bIsExit;
	
	//CMyStack m_IdleThreadStack;
	CMyList m_ThreadList;
	//CMyQueue m_TaskQueue;
	
};

