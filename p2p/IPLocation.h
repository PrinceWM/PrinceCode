#ifndef  __IPLocation_H_
#define  __IPLocation_H_

#ifdef  WIN32
#include <winsock.h> 
#else
#include <pthread.h>
#include <sys/socket.h>  
#endif
#include <string>
#include "def.h"
#define  CARRIR_LEN 16
#define  PROVICE_LEN 32
#define  CITY_LEN 32

typedef  struct  
{
	char carrir[CARRIR_LEN];
	char provice[PROVICE_LEN];
	char city[CITY_LEN];
	//char ip[16];
}IpQueryReplay;

/*
//IPLocation 接口是异步的，完成之后会调用queryCb

1.同步调用

	IpQueryCxt* ctx  =  IPLocation("61.243.64.0" ,0);

	#ifdef WIN32
		DWORD dwRet = WaitForSingleObject(ctx->threadid, 50000000);
		if(dwRet == WAIT_OBJECT_0)
		{
			printf("Thread exit success!");
		}
	#else
		void *thread_result;
		pthread_join(ctx->threadid,&thread_result);
	#endif

	
	ctx->replay;里面是结果，在这里把结果取出来。

	destroyQeuryCxt(ctx);


2.异步调用
	void queryCb(IpQueryReplay*replay,int result)
	{
		printf("\nqueryCb result:%d\n",result);	
		destroyQeuryCxt(ctx);
	}

	
	IpQueryCxt* ctx  =  IPLocation("61.243.64.0" ,queryCb);

*/



//返回查询结果，result == 0 ，查询成功，否则查询失败。
typedef void (*IpQueryNotifyCB)(IpQueryReplay*,int result);

typedef struct  
{
#ifdef WIN32
	HANDLE threadid;
#else
	pthread_t threadid;
#endif

	IpQueryNotifyCB  cb;
	char* ip;
	IpQueryReplay* replay;

	int quit;
	SOCKET sock;

	int result;
}IpQueryCxt;

IpQueryCxt* IPLocation(const char* ip,IpQueryNotifyCB cb);
void destroyQeuryCxt(IpQueryCxt* ctx);


int synchronGetLocation(const char* ip,IpQueryReplay& replay);








#endif //__IPLocation_H_
