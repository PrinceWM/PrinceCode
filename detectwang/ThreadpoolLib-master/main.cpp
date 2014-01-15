//#include <time.h>
#include <vld.h>
#include <iostream>
#include "MyThreadPool.h"
#include "MyThread.h"
#include"TestTask.h"
#include "../eventlisten.h"
#include "../client.h"
#if 0
#include <winsock2.h>  //windows
#include <windows.h>
#include <ws2tcpip.h>
#endif

const long kDefault_UDPRate = 1024 * 1024; 

void Settings_Initialize( SettingPara *main ) {
	// Everything defaults to zero or NULL with
	// this memset. Only need to set non-zero values
	// below.
	memset( main, 0, sizeof(SettingPara) );
	memset( main->mHost, 0, sizeof(main->mHost) );
	main->mSock = INVALID_SOCKET;
	main->mUDPRate = kDefault_UDPRate;
	main->isudp = 1;
	main->once = 0;
	main->mBufLen       = /*32 * 1024*/512;      // -l,  8 Kbyte
	main->mPort         = 50001;          // -p,  ttcp port
	main->mAmount       = 300;          // -t,  5 seconds
	main->mTTL          = 1;             // -T,  link-local TTL
	
} // end Settings



int ParseArag(int argc, char* argv[], SettingPara* setting)
{
	int ret = 0;
	int i = 0;

	for (i = 1; i < argc; i++)
	{
		if (strcmp(argv[i], "-c") == 0 )
		{//client mode
			int len = strlen(argv[i+1])+1;
			//printf("len =%d \n",len);			
			memcpy(setting->mHost ,argv[i+1], len);
			i++;
			ret = 0;			
		}
		else if(strcmp(argv[i], "-s") == 0 )
		{//server mode
			ret = 1;
		}
		else if(strcmp(argv[i], "-o") == 0 )
		{
			setting->once = 1;
		}
		else if(strcmp(argv[i], "-u") == 0 )
		{
			setting->isudp = 1;
		}
		else if(strcmp(argv[i], "-b") == 0 )
		{
			setting->isudp = 1;//-b just use for udp 
			setting->mUDPRate = atoi(argv[i+1]);
			i++;
		}
		else if(strcmp(argv[i], "-l") == 0 )
		{
			setting->mBufLen = atoi(argv[i+1]);
			i++;
		}
		else if(strcmp(argv[i], "-t") == 0 )
		{
			setting->mAmount = atoi(argv[i+1]);
			i++;
		}
		else if(strcmp(argv[i], "-p") == 0 )
		{
			setting->mPort = atoi(argv[i+1]);
			i++;
		}
		else if(strcmp(argv[i], "-w") == 0 )
		{
			setting->mTCPWin = atoi(argv[i+1]);
			i++;
		}		
	}
	return ret;
}

DWORD WINAPI testspeed( LPVOID para )
{
	int ret = 0;
	int lastspeed = 0;
	int currspeed = 0;
	int port = 0;
	int sessionid = 0;
	int randid = 0;

	SettingPara* setting = (SettingPara*)para;
	if(strlen(setting->mHost) == 0)
	{
		//memcpy(setting.mHost,"112.124.0.75",strlen("112.124.0.75"));
		memcpy(setting->mHost,"113.140.73.3",strlen("113.140.73.3"));
	}

	printf("set client mode\n");
REDETECT:
	client* clientptr = new client(setting);
	ret = clientptr->creatsock();
	//printf("client connect \n");
	if(ret != 0)
	{
		printf("client connect error \n");
	}
	else
	{		
		ret = clientptr->checkconnectack(&port,&sessionid,&randid);
		if(ret == 0)
		{
			ret = clientptr->setsendtoport( port );
			if(ret == 0)
			{				
				currspeed = clientptr->run(sessionid,randid);			
				//printf("client run ret in main = %d\n",ret);
				if (currspeed < 0)
				{
					printf("not runable \n");
				}
			}
		}
		else
		{
			printf("11check ack error\n");
		}
	}


	if(setting->once == 0 && setting->isudp && (currspeed>lastspeed))
	{
		lastspeed = currspeed;
		clientptr->storespeed(currspeed);
		setting->mUDPRate = setting->mUDPRate*2; 			
		delete clientptr;
		//Sleep(1000*3);			
		goto REDETECT;
	}
	else
	{
		delete clientptr;
	}

	if(lastspeed>=currspeed)
	{
		printf("detect speed is %d KBYTE /sec",lastspeed/8);
	}
	else
	{
		printf("detect speed is %d KBYTE /sec",currspeed/8);
	}
	return 0;
}
//CreateThread(0,0,threadProc,this,0,&m_threadID);


void test(void* para)
{
	int i = 0;
	for(i = 0;i<300;i++)
	{
		CreateThread(0,0,testspeed,para,0,NULL);
	}

}
#if 0
int iss = 0;
void test1(
		   HWND hwnd,        // handle to window for timer messages
		   UINT message,     // WM_TIMER message
		   UINT idTimer,     // timer identifier
		   DWORD dwTime)     // current system time

{
	printf(" %d \n",iss);
}
#endif
#define MAX_THREAD_NUM 10
#if 0
void *operator new[](size_t size)
{
	malloc(size);
	printf("new new\n");
	return NULL;
}
#endif
int main(int argc,char**argv)
{
	SettingPara setting;
	int ret = 0;
	int lastspeed = 0;
	int currspeed = 0;
	int port = 0;
	int sessionid = 0;
	int randid = 0;
#if 0
	size_t size=100;
	char* ptr;
	ptr = new char[size];
#endif
#ifdef WIN32_BANDTEST
	// Start winsock
	WSADATA wsaData;
	int rc = WSAStartup(MAKEWORD(1,1), &wsaData);
	//int rc = WSAStartup( 0x202, &wsaData );	
	if (rc == SOCKET_ERROR)
		return 0;
#endif
	Settings_Initialize(&setting);
	ret = ParseArag(argc,argv,&setting);
#if 0
	test((void*)( &setting));
	while(1);
#endif

#if 0
	UINT uResult = SetTimer(NULL,0,1000,(TIMERPROC) test1);
	if(uResult == 0)
	{
		printf("No timer is available at taskNeighborNodeProcesser");
	}
	while(1);
#endif


	if(ret == 0)
	{//client

		if(strlen(setting.mHost) == 0)
		{
			//memcpy(setting.mHost,"112.124.0.75",strlen("112.124.0.75"));
			memcpy(setting.mHost,"113.140.73.3",strlen("113.140.73.3"));
		}

		printf("set client mode\n");
REDETECT:
		client* clientptr = new client(&setting);
		ret = clientptr->creatsock();
		//printf("client connect \n");
		if(ret != 0)
		{
			printf("client connect error \n");
		}
		else
		{		
			ret = clientptr->checkconnectack(&port,&sessionid,&randid);
			if(ret == 0)
			{
				ret = clientptr->setsendtoport( port );
				if(ret == 0)
				{				
					currspeed = clientptr->run(sessionid,randid);			
					//printf("client run ret in main = %d\n",ret);
					if (currspeed < 0)
					{
						printf("not runable \n");
					}
				}
			}
			else
			{
				printf("check ack error\n");
			}
		}
		

		if(setting.once == 0 && setting.isudp && (currspeed>lastspeed))
		{
			lastspeed = currspeed;
			clientptr->storespeed(currspeed);
			delete clientptr;
			printf("rate *7/10 =%d speed=%d\n",(int)((double)setting.mUDPRate*7/(1024*10)) , currspeed);
			if((int)((double)setting.mUDPRate*7/(1024*10)) < currspeed)
			{
				setting.mUDPRate = setting.mUDPRate*2; 
				goto REDETECT;
			}		
			//Sleep(1000*3);			
		}
		else
		{
			delete clientptr;
		}

		if(lastspeed>=currspeed)
		{
			printf("detect speed is %d KBYTE /sec",lastspeed/8);
		}
		else
		{
			printf("detect speed is %d KBYTE /sec",currspeed/8);
		}
		system("pause");
	}
	else if(ret == 1)
	{//server
		CDataTask*p=NULL;
		CMyThreadPool* threadpool = new CMyThreadPool(MAX_THREAD_NUM,setting.mPort+1,setting.mAmount);

		eventlisten mlisten(threadpool,setting.mPort);
		mlisten.creatlistensock();
		mlisten.dealevent();
		//release all thread in thread pool
		//printf("destroyThreadPool start\n");
		threadpool->destroyThreadPool();
		//printf("destroyThreadPool finish\n");
		delete threadpool;
	}
	else 
	{
		printf("not set mode");
	}
#ifdef WIN32_BANDTEST
	rc = WSACleanup ( );	
	if (rc == SOCKET_ERROR)
	{
		printf("cleanup sock error\n");
		return -1;
	}
#endif
	printf("finish\n");
	return 0;




}