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
	int ret = 0;
	mutext.Lock();
	if(source[port][sessionid].isuse == true)
	{
		source[port][sessionid].isuse = false;
		source[port][sessionid].sessionid = 0;
		source[port][sessionid].port = 0;
	}
	else
	{
		ret = -1;
	}
	mutext.Unlock();
	return ret;
}