
//#if defined(MOHO_WIN32)
	#include "stdafx.h"
//#elif defined(MOHO_X86)
//	#include "../stdafx.h"
//#endif
#include "client.h"
#include "../mypthread.h"
#include "../message.h"
#include "../netmain.h"
extern myConfig globalConfig;
const long kDefault_UDPRate = 1024 * 1024; 

void Settings_Initialize( SettingPara *main ) 
{
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
int sendspeed(int localspeed)
{
	int len;
	char buff[1500];
	memset(buff,0,sizeof(buff));
	pduHeader* pHeader = (pduHeader*)buff;
	int* netspeed = (int *)(&buff[sizeof(pduHeader)]);

	SOCKET localSendSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	SOCKADDR_IN localIP;
	localIP.sin_family = AF_INET;
#if defined(MOHO_X86)
	localIP.sin_addr.s_addr = globalConfig.ipAddress;
#elif defined(MOHO_WIN32)
	localIP.sin_addr.S_un.S_addr = globalConfig.ipAddress;
#endif
	localIP.sin_port = htons(globalConfig.publicationPort);
	int tolen = sizeof(localIP);	
	
	pHeader->Type = GET_SPEED;
	pHeader->myUID = globalConfig.UID;
	pHeader->Length = sizeof(pduHeader) + sizeof(int);

	*netspeed = htonl(localspeed);
	len = sendto(localSendSocket, buff, pHeader->Length, 0, (SOCKADDR *)(&localIP), tolen);
	messager(msgFATAL, "sendto len=%d speed= %d ip=%s:%d",len,localspeed,inet_ntoa(localIP.sin_addr),globalConfig.publicationPort);
	//mySleep(10);
	#if defined(MOHO_X86)
	close(localSendSocket);
	#elif defined(MOHO_WIN32)
	closesocket(localSendSocket);
	#endif
	return 0;
}

#if defined(MOHO_X86)
void* detectspeed(LPVOID para)
#elif defined(MOHO_WIN32)
static DWORD WINAPI detectspeed(LPVOID para)
#endif
{	
	SettingPara* setting = NULL;
	int ret = 0;
	int lastspeed = 0;
	int currspeed = 0;
	int port = 0;
	int sessionid = 0;
	int randid = 0;
	//Settings_Initialize(&setting);
	if(para == NULL)
	{
#if defined(MOHO_X86)
            return NULL;
        #elif defined(MOHO_WIN32)
            return -1;
        #endif
        }

	setting = (SettingPara*)para;
	//ret = ParseArag(argc,argv,&setting);

	if(ret == 0)
	{//client

		if(strlen(setting->mHost) == 0)
		{
			//memcpy(setting->mHost,"112.124.0.75",strlen("112.124.0.75"));
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
					messager(msgFATAL,"client run currspeed = %d\n",currspeed);
					if (currspeed < 0)
					{						
						messager(msgFATAL, "not runable");
					}
				}
			}
			else
			{
				messager(msgFATAL, "check ack error");
			}
		}


		if(setting->once == 0 && setting->isudp && (currspeed>lastspeed))
		{
			lastspeed = currspeed;
			clientptr->storespeed(currspeed);
			delete clientptr;
			printf("rate *7/10 =%d speed=%d\n",(int)((double)setting->mUDPRate*7/(1024*10)) , currspeed);
			if((int)((double)setting->mUDPRate*7/(1024*10)) < currspeed)
			{
				setting->mUDPRate = setting->mUDPRate*2; 
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
			messager(msgFATAL, "return lastspeed=%d",lastspeed);
			sendspeed(lastspeed);
		}
		else
		{
			messager(msgFATAL, "return currspeed=%d",currspeed);
			sendspeed(currspeed);
		}
		
		//system("pause");
	}
#if defined(MOHO_X86)
    return NULL;
#elif defined(MOHO_WIN32)
    return 0;
#endif

}



SettingPara detectpara;
bool bstart = false;

#if defined(MOHO_X86)
extern timer_t g_detect_process_timeid ;
void detectthread(union sigval v)
#elif defined(MOHO_WIN32)
VOID CALLBACK detectthread(  //all consumer tasks use only one timer, maybe???
									HWND hwnd,        // handle to window for timer messages
									UINT message,     // WM_TIMER message
									UINT idTimer,     // timer identifier
									DWORD dwTime)     // current system time
#endif
{
#if defined(MOHO_X86)
	timer_t* timeid = (timer_t*)v.sival_ptr;
	if(*timeid != g_detect_process_timeid)
	{
		messager(msgINFO, "Not the taskConsumerProcesser timer ID, timeID = %p", *timeid);
		return;
	}
#elif defined(MOHO_WIN32)
	//messager(msgINFO, "taskConsumerProcesser timer ID");
	if(idTimer != IDT_DETECT_PROCESS_TASK)
	{
		messager(msgINFO, "Not the taskConsumerProcesser timer ID, timeID = %d", idTimer);
		return;
	}
#endif




	if(bstart == false)
	{
		bstart = true;
	}
	else
	{
		return ;
	}
	messager(msgFATAL, "detect thread");	
	Settings_Initialize(&detectpara);
	CMyThread localThread;
	if(0 == localThread.creat(detectspeed,(void*)&detectpara))
	{
		messager(msgFATAL, "CreateThread  failed (%d)", myGetLastError());
	}
	messager(msgFATAL, "detect thread return1");
}
