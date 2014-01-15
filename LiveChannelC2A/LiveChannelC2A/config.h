//config.h 定义需要配置的变量和常量，以及加载配置的方法
#ifndef __CONFIG_H
#define __CONFIG_H

#include "def.h"
#include <string>
#define DEFAULT_BUFFER_SIZE         4096   // default buffer size
#define DEFAULT_PACKET_SIZE         1024   // default buffer size

#define REQ_BLOCK_PIECE_NUM 2048 //请求资源ID所代表的1KB数量
#define SLOT_PIECE_NUM 32 //每块所包含的1KB数量
#define SLOT_PIECE_REQ 32  //每次请求的1KB的数量
#define SLOT_PIECE_SIZE 1024 //默认的资源长度

#define MAX_FILE_NAME 255 //最大的文件名长度
#define DEFAULT_THREAD_COUNT 10 //默认的发送线程数
#define SLOT_INIT_COUNT 10 //初次分配的SLOT个数 1.5*MAX_SUBNODES_COUNT
#define SLOT_ONCE_COUNT 5 //每次分配的SLOT个数

#define TIMEOUT_OF_LIVENODE 5000 //节点没有响应的时间超过这个则删除
#define TIME_OF_RETRY_NETSEARCH 5 //节点查询失败后超过这个时间再重新查询，秒
#define TIME_OF_FILE_CHECK 60 //每分钟检查一次文件是否删除
#define TIME_OF_FILE_CLOSE 60*5 //每5分钟检查一次文件是否需要关闭

#ifdef _DEBUG
#define MAX_SUBNODES_COUNT 24 //最大提供服务的节点数
#define MAX_SLOT_COUNT 36 //最多分配的SLOT个数 1.5*MAX_SUBNODES_COUNT
#define MAX_RESTASK_COUNT 96 //最多分配的资源传输任务数 3*MAX_SUBNODES_COUNT
#else
#define MAX_SUBNODES_COUNT 120 //最大提供服务的节点数
#define MAX_SLOT_COUNT 600 //最多分配的SLOT个数 5*MAX_SUBNODES_COUNT
#define MAX_RESTASK_COUNT 1200 //最多分配的资源传输任务数 10*MAX_SUBNODES_COUNT
#endif // _DEBUG

#define MAX_SUBNODE_TASK_COUNT 320 //一个子节点允许的最大任务节点数


typedef enum NEIGHBOR_NODE_STATUS
{
    nnAlive,//网络尚未初始化
    nnTimeOut,//尚未登录
    nnDead//登录密码错误
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


extern UINT16 tcpResSearchPort;//DSN接收文件请求的端口
extern UINT16 tcpLivePort;//DSN接收直播请求的端口
extern UINT16 tcpSyncNotifyPort;//同步服务接收同步任务的监听端口
extern UINT16 udpResSendPort;//DSN发送本地资源文件的端口
extern UINT16 udpNATSendPort;//DSN发送NAT信息的端口
extern UINT32 msSyncIP;//本地同步服务的IP
extern UINT32 localIP;//本地服务IP

extern UINT32 mediaInfoReqIP;
extern UINT16 tcpMediaInfoReqPort;//媒体信息服务接收任务的监听端口

extern CHAR strIP1[];//保存配置的TS域名
extern CHAR strIP2[];
extern UINT32 TSIP;
extern UINT16 tcpTSLoginPort;//TS接收登录请求的端口
extern UINT16 tcpTSSearchPort;//TS接收资源查询的端口
extern UINT16 tcpTSLivePort;//TS接收直播流请求的端口
extern UINT16 udpTSKeepLivePort;//TS接收保活包的端口
extern UINT16 udpTSNatPort;//TS接收NAT-SS1的端口

extern INT TIMEOUT_OF_SEND;
extern INT TIMEOUT_OF_RECV;//TCP接收和发送的超时时间

extern INT TIMEOUT_OF_LOGIN;//毫秒，登录等待超时时间
extern INT COUNT_OF_RETRY;//重试次数

//TCP接收缓冲区
extern INT DEFAULT_RING_SIZE;
extern INT DEFAULT_POST_SIZE;
extern INT MAX_PACKET_LENGTH;

extern UINT MAX_ACCEPT_CLIENT;//最大的未处理连接数

extern INT TIMEOUT_OF_PAYLOAD;//毫秒，本机负载计时器超时时间
extern INT TIMEOUT_OF_KEEPLIVE;//毫秒，本机保活超时时间
extern INT TIMEOUT_OF_MONITOR;//毫秒，监控输出时间

extern UINT32 PACKET_SEND_PERIOD;//毫秒，每周期发送一次UDP包

//用于临时的认证
extern CHAR g_guid[];

//加载配置文件
INT loadConfig();
INT LoadDBHost(CHAR *pHost,UINT32 *pPort);
#endif
