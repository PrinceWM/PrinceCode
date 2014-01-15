//config.h ������Ҫ���õı����ͳ������Լ��������õķ���
#ifndef __CONFIG_H
#define __CONFIG_H

#include "def.h"
#include <string>
#define DEFAULT_BUFFER_SIZE         4096   // default buffer size
#define DEFAULT_PACKET_SIZE         1024   // default buffer size

#define REQ_BLOCK_PIECE_NUM 2048 //������ԴID�������1KB����
#define SLOT_PIECE_NUM 32 //ÿ����������1KB����
#define SLOT_PIECE_REQ 32  //ÿ�������1KB������
#define SLOT_PIECE_SIZE 1024 //Ĭ�ϵ���Դ����

#define MAX_FILE_NAME 255 //�����ļ�������
#define DEFAULT_THREAD_COUNT 10 //Ĭ�ϵķ����߳���
#define SLOT_INIT_COUNT 10 //���η����SLOT���� 1.5*MAX_SUBNODES_COUNT
#define SLOT_ONCE_COUNT 5 //ÿ�η����SLOT����

#define TIMEOUT_OF_LIVENODE 5000 //�ڵ�û����Ӧ��ʱ�䳬�������ɾ��
#define TIME_OF_RETRY_NETSEARCH 5 //�ڵ��ѯʧ�ܺ󳬹����ʱ�������²�ѯ����
#define TIME_OF_FILE_CHECK 60 //ÿ���Ӽ��һ���ļ��Ƿ�ɾ��
#define TIME_OF_FILE_CLOSE 60*5 //ÿ5���Ӽ��һ���ļ��Ƿ���Ҫ�ر�

#ifdef _DEBUG
#define MAX_SUBNODES_COUNT 24 //����ṩ����Ľڵ���
#define MAX_SLOT_COUNT 36 //�������SLOT���� 1.5*MAX_SUBNODES_COUNT
#define MAX_RESTASK_COUNT 96 //���������Դ���������� 3*MAX_SUBNODES_COUNT
#else
#define MAX_SUBNODES_COUNT 120 //����ṩ����Ľڵ���
#define MAX_SLOT_COUNT 600 //�������SLOT���� 5*MAX_SUBNODES_COUNT
#define MAX_RESTASK_COUNT 1200 //���������Դ���������� 10*MAX_SUBNODES_COUNT
#endif // _DEBUG

#define MAX_SUBNODE_TASK_COUNT 320 //һ���ӽڵ�������������ڵ���


typedef enum NEIGHBOR_NODE_STATUS
{
    nnAlive,//������δ��ʼ��
    nnTimeOut,//��δ��¼
    nnDead//��¼�������
}NEIGHBOR_NODE_STATUS;

typedef struct myConfig
{
	char nodeType;
	SOCKADDR_IN addrSrv;
    UINT ipAddress;
    USHORT publicationPort;
    USHORT supplyPort;
    UINT UID;
    USHORT zoneNumber;
    USHORT Capability;
    UCHAR timeZone;
    volatile SHORT currLoad;
    volatile LONG seqNumber;
    CHAR linkOneMonitorFilePath[256];
    CHAR linkTwoMonitorFilePath[256];
    INT maxSupplyBuffer;
	std::string carrier;
	std::string province;
	std::string city;
	SOCKADDR_IN internetAddr;
        INT logLevel;
        INT logSaved;
}myConfig;

typedef struct trackerNode
{
	UINT ipAddress;
	USHORT servicePort;
}trackerNode;

typedef struct neighborNode
{
    UINT UID;
    UINT ipAddress;
    USHORT supplyPort;
    USHORT zoneNumber;
    USHORT Capability;
    USHORT currLoad;
    UCHAR Status;
    BOOL checked;
    SOCKADDR_IN publicationAddr;
	SOCKADDR_IN intranetAddr;
	SOCKADDR_IN dataTransferAddr;
}neighborNode;


extern UINT16 tcpResSearchPort;//DSN�����ļ�����Ķ˿�
extern UINT16 tcpLivePort;//DSN����ֱ������Ķ˿�
extern UINT16 tcpSyncNotifyPort;//ͬ���������ͬ������ļ����˿�
extern UINT16 udpResSendPort;//DSN���ͱ�����Դ�ļ��Ķ˿�
extern UINT16 udpNATSendPort;//DSN����NAT��Ϣ�Ķ˿�
extern UINT32 msSyncIP;//����ͬ�������IP
extern UINT32 localIP;//���ط���IP

extern UINT32 mediaInfoReqIP;
extern UINT16 tcpMediaInfoReqPort;//ý����Ϣ�����������ļ����˿�

extern CHAR strIP1[];//�������õ�TS����
extern CHAR strIP2[];
extern UINT32 TSIP;
extern UINT16 tcpTSLoginPort;//TS���յ�¼����Ķ˿�
extern UINT16 tcpTSSearchPort;//TS������Դ��ѯ�Ķ˿�
extern UINT16 tcpTSLivePort;//TS����ֱ��������Ķ˿�
extern UINT16 udpTSKeepLivePort;//TS���ձ�����Ķ˿�
extern UINT16 udpTSNatPort;//TS����NAT-SS1�Ķ˿�

extern INT TIMEOUT_OF_SEND;
extern INT TIMEOUT_OF_RECV;//TCP���պͷ��͵ĳ�ʱʱ��

extern INT TIMEOUT_OF_LOGIN;//���룬��¼�ȴ���ʱʱ��
extern INT COUNT_OF_RETRY;//���Դ���

//TCP���ջ�����
extern INT DEFAULT_RING_SIZE;
extern INT DEFAULT_POST_SIZE;
extern INT MAX_PACKET_LENGTH;

extern UINT MAX_ACCEPT_CLIENT;//����δ����������

extern INT TIMEOUT_OF_PAYLOAD;//���룬�������ؼ�ʱ����ʱʱ��
extern INT TIMEOUT_OF_KEEPLIVE;//���룬�������ʱʱ��
extern INT TIMEOUT_OF_MONITOR;//���룬������ʱ��

extern UINT32 PACKET_SEND_PERIOD;//���룬ÿ���ڷ���һ��UDP��

//������ʱ����֤
extern CHAR g_guid[];

//���������ļ�
INT loadConfig();
INT LoadDBHost(CHAR *pHost,UINT32 *pPort);
#endif
