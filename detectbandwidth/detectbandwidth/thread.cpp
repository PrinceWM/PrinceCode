#include "common.h"
#ifdef WIN32_BANDTEST
#include <process.h>
#include <Windows.h> 
#else
#include <pthread.h>
#endif
int creatthread(void* func,void* para)
{
#ifdef WIN32_BANDTEST
	HANDLE hThread;  
	hThread = (HANDLE)_beginthreadex(NULL, 0, (unsigned int (__stdcall *)(void *))(func), para, 0, NULL);  
	CloseHandle(hThread);  
#else
	int s;
	int tid;
	pthread_attr_t attr;
	s = pthread_attr_init(&attr);
	pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_DETACHED);
	s = pthread_create(&tid, &attr, func, para);
	pthread_attr_destroy (&attr);
	if(s != 0)
	{
		printf(" pthread_create failed\n");
		return -1;
	}
	return 0;

#endif
	return 0;
}