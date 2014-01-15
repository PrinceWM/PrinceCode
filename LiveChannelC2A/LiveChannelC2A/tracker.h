
#ifndef _TRACKER_H
#define _TRACKER_H

#include <string>
#include <list>
#include <deque>
#include <vector>
#include <set>
#include <map>

//typedef struct myConfig
//{
//	UINT ipAddress;
//	USHORT Port;
//}myConfig;


/*
LIVECDN_CHANNEL_UPDATE
*/
//三种报文的标识
#define LIVECDN_CHANNEL_UPDATE			0x01
#define LIVECDN_CHANNEL_QUERY			0x02
#define LIVECDN_REPLY_CHANNEL_QUERY		0x03
#define LIVECDN_QUERY_CARRIR_NOT_SPPORT 0x04
#define LIVECDN_NOTIFY_CHANNEL_QUERY	0x05

typedef enum ERROR_TYPE
{
	errCarrirNotSupport,
	errRegionNotSupport,
	errCarrirAndRegionNotSupport,

};

#define CHANNEL_UPDATE_PERIOD 5//second

#define MAX_SUPPLIER_NUM 5

typedef enum LIVE_CDN_NODETYPE
{
	ntSUPER_WITH_TS,
	ntSUPER,
	ntNORMAL
}LIVE_CDN_NODETYPE;

typedef enum UPDATE_EVENT_TYPE
{
	evtAlive,
	evtQuit
}UPDATE_EVENT_TYPE;

typedef enum PLATFORM_TYPE
{
	OS_WINDOWS,
	OS_LINUX,
	OS_ANDROID,
}PLATFORM_TYPE;


typedef struct liveCDNNode
{
	UINT UID;
	UCHAR type;
	USHORT capability;
	USHORT currLoad;
	SOCKADDR_IN internetAddr;
	SOCKADDR_IN intranetAddr;
	char carrier[16];
	char province[32];
	char city[32];
	PLATFORM_TYPE platform;

	liveCDNNode()
	{
		memset(carrier,0,sizeof(carrier));
		memset(province,0,sizeof(province));
		memset(city,0,sizeof(city));

	}

	

	bool operator<(const liveCDNNode& st) const   
	{
		return load_factor() < st.load_factor();
	}

	bool operator>(const liveCDNNode& st) const   
	{
		return load_factor() > st.load_factor();
	}


	bool operator==(const liveCDNNode& st)const   
	{
		return this->UID == st.UID;
	}

	bool operator!=(const liveCDNNode& st)const   
	{
		return this->UID != st.UID;
	}

	int load_factor()const
	{
		if(ntSUPER_WITH_TS ==  type 
			|| ntSUPER == type)
		{
			return 100 + (capability - currLoad );
		}
		else
			return (capability - currLoad)*100/capability;
	}

	bool full()
	{
		return capability == currLoad;
	}




}liveCDNNode;


class NodeComparer{     
public:
	bool operator()(const liveCDNNode * node1,const liveCDNNode * node2)	   
	{
		return node1->load_factor() > node2->load_factor();  
	}
};

typedef std::multiset<const liveCDNNode *,NodeComparer>  CDNNodeVector;

typedef struct liveCDNChannel
{
	UINT channelID;
	std::string channelName;
	CDNNodeVector nodeSet[6][100];//main carrir & larger city
	CDNNodeVector superNodeSet[6];	
	
}liveCDNChannel;

typedef enum LIVE_CDN_NODE_STATUS
{
	stNOCHannel,
	stQuit,
	stNORMAL,
}LIVE_CDN_NODE_STATUS;


typedef struct liveCDNNodeDynamic
{
	liveCDNNodeDynamic():nodeInfo(0){}

	liveCDNNode *nodeInfo;
	std::deque<UINT> channelDeque;															
	int nodeStatus;
	DWORD update_time;		//micro sec

}liveCDNNodeDynamic;

////报文统一包头
//typedef struct pduHeader
//{
//	UCHAR Type;																		//报文类型
//	USHORT Length;																	//报文长度
//	UINT	myUID;																	//报文发送方的统一ID
//	UINT64	seqNumber;																//报文序列号
//
//
//}pduHeader;

typedef struct pduUpdateSingleChannel
{
	INT channelID;
	UCHAR updateEvent;
}pduUpdateSingleChannel;

typedef struct pduUpdate
{
	liveCDNNode nodeInfo;
	UINT channelNumber;

	//pduUpdateSingleChannel1
	//pduUpdateSingleChannel2
	//pduUpdateSingleChannel3
	//......
}pduUpdate;



typedef struct pduQuery
{
	liveCDNNode nodeInfo;
	UINT channelID;
	
}pduQuery;


typedef struct pduReplyQuerySingleSupplier
{
	UINT supplierUID;
	SOCKADDR_IN internetAddr;
	SOCKADDR_IN intranetAddr;

}pduReplyQuerySingleSupplier;


typedef struct pduReplyQuery
{
	liveCDNNode queryNode;
	UINT channelID;
	UCHAR supplierNumber;
	//pduReplyQuerySingleSupplier1
	//pduReplyQuerySingleSupplier2
	//pduReplyQuerySingleSupplier3

	
}pduReplyQuery;

typedef struct pduNotifyQuery
{
	UINT channleID;
	liveCDNNode queryNode;
}pduNotifyQuery;



int initNetwork();

#endif //_TRACKER_H
