#include "common.h"
#include "eventrelay.h"
#include <time.h>
int eventrelay::recvpacket()
{
	int ret;
	SENDPACK spack;
	RELAYPACK rpack;
	struct sockaddr_in sockadd;
	int len = sizeof(sockadd);
	list<RELAYPACK>::iterator it;
	ret = recvfrom(getsock(),(char*)&spack,sizeof(spack),0,(sockaddr *)&sockadd,&len);
	if(ret < 0)
	{
		printf("recv error %d\n",GetLastError());
		return ret;
	}
	switch(spack.packtype)
	{
	case PAYLOAD_PACK:
		//printf("relay payload packet\n");
		//insert list
		rpack.tm = time(NULL);
		rpack.uid = spack.uid;
		rpack.packtype = spack.packtype;

		rpack.packsendaddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
		rpack.packsendaddr.sin_family = AF_INET ;
		rpack.packsendaddr.sin_port = htons(DEAL_PORT) ;
		setsendadd(rpack.packsendaddr);
		setpack(spack);
		sendpacket();
		
		if(rlist.size() == 0)
		{//insert at front
			rlist.push_front(rpack);
		}
		else
		{
			for (it = rlist.begin(); it != rlist.end(); it++)
			{
				if((it->tm) > rpack.tm)
				{//insert at middle position
					it = rlist.insert((it),rpack);
					break;
				}
				else if((it->tm) == rpack.tm)
				{
					printf("error tm\n");
					return 0;
				}
			}

			if(it == rlist.end())
			{//insert at last
				rlist.push_back(rpack);
			}
		}
#if 0
		printf("insert\n");
		for(it = rlist.begin(); it != rlist.end(); it++)
		{
			printf("%lld %d\n",it->tm,it->uid);
		}
#endif
		break;
	case REPLAY_PACK:
		//clear list
		rpack.tm = time(NULL);
		rpack.uid = spack.uid;
		rpack.packtype = spack.packtype;
		memcpy(&rpack.packsendaddr,&sockadd,len);
		for (it = rlist.begin(); it != rlist.end(); it++)
		{
			if((it->uid) == rpack.uid)
			{
				rlist.erase(it);
				break;
			}
		}
		
#if 0
		printf("erase\n");
		for(it = rlist.begin(); it != rlist.end(); it++)
		{
			printf("%lld %d\n",it->tm,it->uid);
		}
#endif
		//printf("relay replay packet\n");
		break;		
	default:
		ret = -1;
		break;
	}
	return ret;
}

int eventrelay::checktimout()
{
	list<RELAYPACK>::iterator it;
	list<RELAYPACK>::iterator tmpit;
	list<RELAYPACK> tmplist;
	SENDPACK spack;
	time_t tmptm = time(NULL);
	for (it = rlist.begin(); it != rlist.end(); it++)
	{
		if(tmptm - (it->tm) <= 10)
		{			
			break;
		}
	}

// 	if(it == rlist.end())
// 	{
// 		return 0;
// 	}
	
	tmplist.assign(rlist.begin(),it);
	if(tmplist.size() <= 0)
	{
		return 0;
	}
	rlist.erase(rlist.begin(),it);
	//tmplist.assign(rlist.begin(),rlist.end());
	for (tmpit = tmplist.begin() ; tmpit != tmplist.end(); tmpit++)
	{
		setsendadd(tmpit->packsendaddr);
		spack.uid = tmpit->uid;
		spack.packtype = tmpit->packtype;
		spack.playloadlen = MAX_PAYLOAD;
		memset(spack.payload,1,spack.playloadlen);
		tmpit->tm = time(NULL);
		setpack(spack);
		sendpacket();
	}
	rlist.splice(rlist.end(),tmplist);
	return 0;
}
