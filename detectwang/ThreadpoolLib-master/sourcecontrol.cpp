#include "sourcecontrol.h"

CSourcControl::CSourcControl(int startport)
{
	int i = 0;
	int j = 0;
	for(i = 0;i<MAXPORT;i++)
		for(j = 0;j<MAXSESSION;j++)
	{
		source[i][j].isuse = false;
		source[i][j].port = startport+i;
		source[i][j].sessionid = j;
	}	
}

CSourcControl::~CSourcControl()
{

}

int CSourcControl::getfreesource(int *port,int *sessionid)
{
	int i = 0;
	int j = 0;
	int tmpi,tmpj;
	int ret = -1;
	mutext.Lock();
	for(i = 0;i<MAXPORT;i++)
		for(j = 0;j<MAXSESSION;j++)
	{
		if(source[i][j].isuse == false)
		{//get a not use session and port
			ret = 0;
			tmpi = i;
			tmpj = j;
			for(j = 0;j<MAXSESSION;j++)
			{//check this port thread have work session or not 
				if(source[i][j].isuse == true)
				{
					ret = 1;
					break;
				}
			}

			source[tmpi][tmpj].isuse = true;
			*port = source[tmpi][tmpj].port;
			*sessionid = source[tmpi][tmpj].sessionid;

			mutext.Unlock();
			return ret;
		}
	}
	mutext.Unlock();
	return -1;
}

int CSourcControl::setfreesource(int port,int sessionid)
{
	//printf("setfreesource port=%d",port);
	int ret = -1;
	int i = 0;
	int j = 0;

	mutext.Lock();
	for(i = 0;i<MAXPORT;i++)
	{
		if(source[i][0].port == port)
		{
			source[i][sessionid].isuse = false;
			ret = 0;
			break;
		}
	}
	mutext.Unlock();
	return ret;
}