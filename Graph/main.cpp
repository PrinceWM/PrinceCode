
#include <stdio.h>  
#include <stdlib.h>  
#include <iostream>
#include <queue>
#include <string>
using namespace std;
#include "graph.h"
#include "IPLocation.h"
int main()  
{  
	//extern void test();
	//test();
	MGraph G;  
	int index;
	int i;
#define MAXIP 2
	int verlist[MAXIP]/*={e_ZHEJIANG,e_HAINAN}*/;
	int listnum = MAXIP;

	char *iplist[MAXIP]={"61.243.64.0","59.48.134.0"};
	creatgraph(&G);

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

	for(i = 0;i<MAXIP;i++)
	{
		IpQueryCxt* ctx  =  IPLocation(iplist[i] ,0);

		DWORD dwRet = WaitForSingleObject(ctx->threadid, 50000000);
		if(dwRet == WAIT_OBJECT_0)
		{
			printf("Thread exit success!");
		}
		index = convertproindex(ctx->replay->provice);
		if(index>=0)
		{
			verlist[i] = index;
		}
		printf("\n index = %d carrir = %s,city=%s,pro=%s\n",\
			index,ctx->replay->carrir,ctx->replay->city,ctx->replay->provice);
		destroyQeuryCxt(ctx);
	}

	BFSTraverse(G,e_SHANXI,verlist,listnum);
	system("pause");
	return 0;  
}  