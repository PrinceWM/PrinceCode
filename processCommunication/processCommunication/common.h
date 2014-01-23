#pragma once
#include <WinSock.h>
#include <Windows.h>
#include <iostream>
#include <stdlib.h>
#define DEAL_PORT 10001
#define RELAY_PORT 10002
typedef enum
{
	PAYLOAD_PACK,
	DOING_PACK,
	RESEND_PACK,
	REPLAY_PACK,
}PACKTYPE;

#define MAX_PAYLOAD 1500
typedef struct _SENDPACK
{
	int uid;
	unsigned int packtype;
	int playloadlen;
	char payload[MAX_PAYLOAD];	
}SENDPACK;