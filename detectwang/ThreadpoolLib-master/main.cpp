#include <iostream>
#include "MyThreadPool.h"
#include "MyThread.h"
#include"TestTask.h"
#if 0
int test()
{
	int i = 0;
	HANDLE m_hEvent=CreateEvent(NULL,false,false,NULL);
	while(i++ < 10)
	{
		DWORD ret=WaitForSingleObject(m_hEvent,INFINITE);
		if(ret==WAIT_OBJECT_0)
		{
			SetEvent(m_hEvent);
		}
		printf("i=%d \n",i);
		Sleep(1000);
	}
	CloseHandle(m_hEvent);
}

#endif
int main(int argc,char**argv)
{
#if 0
	int i = 0;
	HANDLE m_hEvent=CreateEvent(NULL,false,true,NULL);
	while(i++ < 10)
	{
		DWORD ret=WaitForSingleObject(m_hEvent,INFINITE);
		if(ret==WAIT_OBJECT_0)
		{
			//SetEvent(m_hEvent);
		}
		printf("i=%d \n",i);
		Sleep(1000);
	}
	CloseHandle(m_hEvent);
while(1);
#endif
	CTestTask*p=NULL;
	CMyThreadPool threadpool(10);
	for(int i=0;i<100;i++)
	{
 		p=new CTestTask(i);
		threadpool.addTask(p,PRIORITY::NORMAL);
	}
	p=new CTestTask(102200);
	threadpool.addTask(p,PRIORITY::HIGH);
	//threadpool.destroyThreadPool();
	//主线程执行其他工作。
	{
		Sleep(1000*1000);
	}
	
	return 0;
}