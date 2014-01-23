#include "common.h"
#include "dealevent.h"
#include "eventrelay.h"
#include "sendevent.h" 
#include <time.h>

#define DEFAULT_THREAD_NUM 3
HANDLE	g_hThread[DEFAULT_THREAD_NUM] = { NULL };

DWORD WINAPI sendeventthread( LPVOID lpParam )
{
	printf("sendeventthread\n");
	sendevent* sevent = new sendevent;
	sevent->setsendadd(RELAY_PORT,"127.0.0.1");
	SENDPACK pack;
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 100*1000;//100 ms
	int ret = 0;
	while(1)
	{		
		ret = sevent->checkevent(tv);
		if(ret >0)
		{
			//check recv pack info
		}
		else
		{
			pack.packtype = PAYLOAD_PACK;
			srand((UINT)time( NULL ));	
			pack.uid = rand();
			pack.playloadlen = MAX_PAYLOAD;
			memset(pack.payload,1,pack.playloadlen);

			sevent->setpack(pack);
			sevent->sendpacket();
			Sleep(1000);  
		}		
	}
	return 0;
}

DWORD WINAPI eventrelaythread( LPVOID lpParam )
{
	printf("eventrelaythread\n");
	eventrelay* revent = new eventrelay;
	revent->bindport(RELAY_PORT);
	//revent->setsendadd(10001,"127.0.0.1");

	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 1000*1000;//100 ms
	int ret = 0;
	while(1)
	{		
		ret = revent->checkevent(tv);
		if(ret >0)
		{
			//check recv pack info
			revent->recvpacket();
		}
		else
		{//check packet timeout 
			revent->checktimout();
		}		
	}
	return 0;
}

DWORD WINAPI dealeventthread( LPVOID lpParam )
{
	printf("dealeventthread\n");
	dealevent* devent = new dealevent;
	devent->bindport(DEAL_PORT);
	devent->setsendadd(RELAY_PORT,"127.0.0.1");

	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 2*1000*1000;//100 ms
	int ret = 0;
	while(1)
	{		
		ret = devent->checkevent(tv);
		if(ret >0)
		{
			//check recv pack info
			devent->recvpacket();
		}
		else
		{
			
		}		
	}
	return 0;
}
LPTHREAD_START_ROUTINE addlist[DEFAULT_THREAD_NUM] = {dealeventthread,eventrelaythread,sendeventthread};



int main(int argv,char** argc)
{
#if 0
	list<int> rlist;
	list<int> blist;
	rlist.push_front(1);
	rlist.push_front(2);
	rlist.push_front(3);

	blist.assign(rlist.begin(),rlist.end());

	printf("%d %d",rlist.size(),blist.size());
	system("pause");
	return 0;

	list<int>::iterator it;
	int itmp;
	while(1)
	{
		cout<<"input:";
		cin>>itmp;
		cout<<endl;
		if(rlist.size() == 0)
		{//insert at front
			rlist.push_front(itmp);
		}
		else
		{
			for (it = rlist.begin(); it != rlist.end(); it++)
			{
				if((*it) > itmp)
				{//insert at middle position
					it = rlist.insert((it),itmp);
					break;
				}
				else if((*it) == itmp)
				{
					printf("error\n");
				}
			}

			if(it == rlist.end())
			{//insert at last
				rlist.push_back(itmp);
			}
		}
		for(it = rlist.begin(); it != rlist.end(); it++)
		{
			cout<<*it;
		}
		cout<<endl;
	}

	return 0;
#endif


	int i;
	WORD wVersionRequested;
	WSADATA wsaData;
	int err;
	wVersionRequested = MAKEWORD( 1, 1 );
	err = WSAStartup( wVersionRequested, &wsaData );
	if ( err != 0 )
	{
		return -1;
	}
	if ( LOBYTE( wsaData.wVersion ) != 1 ||
		HIBYTE( wsaData.wVersion ) != 1 )
	{
		WSACleanup( );
		return -1;
	}

	DWORD dwThreadId[DEFAULT_THREAD_NUM] = { 0 };
	for(i = 0;i<DEFAULT_THREAD_NUM;i++)
	{
		g_hThread[i] = CreateThread( NULL, 0, addlist[i], NULL, 0, &dwThreadId[i] );
		Sleep(100);
	}

	WaitForMultipleObjects( DEFAULT_THREAD_NUM, g_hThread, TRUE, INFINITE );
	for( int i = 0; i < DEFAULT_THREAD_NUM; i++ )
	{
		CloseHandle( g_hThread[i] );
		g_hThread[i] = NULL;
	}
	return 0;
}