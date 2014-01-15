#include "logbuff.h"
#include <stdio.h>
#include<iostream>
using std::cin;
typedef struct _PARA
{
	logbuff log;
	HANDLE event;
	bool flag;
	 _PARA(int size):log(size)
	{
		flag = 1;
		event = CreateEvent(NULL,false,false,NULL);
	};

	~_PARA()
	{
		printf("~_PARA");
		//~logbuff();
		CloseHandle(event);
	};
}PARA;

DWORD WINAPI w_thread(void* para)
{
	printf("w_thread\n");
	PARA* pa = (PARA*)para;
	char str[MAX_LOG_SIZE];
	int i=0;
	while(pa->flag)
	{
		printf("input:");
		cin>>str;
		printf("str: %s strlen(str)=%d\n",str,strlen(str));
		if(memcmp(str,"quit",strlen(str))==0)
		{
			pa->flag = 0;
			break;
		}
		pa->log.log_write(str);
		if(++i>=5)
		{
			SetEvent(pa->event);
		}
	}
	return 0;
}

DWORD WINAPI r_thread(void* para)
{
	printf("r_thread\n");
	PARA* pa = (PARA*)para;
	while(pa->flag)
	{
		DWORD ret=WaitForSingleObject(pa->event,INFINITE);		
		//printf("wait finish thread \n");
		if(ret==WAIT_OBJECT_0)
		{
			pa->log.log_read();
		}
		else
		{
			printf("WaitForSingleObject ret = %d\n",ret);
		}
	}
	return 0;
}


int main(int argc,char** argv)
{
	PARA pa(MAX_LOG_SIZE);
	CreateThread(0,0,w_thread,&pa,0,NULL);
	CreateThread(0,0,r_thread,&pa,0,NULL);
	while(1);
	//pa.~_PARA();
	//PARA pa();
	//logbuff(128);
	//HANDLE m_hEvent;


#if 0
	if(memcmp(argv[1],"-w",3) == 0)
	{
	
	}
	else if(memcmp(argv[1],"-r",3) == 0)
	{

	}
#endif
	//LOGGER lg;
	//char msg[0];
	//printf(" %d ",sizeof(msg[0]));
	system("pause");
}