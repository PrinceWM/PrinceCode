//�����������߳���صĺ����ͱ���

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



#define MULTI_CARRIER_NODE "����"

#define MAX_RESEND_NUMBER_PER_ROUND_TRIP 5
#define WAIT_FOR_ID_TIMES 15000
//#define RESTART_CHANNEL_CONDITION_2 750//supplier buffer size >
//#define RESTART_CHANNEL_CONDITION_2 20000//supplier buffer size >					//Ƶ���ṩ�߻�����ֵ��20000 * 1316��Լ26M���ﵽ����ֵ���ṩ���˳�
#define RESTART_CHANNEL_CONDITION_1 8 //consumer no pdu last seconds				//Ƶ���������ޱ���ʱ����ֵ��15�룬15��û���յ��κα��£������߳����Զ��ָ�
#define RESTART_CHANNEL_CONDITION 2 //supplier totalpcr behind seconds >			//Ƶ���ṩ����ʱ��ֵ��15�룬������ʱ�ӳ�ǰ��ʱ��15����ṩ���˳�


//�����߻�������
#ifdef MOHO_X86
#define MAX_ARRIVED_BUF_LENGTH 5000
#define NEED_TO_RESTART_PERCENT 0.9
#else
#define MAX_ARRIVED_BUF_LENGTH 20000
#define NEED_TO_RESTART_PERCENT 0.3
#endif

#define MAX_SEND_NUM 16																//�ṩ���������ͱ�����Ŀ���ﵽ����Ŀ��ǿ������һ��ʱ��
#define RESEND_COMMAND_TIME 50														//Ƶ���������ش��������ƣ��ﵽ�˴����������߳����Զ��ָ�
#define MAX_NO_PDU_TIME 30															//��������ޱ���ʱ�䣬�ﵽ��ʱ�䣬��������Ҫ�л���·
#define BACKUP_LINK_TIME 60		//minute											//������·����ʱ�䣬�Է���Ϊ��λ
#define LINK_ERROR 1																//��·������
#define SEND_BUF_SIZE_WARNING_SIZE 4096												//�ṩ�߻���澯��ֵ���ﵽ�澯ֵ���ṩ����Ҫ�Ӵ󷢰����
#define MAX_PCR_PERIOD_MS 10000														//����PCR���������10000���룬��10��
#define TS_PDU_LENGTH 7																//ÿ�����İ�����TS����Ŀ
#define TS_READ_SIZE 2048															//�ṩ���ļ�I/O����ش�С����188�ֽ�Ϊ��λ
//#define PUBLICATION_PORT 2632														//��Դ�����˿�
#define TS_CONSUMER_START_TIME_OFFSET 2												//�����߻�ȡ��Դ��ʱ���Է���Ϊ��λ
#define TS_FTDS_TIME_OFFSET 3														//��FTDS������ʱ���Է���Ϊ��λ
#define SUPPLIER_SEND_SLEEP_TIME 2													//�ṩ����СSleep�������Ҫ��ý��ʱ��֧��
#define SUPPLIER_SEND_SLEEP_TIME_1 64												//�ṩ�����Sleep���
#define FTDS_SLEEP_TIME 8															//��FTDS������Sleep���
#define READ_BUF_LENGTH 2500														//�������ļ�����ش�С����KΪ��λ

#define IDT_NEIGHBOR_PROCESS_TASK 26300												//�ھ�ʱ���жϺ�
#define TIME_PERIOD_OF_NEIGHBOR_TASK 10000											//�ھ�ʱ�Ӽ�����Ժ���Ϊ��λ

#define IDT_CONSUMER_PROCESS_TASK 26301												//������ʼ���жϺ�
#define TIME_PERIOD_OF_CONSUMER_TASK 100											//������ʱ�Ӽ�����Ժ���Ϊ��λ	
#define CONSUMER_SUPPLIER_TIME_OUT 60												//������-�ṩ�߳�ʱ������Ϊ��λ��û������
#define DELETE_BUFFER_PERIOD 1000													//ɾ�����յ����ļ�����Ժ���Ϊ��λ

#define IDT_RELAY_PROCESS_TASK 26302												//��תʱ���жϺ�
#define TIME_PERIOD_OF_RELAY_TASK 200												//��תʱ�Ӽ�����Ժ���Ϊ��λ
#define NEXT_TRANS_LAST_INDEX_FOR_DELETE 1

#ifdef MOHO_F20
#define MAX_FILE_SAVED 60															//�����ļ���Ŀ�����������Ŀ�󣬴���ɵ��ļ���ʼɾ��
#else
#define MAX_FILE_SAVED 240
#endif														//�����ļ���Ŀ�����������Ŀ�󣬴���ɵ��ļ���ʼɾ��

#define IDT_FTDS_PROCESS_TASK 26302													//FTDS����ʱ���жϺ�
#define FTDS_SEND_PERIOD 475														//FTDS�����������λΪ����

#define PDU_ROUND_TRIP 2000															//�����������е������ڣ��Ժ���Ϊ��λ

#define RESEND_NUMBER 1																//�ش����ı�ʶ

//ʮ�ֱ��ĵı�ʶ
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
//P2P��HTTP֮��������
#define GET_CHANNEL					0x7c
#define RELEASE_CHANNEL				0x7d

#define TS_FILE_SIZE_OF_MINUTE 1													//�ļ�����ʱ�䣬�Է���Ϊ��λ

#define MAX_SENT_BUF_SIZE 24000														//�ṩ�߷��ͻ�������֮����λΪ1316�ֽ�

#define MAX_RESEND_PACKET_NUMBER 10000

#define MAX_RESEND_PAIR_NUMBER 5

//�ھ�״̬
typedef enum NeighborNodeStatus
{
	nnsDead,
	nnsNotHealth,
	nnsActivated,
	nnsAlive,
	nnsUnknown
}NeighborNodeStatus;

//������״̬
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

//ftds������������IP��ַ��UDP���ն˿ں�
struct ftds
{
	UINT ipAddressFTDS;
	USHORT portFTDS;
	BOOL checked;
	SOCKADDR_IN recvAddr;
};

//��Դ�����̼߳��ز���
struct channelPublicationThreadInfo
{
	SOCKET *sockSrv;
	SOCKET *sendSocket;
};

//���Ļ���ṹ
typedef struct pduBuffer
{
	UINT64 seqNum;																	//�������кţ�Ψһ
	int length;																		//���ĳ���
	UINT fileMS;																	//�ļ�����
	char buf[1500];																	//������Ч�غ�
	UINT64 arrivedTick;															//ʱ���
	int resendTime;																	//�ش�����
}pduBuffer;


//����ͳһ��ͷ
typedef struct pduHeader
{
	UCHAR Type;																		//��������
	USHORT Length;																	//���ĳ���
	UINT	myUID;																	//���ķ��ͷ���ͳһID
	UINT64	seqNumber;																//�������к�
}pduHeader;

//����ѯ�ʱ���
typedef struct pduHowAreYou
{
	UINT myUID;																		//ѯ����UID
}pduHowAreYou;

//����Ӧ����
typedef struct pduIAmOk
{
	USHORT mySupplyPort;															//�ṩ�߶˿ں�												
	USHORT myZoneNumber;															//�����
	USHORT myCapability;															//��������
	USHORT myLoad;																	//��ǰ����
}pduIAmOk;

//��Դ��ѯ����
typedef struct pduDoYouHave
{
	UINT consumerUID;																//������UID
	UINT supplierUID;
	UINT channelID;																	//Ƶ��ID
	time_t s;																		//Ƶ����ʼʱ��
	time_t e;																		//Ƶ����ֹʱ��
}pduDoYouHave;

//��Դ��ѯӦ����
typedef struct pduIHave
{
	UINT64 mostRecentSeqNum;
	UINT YerOrNo;																	//�Ƿ�ӵ�и���Դ
	UINT UID;																		//ӵ����Դ�Ľڵ��UID
	UINT tsUID;
	UINT ipAddress;																	//ӵ����Դ�Ľڵ��IP��ַ	
	UINT channelID;																	//��Դ��Ƶ��ID
	UINT bitRate;																	//��Դ��������С����bpsΪ��λ
	UINT pcrPID;																	//PCR���ĵ�ID��
	time_t synStartTime;															//��ԴУ׼ʱ��
}pduIHave;


//�����ͱ���
typedef struct pduPleaseStartTrans
{
	UINT64 expectedSeqNum;
	UINT channelID;																	//Ƶ��ID
	USHORT consumerPort;															//Ƶ������UDP�˿ں�
	time_t s;																		//Ƶ��������ʼʱ��
}pduPleaseStartTrans;

//������Ӧ����
typedef struct pduStartTrans
{
	UINT channelID;																	//Ƶ��ID
	time_t synStartTransTime;														//Ƶ��ʵ����ʼ����ʱ��
}pduStartTrans;

typedef struct pduResendPair
{
	UINT64 resendStartSeq;															//�ش���ʼ���к�
	UINT64 resendStopSeq;
}pduResendPair;

//�����ش�����
typedef struct pduNextTrans
{
	//UINT64 resendStartSeq;															//�ش���ʼ���к�
	//UINT64 resendStopSeq;
	UINT channelID;																	//Ƶ��ID
	UINT pairNumber;																	//�ش���ʼ�ֽ�
	UINT lastIndex;	
	UINT tsUID;														//�ϴγɹ����ܵ�������

	//UINT resendSeqNum;
	//�ش���ֹ���к�
}pduNextTrans;

//��ý�屨��
typedef struct pduTransData
{
	UINT tsUID;
	UINT channelID;																	//Ƶ��ID
	UINT fileMS;																	//�ļ�����ʱ��
	//	UINT64 grossBytes;
	//	UCHAR index;
	//	BYTE data[];
}pduTransData;

//����ʼ��ת����
typedef struct pduPleaseStartRelayTrans
{
	UINT channelID;																	//Ƶ��ID
	UINT realSupplierUID;															//��ʵ�ṩ��UID
	USHORT consumerPort;															//�����߽�������UDP�˿ں�
	time_t s;																		//��ʵ����ʱ��	
}pduPleaseStartRelayTrans;


//����ʼ�д�����Ӧ����
typedef struct pduStartRelayTrans
{
	UINT channelID;																	//Ƶ��ID
	time_t synStartRelayTransTime;													//ʵ�ʿ�ʼ����ʱ��
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
//������
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

//�ṩ��
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



//��ת��
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


//��ѯ
typedef struct tsQueryTask
{
	UINT consumerUID;
	UINT destUID;
	UINT channelID;
	time_t s;
	time_t e;
}tsQueryTask;





//��ʼ��ʱ���жϴ�����
int initTimer();

//��ʼ���������ӣ�����Dll���μ��ص�ʱ����ɶ˿ڴ������ڴ�����ʼ��
int initNetwork();

//���һ��������
int addNewConsumer(tsConsumerTask *cInfo);

//ftds����
int ftdsTest();

//��������ģ�����Դ
int destroyNetwork();

void sayHello2EveryOne();

void checkConsumer();

int startOneSupplier(CLiveChannel *channel, UINT64 firstPacketSeqNum, UINT tsUID, UINT fileIndex);

void sendChannelUpdate(tsSupplierTask *task, int channelEvent);

#endif //_NETMAIN_H
