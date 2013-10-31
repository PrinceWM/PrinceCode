#include "CMySession.h"
#include "MyThread.h"



CMySession::CMySession()
{
	int index = 0;
	for(index = 0;index<MAXSESSION;index++)
	{
		sessioninfostore[index].udpsessionuse = false;
		sessioninfostore[index].length = 0;
		sessioninfostore[index].transfertime.set(0,0);
		sessioninfostore[index].currenttime.set(0,0);
		sessioninfostore[index].applytime.set(0,0);
		memset(&sessioninfostore[index].clientadd,0,sizeof(sessioninfostore[index].clientadd));
		sessioninfostore[index].clientaddlen = 0;
		sessioninfostore[index].tmptime = (((AMOUNT/100+1))*(1e6));
		sessioninfostore[index].lastlen = 0;
	}
}

CMySession::~CMySession(void)
{

}

int CMySession::setsession(int sessionid,int randid)
{
	if(sessionid >= MAXSESSION || sessioninfostore[sessionid].udpsessionuse == true)
	{
		printf("set session error %d\n",sessionid);
		return -1;
	}
	sessioninfostore[sessionid].udpsessionuse = true;
	sessioninfostore[sessionid].randid = randid;
	sessioninfostore[sessionid].applytime.setnow();
	return 0;
}

#define BORDER_TIME 10


int CMySession::updatesession( int mAmount,CMyThread* taskThread)
{
	int itemp;
	long time = 0;

	long msfeed = 0;
	double speed = 0.0;
	if(taskThread == NULL)
	{
		return -1;
	}
	datagram* serverdgmbuff = (datagram*)taskThread->recvbuff;
	int ret = 0;
	struct timeval currentTime;
	gettimeofday(&currentTime,NULL);
	for(itemp = 0;itemp<MAXSESSION;itemp++)
	{
		if(sessioninfostore[itemp].udpsessionuse ==  true)
		{
			sessioninfostore[itemp].currenttime.set(currentTime.tv_sec,currentTime.tv_usec);
			if(sessioninfostore[itemp].length > 0)
			{//check start transfer session time ,data				

				time = sessioninfostore[itemp].currenttime.subUsec(sessioninfostore[itemp].transfertime);				
				if(time >= (long)sessioninfostore[itemp].tmptime)
				{
					sessioninfostore[itemp].tmptime += 1e6;//1 sec 				
					sessioninfostore[itemp].tmprecvnumpersec[sessioninfostore[itemp].tmpseccount] = (int)((sessioninfostore[itemp].length-sessioninfostore[itemp].lastlen)/taskThread->recvbufflen);
					sessioninfostore[itemp].lastlen = sessioninfostore[itemp].length;
					printf("time %d tmptime %d pack recv %d\n",time,sessioninfostore[itemp].tmptime,sessioninfostore[itemp].tmprecvnumpersec[sessioninfostore[itemp].tmpseccount]/*(sessioninfostore[itemp].length-lastlen)/serverbufflen*/);
					sessioninfostore[itemp].tmpseccount++;
				}

				if(time >= ((mAmount/100)+BORDER_TIME)*(1e6))//ns
				{
					if(sessioninfostore[itemp].tmpseccount<10)
					{
						printf("******************\n");
						printf("********%d**********\n",sessioninfostore[itemp].tmpseccount);
						printf("******************\n");
					}
					printf("send speed id\n");
					msfeed = sessioninfostore[itemp].transfertime.delta_usec();
					printf("end time %d: %d %d \n",itemp,sessioninfostore[itemp].transfertime.getSecs(),sessioninfostore[itemp].transfertime.getUsecs());
					//speed = (((double)(sessioninfostore[itemp].length*8)/(1024*1024))/((double)msfeed/(1000*1000)));//Mbit/sec
					speed = ((double)(sessioninfostore[itemp].length*8)/(1024*1024))/(mAmount/100);
					printf("speed = %f sessioninfostore[i].length=%d msfeed=%ld recvpack=%d\n",speed,sessioninfostore[itemp].length,msfeed,sessioninfostore[itemp].length/taskThread->recvbufflen);

					serverdgmbuff->id      = htonl( /*datagramID*/-2 ); 
					serverdgmbuff->send_sec  = htonl( currentTime.tv_sec ); 
					serverdgmbuff->send_sec = htonl( currentTime.tv_usec ); 				
					serverdgmbuff->speed = htonl((int)(speed*1000));


					serverdgmbuff->seccount = htonl( sessioninfostore[itemp].tmpseccount );
					int i;
					for(i = 0;i<sessioninfostore[itemp].tmpseccount;i++)
					{
						serverdgmbuff->recvnumpersec[i] = htonl(sessioninfostore[itemp].tmprecvnumpersec[i]);
					}
					memset(sessioninfostore[itemp].tmprecvnumpersec,0,sizeof(sessioninfostore[itemp].tmprecvnumpersec));
					sessioninfostore[itemp].tmpseccount = 0;
					ret = sendto( taskThread->datasock, taskThread->recvbuff,sizeof(datagram)/*serverbufflen*/, 0,(struct sockaddr*) &sessioninfostore[itemp].clientadd, sessioninfostore[itemp].clientaddlen);
					if(ret > 0)
					{
						clearsession(itemp);	
						sessioninfostore[itemp].tmptime = (((mAmount/100+1))*(1e6));
						sessioninfostore[itemp].lastlen = 0;
						taskThread->getbelongthreadpool()->m_ContrSource.setfreesource(taskThread->dataport,itemp);
						//free source need lock
						return 1;
					}
					else
					{
						printf("final send to error %d\n",WSAGetLastError());
						return -1;
					}
				}
			}
			else if(sessioninfostore[itemp].length == 0)
			{//check not start transfer session time ,if time out close this session	
				time = sessioninfostore[itemp].currenttime.subUsec(sessioninfostore[itemp].applytime);				
				if(time >= ((mAmount/100))*(1e6))
				{//this session not work in amount time len so clear it
					printf("clear not use session\n");
					clearsession(itemp);
					taskThread->getbelongthreadpool()->m_ContrSource.setfreesource(taskThread->dataport,itemp);
					//free source need lock
				}				
			}
		}
	}
	return 0;

}

int CMySession::clearsession(int index )
{
	if(index >= MAXSESSION)
	{
		printf("clear index is too big \n");
		return -1;
	}

	sessioninfostore[index].udpsessionuse = false;
	sessioninfostore[index].length = 0;
	sessioninfostore[index].transfertime.set(0,0);
	sessioninfostore[index].currenttime.set(0,0);
	sessioninfostore[index].applytime.set(0,0);
	memset(&sessioninfostore[index].clientadd,0,sizeof(sessioninfostore[index].clientadd));
	sessioninfostore[index].clientaddlen = 0;
	return 0;

}



