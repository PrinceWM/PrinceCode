#include "common.h"
#include "dealevent.h"

int dealevent::recvpacket()
{
	int ret;
	SENDPACK pack;
	struct sockaddr_in sockadd;
	int len = sizeof(sockadd);
	ret = recvfrom(getsock(),(char*)&pack,sizeof(pack),0,(sockaddr *)&sockadd,&len);
	if(ret < 0)
	{
		printf("recv error %d\n",GetLastError());
		return ret;
	}
	switch(pack.packtype)
	{
	case PAYLOAD_PACK:
		pack.packtype = REPLAY_PACK;
// 		pack.playloadlen = MAX_PAYLOAD;
// 		memset(pack.payload,1,pack.playloadlen);
		setpack(pack);
		setsendadd(sockadd);
		//sendpacket();
		//printf("deal payload packet\n");
		break;
// 	case DOING_PACK:
// 		printf("doing packet\n");
// 		break;
// 	case RESEND_PACK:
// 		printf("resend packet\n");
// 		break;
// 	case REPLAY_PACK:
// 		printf("replay packet\n");
// 		break;		
	default:
		ret = -1;
		break;
	}
	return ret;
}

