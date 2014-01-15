//定义网络主线程相关的函数和变量

#ifndef _NETMAIN_H
#define _NETMAIN_H

//#include "buffer.h"

#include "config.h"
#include "LiveChannel.h"
#if defined(MOHO_X86)
#include <sys/socket.h>
#include <deque>
#endif
#include "mylock.h"
#include "mypthread.h"

extern myConfig globalConfig;



#define MULTI_CARRIER_NODE "多线"

#define MAX_RESEND_NUMBER_PER_ROUND_TRIP 5
#define WAIT_FOR_ID_TIMES 15000
//#define RESTART_CHANNEL_CONDITION_2 750//supplier buffer size >
//#define RESTART_CHANNEL_CONDITION_2 20000//supplier buffer size >					//频道提供者缓存阈值：20000 * 1316，约26M，达到此阈值后，提供者退出
#define RESTART_CHANNEL_CONDITION_1 8 //consumer no pdu last seconds				//频道消费者无报文时间阈值：15秒，15秒没有收到任何保温，消费者尝试自动恢复
#define RESTART_CHANNEL_CONDITION 2 //supplier totalpcr behind seconds >			//频道提供者延时阈值：15秒，当发送时钟超前流时钟15秒后，提供者退出


//消费者缓存上限
#ifdef MOHO_X86
#define MAX_ARRIVED_BUF_LENGTH 5000
#define NEED_TO_RESTART_PERCENT 0.9
#else
#define MAX_ARRIVED_BUF_LENGTH 20000
#define NEED_TO_RESTART_PERCENT 0.3
#endif

#define MAX_SEND_NUM 16																//提供者连续发送报文数目，达到此数目后，强制休眠一段时间
#define RESEND_COMMAND_TIME 50														//频道消费者重传次数限制，达到此次数，消费者尝试自动恢复
#define MAX_NO_PDU_TIME 30															//消费者最长无报文时间，达到此时间，消费者需要切换链路
#define BACKUP_LINK_TIME 60		//minute											//备份链路运行时间，以分钟为单位
#define LINK_ERROR 1																//链路错误标记
#define SEND_BUF_SIZE_WARNING_SIZE 4096												//提供者缓冲告警阈值，达到告警值，提供者需要加大发包间隔
#define MAX_PCR_PERIOD_MS 10000														//两个PCR的最大间隔，10000毫秒，即10秒
#define TS_PDU_LENGTH 7																//每个报文包含的TS包数目
#define TS_READ_SIZE 2048															//提供者文件I/O缓冲池大小，以188字节为单位
//#define PUBLICATION_PORT 2632														//资源发布端口
#define TS_CONSUMER_START_TIME_OFFSET 2												//消费者获取资源延时，以分钟为单位
#define TS_FTDS_TIME_OFFSET 3														//向FTDS发包延时，以分钟为单位
#define SUPPLIER_SEND_SLEEP_TIME 2													//提供者最小Sleep间隔，需要多媒体时钟支持
#define SUPPLIER_SEND_SLEEP_TIME_1 64												//提供者最大Sleep间隔
#define FTDS_SLEEP_TIME 8															//向FTDS发包的Sleep间隔
#define READ_BUF_LENGTH 2500														//消费者文件缓冲池大小，以K为单位

#define IDT_NEIGHBOR_PROCESS_TASK 26300												//邻居时钟中断号
#define TIME_PERIOD_OF_NEIGHBOR_TASK 10000											//邻居时钟间隔，以毫秒为单位

#define IDT_CONSUMER_PROCESS_TASK 26301												//消费者始终中断号
#define TIME_PERIOD_OF_CONSUMER_TASK 100											//消费者时钟间隔，以毫秒为单位	
#define CONSUMER_SUPPLIER_TIME_OUT 60												//消费者-提供者超时，以秒为单位，没有启用
#define DELETE_BUFFER_PERIOD 1000													//删除已收到报文间隔，以毫秒为单位

#define IDT_RELAY_PROCESS_TASK 26302												//中转时钟中断号
#define TIME_PERIOD_OF_RELAY_TASK 200												//中转时钟间隔，以毫秒为单位
#define NEXT_TRANS_LAST_INDEX_FOR_DELETE 1

#ifdef MOHO_F20
#define MAX_FILE_SAVED 60															//缓存文件数目，超过这个数目后，从最旧的文件开始删除
#else
#define MAX_FILE_SAVED 240
#endif														//缓存文件数目，超过这个数目后，从最旧的文件开始删除

#define IDT_FTDS_PROCESS_TASK 26302													//FTDS发包时钟中断号
#define FTDS_SEND_PERIOD 475														//FTDS发包间隔，单位为毫秒

#define PDU_ROUND_TRIP 2000															//报文在网络中的生存期，以毫秒为单位

#define RESEND_NUMBER 1																//重传报文标识

//十种报文的标识
#define HOW_ARE_YOU					0x71
#define I_AM_OK						0x72
#define DO_YOU_HAVE					0x73
#define I_HAVE						0x74
#define PLEASE_START_TRANS			0x75
#define START_TRANS					0x76
#define NEXT_TRANS					0x77
#define TRANS_DATA					0x78
#define PLEASE_START_RELAY_TRANS	0x79
#define	START_RELAY_TRANS			0x7a
//P2P与HTTP之间的信令交互
#define GET_CHANNEL					0x7c
#define RELEASE_CHANNEL				0x7d

#define TS_FILE_SIZE_OF_MINUTE 1													//文件持续时间，以分钟为单位

#define MAX_SENT_BUF_SIZE 24000														//提供者发送缓冲的最大之，单位为1316字节

#define MAX_RESEND_PACKET_NUMBER 10000

#define MAX_RESEND_PAIR_NUMBER 5

//邻居状态
typedef enum NeighborNodeStatus
{
	nnsDead,
	nnsNotHealth,
	nnsActivated,
	nnsAlive,
	nnsUnknown
}NeighborNodeStatus;

//消费者状态
typedef enum ConsumerTaskStatus
{
	ctsTrackerQuery,
	ctsNotStarted,
	ctsLinkError,
	ctsQueryFinished,
	ctsStarted,
	ctsWaitForData1, 
	ctsWaitForData2,
	ctsWaitForData3,
	ctsFinished
}ConsumerTaskStatus;

//ftds服务器描述：IP地址和UDP接收端口号
struct ftds
{
	UINT ipAddressFTDS;
	USHORT portFTDS;
	BOOL checked;
	SOCKADDR_IN recvAddr;
};

//资源发布线程加载参数
struct channelPublicationThreadInfo
{
	SOCKET *sockSrv;
	SOCKET *sendSocket;
};

//报文缓冲结构
typedef struct pduBuffer
{
	UINT64 seqNum;																	//报文序列号，唯一
	int length;																		//报文长度
	UINT fileMS;																	//文件长度
	char buf[1500];																	//报文有效载荷
	UINT64 arrivedTick;															//时间戳
	int resendTime;																	//重传次数
}pduBuffer;


//报文统一包头
typedef struct pduHeader
{
	UCHAR Type;																		//报文类型
	USHORT Length;																	//报文长度
	UINT	myUID;																	//报文发送方的统一ID
	UINT64	seqNumber;																//报文序列号
}pduHeader;

//心跳询问报文
typedef struct pduHowAreYou
{
	UINT myUID;																		//询问者UID
}pduHowAreYou;

//心跳应答报文
typedef struct pduIAmOk
{
	USHORT mySupplyPort;															//提供者端口号												
	USHORT myZoneNumber;															//区域号
	USHORT myCapability;															//负载能力
	USHORT myLoad;																	//当前负载
}pduIAmOk;

//资源查询报文
typedef struct pduDoYouHave
{
	UINT consumerUID;																//消费者UID
	UINT supplierUID;
	UINT channelID;																	//频道ID
	time_t s;																		//频道起始时间
	time_t e;																		//频道终止时间
}pduDoYouHave;

//资源查询应答报文
typedef struct pduIHave
{
	UINT64 mostRecentSeqNum;
	UINT YerOrNo;																	//是否拥有该资源
	UINT UID;																		//拥有资源的节点的UID
	UINT tsUID;
	UINT ipAddress;																	//拥有资源的节点的IP地址	
	UINT channelID;																	//资源的频道ID
	UINT bitRate;																	//资源的码流大小，以bps为单位
	UINT pcrPID;																	//PCR报文的ID号
	time_t synStartTime;															//资源校准时间
}pduIHave;


//请求传送报文
typedef struct pduPleaseStartTrans
{
	UINT64 expectedSeqNum;
	UINT channelID;																	//频道ID
	USHORT consumerPort;															//频道接受UDP端口号
	time_t s;																		//频道传递起始时间
}pduPleaseStartTrans;

//请求传送应答报文
typedef struct pduStartTrans
{
	UINT channelID;																	//频道ID
	time_t synStartTransTime;														//频道实际起始传递时间
}pduStartTrans;

typedef struct pduResendPair
{
	UINT64 resendStartSeq;															//重传起始序列号
	UINT64 resendStopSeq;
}pduResendPair;

//请求重传报文
typedef struct pduNextTrans
{
	//UINT64 resendStartSeq;															//重传起始序列号
	//UINT64 resendStopSeq;
	UINT channelID;																	//频道ID
	UINT pairNumber;																	//重传起始字节
	UINT lastIndex;	
	UINT tsUID;														//上次成功接受的索引号

	//UINT resendSeqNum;
	//重传终止序列号
}pduNextTrans;

//流媒体报文
typedef struct pduTransData
{
	UINT tsUID;
	UINT channelID;																	//频道ID
	UINT fileMS;																	//文件持续时间
	//	UINT64 grossBytes;
	//	UCHAR index;
	//	BYTE data[];
}pduTransData;

//请求开始中转传送
typedef struct pduPleaseStartRelayTrans
{
	UINT channelID;																	//频道ID
	UINT realSupplierUID;															//真实提供者UID
	USHORT consumerPort;															//消费者接受数据UDP端口号
	time_t s;																		//其实传递时间	
}pduPleaseStartRelayTrans;


//请求开始中传传送应答报文
typedef struct pduStartRelayTrans
{
	UINT channelID;																	//频道ID
	time_t synStartRelayTransTime;													//实际开始传递时间
}pduStartRelayTrans;


typedef struct pduGetChannel
{
    UINT channelID;
    SOCKADDR_IN recvAddr;
}pduGetChannel;

typedef struct pduReleaseChannel
{
    UINT channelID;
    SOCKADDR_IN recvAddr;
}pduReleaseChannel;

void sayHello(SOCKADDR_IN *dest);
//消费者
typedef struct tsConsumerTask
{
	UINT supplierUID;
	UINT lastSupplierUID;															
	UINT relayRealSupplierUID;
	//BOOL relay;
	CLiveChannel* channel;
	UINT channelID;
	std::string channelName;
	std::string channelPath;
	UINT bitRate;
	UINT pcrPID;
	time_t startTime;	 //time of supplier
	time_t lastPDUTime;  //time of this machine
	std::list<UINT64> waitForReplySeq; 
	UCHAR Status;
	char *recvBuf;
	int bufLength;
	//DWORD tickCount;
	int recvBytes;
	std::list<pduBuffer *> arrivedBufList;
	std::deque<pduBuffer *>arrivedBufDeque;
	UINT lastResentStartSeqNum;
	UINT lastResentStopSeqNum;
	//UINT ipAddressFTDS;
	//USHORT portFTDS;
	UINT resendTime;
	USHORT consumerPort;
	CMyThread consumerThread;
	//	DWORD consumerThreadID;
	//	HANDLE consumerThread;
	CMyThread ftdsThread;
	//	DWORD ftdsThreadID;
	//	HANDLE ftdsThread;
	//	HANDLE ftdsSendEvent;
	int fileIndex;
	std::list<std::string> savedFile;
	BOOL readyForFtds;
	int consumerTimes;
	std::list<ftds> ftdsList;
	int linkNumber;
	UINT64 lastPDUTimeTick;
	UINT64 linkTwoTimeTick;
	BOOL EXITftds;
	BOOL EXITconsumer;
	int linkNumberLast;
	int linkNumberReadFromConfigFile;
	BOOL checked;
	SOCKET consumerSrv;
	UINT recvdFilesNumber;
	UINT tsUID;
	std::deque<pduResendPair> resendDeque;
	UINT64 nextPduSeq;
	CMyLock lockConsumerArrivedDeque;
	CMyThread consumerResendThread;
	std::deque<neighborNode> supplierDeque;
	UINT timesInCtsNotStartedStatus;
	
	SOCKADDR_IN *getSupplierAddr()
	{
		std::string s1;
		std::string s2;
		s2 = inet_ntoa(globalConfig.internetAddr.sin_addr);
		for(UINT i = 0; i<supplierDeque.size(); i++)
		{
			if(supplierDeque[i].UID == supplierUID)
			{
				s1 = inet_ntoa(supplierDeque[i].publicationAddr.sin_addr);
				if(s1.compare(s2) == 0)
				{
					return &(supplierDeque[i].intranetAddr);
				}
				else
				{
					return &(supplierDeque[i].publicationAddr);
				}
			}
		}
		return NULL;
	}

	SOCKADDR_IN *getSupplierDataTransferAddr()
	{
		std::string s1;
		std::string s2;
		s2 = inet_ntoa(globalConfig.internetAddr.sin_addr);
		for(UINT i = 0; i<supplierDeque.size(); i++)
		{
			if(supplierDeque[i].UID == supplierUID)
			{
				s1 = inet_ntoa(supplierDeque[i].publicationAddr.sin_addr);
				if(s1.compare(s2) == 0)
				{
					return &(supplierDeque[i].intranetAddr);
				}
				else
				{
					return &(supplierDeque[i].dataTransferAddr);
				}
			}
		}
		return NULL;
	}


	SOCKADDR_IN *getPossibleSupplierAddr(UINT i)
	{
		std::string s1;
		std::string s2;
		s2 = inet_ntoa(globalConfig.internetAddr.sin_addr);
		s1 = inet_ntoa(supplierDeque[i].publicationAddr.sin_addr);
		if(s1.compare(s2) == 0)
		{
			return &(supplierDeque[i].intranetAddr);
		}
		else
		{
			return &(supplierDeque[i].publicationAddr);
		}
	}
	void sayHello2Supplier()
	{
		std::string s1;
		std::string s2;
		s2 = inet_ntoa(globalConfig.internetAddr.sin_addr);
		if(supplierUID == 0 || Status <= ctsNotStarted)
		{
			for(UINT i = 0; i<supplierDeque.size(); i++)
			{
				s1 = inet_ntoa(supplierDeque[i].publicationAddr.sin_addr);
				if(s1.compare(s2) == 0)
				{
					sayHello(&(supplierDeque[i].intranetAddr));
					mySleep(1000);
				}
				else
				{
					sayHello(&(supplierDeque[i].publicationAddr));
				}
			}
		}
		else
		{
			sayHello(getSupplierAddr());
		}
	}
	UINT checkCtsNotStartedStatus()
	{
		timesInCtsNotStartedStatus ++;
		if(timesInCtsNotStartedStatus > 5)
		{
			timesInCtsNotStartedStatus = 0;
		}
		return timesInCtsNotStartedStatus;
	}
	void setDataTransferAddr(UINT UID, SOCKADDR_IN *addr)
	{
		for(UINT i = 0; i<supplierDeque.size(); i++)
		{
			if(supplierDeque[i].UID == UID)
			{
				supplierDeque[i].dataTransferAddr = *addr;
				break;
			}
		}
	}
}tsConsumerTask;


typedef struct nextTransWithPair
{
	pduNextTrans waitNextTrans;
	std::deque<pduResendPair> resendDeque;
}nextTransWithPair;

typedef struct tsConsumer
{
	UINT UID;
	SOCKADDR_IN ipAddr;
	std::deque<nextTransWithPair> nextTrans;
	CMyLock lockConsumerNextTransDeque;
	UINT seqOffset;
}tsConsumer;

typedef struct tsSupplierTask;

typedef struct tsResendTaskPara
{
	tsConsumer *consumer;
	tsSupplierTask *task;
}tsResendTaskPara;

//提供者
typedef struct tsSupplierTask
{
//	UINT consumerUID;
	UINT channelID;
	UINT64 nextTransSeqNumber;
	CLiveChannel* channel;
//	SOCKADDR_IN consumerIP;
	int fileIndex;
	time_t startTime;
	time_t lastPDUTime;
	//HANDLE nextTransEvent;
	CMyThread thread;
	//HANDLE threadHandle;
	UCHAR lastIndex;
	int sendBytes;
	int lastRecvBytes;
	//std::list<pduBuffer *> sentBufList;
std::deque<pduBuffer *> *sentBufQue;
	UINT64 resendSeqNum;
	CMyLock lockSupplierThread;
	CMyLock lockSentBufQue;
	CMyLock lockDeleteQuitConsumer;
	//volatile LONG seqNum;
	UINT64 seqNum;
	UINT64 resendStartSeq;
	UINT64 resendStopSeq;
	USHORT consumerPort;
	std::deque<pduNextTrans> waitNextTrans;
	//HANDLE resendThreadHandle;
	BOOL EXITsupplier;
	//bool streamSend;
	std::deque<tsConsumer> consumerDeque;
	UINT tsUID;
	UCHAR update2TrackerEvent;
	tsResendTaskPara para;
}tsSupplierTask;



//中转者
typedef struct tsRelayTask
{
	UINT consumerUID;
	UINT supplierUID;
	UINT channelID;
	USHORT consumerPort;
	time_t startTime;
	time_t lastPDUTime;
	std::list<UINT> waitForReplySeq;

	CMyThread thread;
	//HANDLE threadHandle;
}tsRelayTask;


//查询
typedef struct tsQueryTask
{
	UINT consumerUID;
	UINT destUID;
	UINT channelID;
	time_t s;
	time_t e;
}tsQueryTask;





//初始化时钟中断处理函数
int initTimer();

//初始化网络连接，用在Dll初次加载的时候，完成端口创建，内存区初始化
int initNetwork();

//添加一个消费者
int addNewConsumer(tsConsumerTask *cInfo);

//ftds测试
int ftdsTest();

//清理网络模块的资源
int destroyNetwork();

void sayHello2EveryOne();

void checkConsumer();

int startOneSupplier(CLiveChannel *channel, UINT64 firstPacketSeqNum, UINT tsUID, UINT fileIndex);

void sendChannelUpdate(tsSupplierTask *task, int channelEvent);

#endif //_NETMAIN_H
