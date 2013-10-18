#include "server.h"
#include "client.h"

int sInterupted = 0;
void Settings_Initialize( thread_Settings *main ) {
	// Everything defaults to zero or NULL with
	// this memset. Only need to set non-zero values
	// below.
	memset( main, 0, sizeof(thread_Settings) );
	main->mSock = INVALID_SOCKET;
	main->mUDPRate = kDefault_UDPRate;
	//main->mReportMode = kReport_Default;
	// option, defaults
	//main->flags         = FLAG_MODETIME | FLAG_STDOUT; // Default time and stdout
	//main->mUDPRate      = 0;           // -b,  ie. TCP mode
	//main->mHost         = NULL;        // -c,  none, required for client
	//main->mMode         = kTest_Normal;  // -d,  mMode == kTest_DualTest
	//main->mFormat       = 'a';           // -f,  adaptive bits
	// skip help                         // -h,
	//main->mBufLenSet  = false;         // -l,	
	main->isudp = 1;
	main->mHost = (char*)malloc(strlen("112.124.0.75"));	
	memcpy(main->mHost,"112.124.0.75",strlen("112.124.0.75"));
	main->mBufLen       = /*32 * 1024*/512;      // -l,  8 Kbyte
	//main->mInterval     = 0;           // -i,  ie. no periodic bw reports
	//main->mPrintMSS   = false;         // -m,  don't print MSS
	// mAmount is time also              // -n,  N/A
	//main->mOutputFileName = NULL;      // -o,  filename
	main->mPort         = 5001;          // -p,  ttcp port
	// mMode    = kTest_Normal;          // -r,  mMode == kTest_TradeOff
	//main->mThreadMode   = kMode_Unknown; // -s,  or -c, none
	main->mAmount       = 500;          // -t,  5 seconds
	// mUDPRate > 0 means UDP            // -u,  N/A, see kDefault_UDPRate
	// skip version                      // -v,
	//main->mTCPWin       = 0;           // -w,  ie. don't set window

	// more esoteric options
	//main->mLocalhost    = NULL;        // -B,  none
	//main->mCompat     = false;         // -C,  run in Compatibility mode
	//main->mDaemon     = false;         // -D,  run as a daemon
	//main->mFileInput  = false;         // -F,
	//main->mFileName     = NULL;        // -F,  filename 
	//main->mStdin      = false;         // -I,  default not stdin
	//main->mListenPort   = 0;           // -L,  listen port
	//main->mMSS          = 0;           // -M,  ie. don't set MSS
	//main->mNodelay    = false;         // -N,  don't set nodelay
	//main->mThreads      = 0;           // -P,
	//main->mRemoveService = false;      // -R,
	//main->mTOS          = 0;           // -S,  ie. don't set type of service
	main->mTTL          = 1;             // -T,  link-local TTL
	//main->mDomain     = kMode_IPv4;    // -V,
	//main->mSuggestWin = false;         // -W,  Suggest the window size.

} // end Settings



int ParseArag(int argc, char* argv[], thread_Settings* setting)
{
	int ret = 0;
	int i = 0;
	int arg_increment = 1;
	for (i = 1; i < argc; i += arg_increment)
	{
		if (strcmp(argv[i], "-c") == 0 )
		{//client mode
			int len = strlen(argv[i+1])+1;
			//printf("len =%d \n",len);
			setting->mHost = (char*)malloc(len);
			memcpy(setting->mHost ,argv[i+1], len);
			arg_increment++;
			ret = 0;			
		}
		else if(strcmp(argv[i], "-s") == 0 )
		{//server mode
			ret = 1;
		}
		else if(strcmp(argv[i], "-u") == 0 )
		{
			setting->isudp = 1;
		}
		else if(strcmp(argv[i], "-b") == 0 )
		{
			setting->isudp = 1;//-b just use for udp 
			setting->mUDPRate = atoi(argv[i+1]);
			arg_increment++;
		}
		else if(strcmp(argv[i], "-l") == 0 )
		{
			setting->mBufLen = atoi(argv[i+1]);
			arg_increment++;
		}
		else if(strcmp(argv[i], "-t") == 0 )
		{
			setting->mAmount = atoi(argv[i+1]);
			arg_increment++;
		}
		else if(strcmp(argv[i], "-p") == 0 )
		{
			setting->mPort = atoi(argv[i+1]);
			arg_increment++;
		}
		else if(strcmp(argv[i], "-w") == 0 )
		{
			setting->mTCPWin = atoi(argv[i+1]);
			arg_increment++;
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
		delete clientptr;

		if(setting.isudp&&currspeed>lastspeed)
		{
			lastspeed = currspeed;
			setting.mUDPRate = setting.mUDPRate*2; 
			Sleep(1000*3);
			goto REDETECT;
		}
		printf("detect speed is %d kbit /sec",lastspeed);
		system("pause");
	}
	else if(ret == 1)
	{//server
		printf("set server mode\n");
		server* serverptr = new server(&setting);
		serverptr->creat();
		serverptr->recvdata();
		delete serverptr;
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
		return 0;
	}
#endif
	//while(1);
	return 0;
}
