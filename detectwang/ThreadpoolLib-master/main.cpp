#include <iostream>
#include "MyThreadPool.h"
#include "MyThread.h"
#include"TestTask.h"
#include "../eventlisten.h"
#if 0
#include <winsock2.h>  //windows
#include <windows.h>
#include <ws2tcpip.h>
#endif
int main(int argc,char**argv)
{	
	CDataTask*p=NULL;
	CMyThreadPool* threadpool = new CMyThreadPool(10);

	eventlisten mlisten(threadpool,50000);
	mlisten.creatlistensock();
	mlisten.dealevent();
	delete threadpool;
#if 0
	CDataTask*p=NULL;
	CMyThreadPool* threadpool = new CMyThreadPool(10);

	for(int i=0;i<100;i++)
	{
 		p=new CDataTask(i,0);
		threadpool->addTask(p,PRIORITY::NORMAL,);
	}
	p=new CDataTask(102200,0);
	threadpool->addTask(p,PRIORITY::HIGH);
	//threadpool.destroyThreadPool();
	//主线程执行其他工作。
	{
		Sleep(1000*1000);
	}
#endif
	return 0;
}