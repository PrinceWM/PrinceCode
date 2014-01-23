
#include "sendevent.h"

sendevent::sendevent()
{	
	memset(&pack,0,sizeof(pack));
	sock = socket( AF_INET , SOCK_DGRAM , IPPROTO_UDP ) ;
	if(sock < 0)
	{
		printf("socket creat fail \n");
	}
};

sendevent::~sendevent()
{
	if(sock >= 0)
	{
		closesocket(sock);
	}
};
int sendevent::sendpacket()
{
	int ret;
	ret = sendto(sock,(char*)(&pack),sizeof(pack),0,(sockaddr *)(&sendaddr),sizeof(sendaddr));
	if(ret < 0)
	{
		printf("sendto error =%d\n",GetLastError());
	}
	return ret;
};

int sendevent::recvpacket()
{
	return 0;
};

int sendevent::setsendadd(short port ,char* ipadd)
{
	sendaddr.sin_addr.S_un.S_addr = inet_addr(ipadd);
	sendaddr.sin_family = AF_INET ;
	sendaddr.sin_port = htons(port) ;	
	return 0;
};
int sendevent::setpack(SENDPACK tpack)
{
	pack.packtype = tpack.packtype ;
	pack.uid = tpack.uid;
	memcpy(pack.payload,tpack.payload,tpack.playloadlen);
	pack.playloadlen = tpack.playloadlen;
	return 0;
}

int sendevent::bindport(short port)
{
	int ret ;
	struct sockaddr_in localadd;
	memset(&localadd,0,sizeof(localadd));
	localadd.sin_family = AF_INET;
	localadd.sin_port = htons(port);
	localadd.sin_addr.s_addr = INADDR_ANY;
	ret = bind( sock, (sockaddr*) &localadd, sizeof(localadd) );
	if(ret < 0)
	{
		printf("bind error =%d\n",GetLastError());
	}
	return 0;
}

int sendevent::checkevent(struct timeval tv)
{
	int ret;
	fd_set fd_read;
	FD_ZERO(&fd_read);
	FD_SET(sock, &fd_read);

	ret = select(sock + 1, (fd_set*)&fd_read,0, (fd_set*)0, &tv);
	if(ret > 0)
	{
		if(FD_ISSET(sock, &fd_read))
		{
			
		}
		else
		{
			ret = 0;	
		}
	}

	return ret;
}