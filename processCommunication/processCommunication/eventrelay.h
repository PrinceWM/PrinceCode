#pragma once
#include "sendevent.h"
#include <list>
using namespace std;

#define MAX_TIMEOUT 10//10 sec
typedef struct _RELAYPACK
{
	int uid;
	unsigned int packtype;
	time_t tm;
	SOCKADDR_IN packsendaddr;
}RELAYPACK;

class eventrelay:public sendevent
{
public:
	int recvpacket();
	int checktimout();
private:
	list<RELAYPACK>	rlist;
};

