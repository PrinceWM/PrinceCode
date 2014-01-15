
#include "client.h"

int sInterupted = 0;
void Settings_Initialize( thread_Settings *main ) {
	// Everything defaults to zero or NULL with
	// this memset. Only need to set non-zero values
	// below.
	memset( main, 0, sizeof(thread_Settings) );
	memset( main->mHost, 0, sizeof(main->mHost) );
	main->mSock = INVALID_SOCKET;
	main->mUDPRate = kDefault_UDPRate;
	main->isudp = 1;
	main->once = 0;
	main->mBufLen       = /*32 * 1024*/512;      // -l,  8 Kbyte
	main->mPort         = 5001;          // -p,  ttcp port
	main->mAmount       = 300;          // -t,  5 seconds
	main->mTTL          = 1;             // -T,  link-local TTL
	main->checkid = 0;
} // end Settings



int ParseArag(int argc, char* argv[], thread_Settings* setting)
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

int main(int argc, char **argv)
{
	thread_Settings setting;
	int lastspeed = 0;
	int currspeed = 0;
	int ret = -1;
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
	if(ret == 0)
	{//client
		
		if(strlen(setting.mHost) == 0)
		{
			memcpy(setting.mHost,"112.124.0.75",strlen("112.124.0.75"));
			//memcpy(setting.mHost,"113.140.73.3",strlen("113.140.73.3"));
			//memcpy(setting.mHost,"125.76.233.68",strlen("125.76.233.68"));
		}
		
		printf("set client mode\n");
REDETECT:
		client* clientptr = new client(&setting);
		ret = clientptr->Connect();
		//printf("client connect \n");
		if(ret != 0)
		{
			printf("client connect error \n");
		}
		else
		{		
			currspeed = clientptr->run();			
			//printf("client run ret in main = %d\n",ret);
			if (currspeed < 0)
			{
				printf("not runable \n");
			}
		}
		

		if(setting.once == 0 && setting.isudp && currspeed>lastspeed)
		{
			lastspeed = currspeed;
			setting.mUDPRate = setting.mUDPRate*2; 
			setting.checkid++;
			clientptr->storespeed(currspeed);
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
		//system("pause");
	}

#ifdef WIN32_BANDTEST
	rc = WSACleanup ( );	
	if (rc == SOCKET_ERROR)
	{
		printf("cleanup sock error\n");
		return 0;
	}
#endif
	return 0;
}
