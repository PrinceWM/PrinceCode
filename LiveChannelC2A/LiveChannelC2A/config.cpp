#include "stdafx.h"
#include "config.h"
#include "xmlParser.h"
#include "message.h"

//日志级别
LOGLEVEL logLevel=logDEBUG;//输出所有信息,logDEBUG,logINFO,logWARN,logERROR,logFATAL
LONG MaxLogSize=1*1024*1024;//最大的日志文件大小

UINT32 localIP = INADDR_ANY;

char strIP1[128];//保存配置的TS域名
char strIP2[128];


int TIMEOUT_OF_SEND=5000;
int TIMEOUT_OF_RECV=5000;//TCP接收和发送的超时时间
int TIMEOUT_OF_NEIGHBOUR_CHECK=30000*1000;//Nbour超时检测的周期,50分钟

int TIMEOUT_OF_LOGIN=50000;//毫秒，登录等待超时时间
int COUNT_OF_RETRY=5;//重试次数
int WAITTIME_OF_RETRY=1000;//毫秒，重试等待时间
int TIMEOUT_OF_CONNSN=5000;//毫秒，连接SN的等待超时时间

int TIMEOUT_OF_DESTROY=5000;//毫秒，清理等待超时时间

//TCP接收缓冲区
int DEFAULT_RING_SIZE=1024*56;
int DEFAULT_POST_SIZE=DEFAULT_BUFFER_SIZE;
int MAX_PACKET_LENGTH=DEFAULT_BUFFER_SIZE;

int DEFAULT_NEIGHBOUR_COUNT=10;//默认的Neighbours后备列表个数

UINT MAX_ACCEPT_CLIENT=10;//最大的未处理连接数

int TIMEOUT_OF_PAYLOAD=5000;//毫秒，本机负载计时器超时时间
int TIMEOUT_OF_KEEPLIVE=2000;//毫秒，本机保活超时时间
int TIMEOUT_OF_MONITOR=2000;//毫秒，监控输出时间

UINT32 PACKET_SEND_PERIOD=4;//毫秒，每周期发送一次UDP包

//用与临时的认证
CHAR g_guid[]={"00000000-0000-0000-0000-000000000000"};


UINT32 msSyncIP = inet_addr("127.0.0.1");
UINT16 tcpResSearchPort;//DSN接收文件请求的端口
UINT16 tcpLivePort;//DSN接收直播请求的端口
UINT16 tcpSyncNotifyPort;//同步服务接收同步任务的监听端口
UINT16 udpResSendPort;//DSN发送本地资源文件的端口
UINT16 udpNATSendPort;//DSN发送NAT信息的端口


UINT32 mediaInfoReqIP = inet_addr("127.0.0.1");
UINT16 tcpMediaInfoReqPort;//同步服务接收同步任务的监听端口

UINT16 tcpTSLoginPort;//TS接收登录请求的端口
UINT16 tcpTSSearchPort;//TS接收资源查询的端口
UINT16 tcpTSLivePort;//TS接收直播流请求的端口,20100910新增
UINT16 udpTSKeepLivePort;//TS接收保活包的端口
UINT16 udpTSNatPort;//TS接收NAT-SS1的端口

//加载配置文件,打开日志文件
int loadConfig()
{
    tcpResSearchPort = htons(5105);//DSN接收文件请求的端口
    tcpLivePort = htons(5125);//DSN接收直播请求的端口
    tcpSyncNotifyPort = htons(5103);//同步服务接收同步任务的监听端口
    udpResSendPort = htons(5101);//DSN发送本地资源文件的端口
    udpNATSendPort = htons(5113);//DSN发送NAT信息的端口

    tcpMediaInfoReqPort = htons(5119);//同步服务接收同步任务的监听端口
    tcpTSLoginPort = htons(5007);//TS接收登录请求的端口
    tcpTSSearchPort = htons(5005);//TS接收资源查询的端口
    tcpTSLivePort = htons(5021);//TS接收直播流请求的端口,20100910新增
    udpTSKeepLivePort=htons(5011);//TS接收保活包的端口
    udpTSNatPort=htons(5013);//TS接收NAT-SS1的端口

    //初始化配置信息
    strIP1[0] = 0;
    strIP2[0] = 0;
    PACKET_SEND_PERIOD = 4;
    
    //读取XML文件，获取对应的配置信息
    XMLResults results;
    XMLNode xMainNode = XMLNode::parseFile("../config.xml","config",&results);

    if(xMainNode.isEmpty()|| results.error!=eXMLErrorNone)
    {
        logger(msgERROR,"1loadConfig：获取根节点config出错");
        return -1;
    }

    XMLNode tsNode = xMainNode.getChildNode("Tracker");
    if (!tsNode.isEmpty())
    {
        //host="192.168.3.173" loginPort="5007" ssnResPort="5003" dsnResPort="5005" keepAlivePort="5011" 
        const char* ts1 = tsNode.getAttribute("ts1");
        const char* ts2 = tsNode.getAttribute("ts2");
        const char* loginPort = tsNode.getAttribute("loginPort");
        const char* dsnResPort = tsNode.getAttribute("dsnResPort");
        const char* keepAlivePort = tsNode.getAttribute("keepAlivePort");

        if (ts1 != NULL)
        {
            strcpy(strIP1,ts1);
        }
        
        if (ts2 != NULL)
        {
            strcpy(strIP2,ts2);
        }

        if (loginPort != NULL)
        {
            tcpTSLoginPort = htons(atoi(loginPort));
        }

        if (dsnResPort != NULL)
        {
            tcpTSSearchPort = htons(atoi(dsnResPort));
        }

        if (keepAlivePort != NULL)
        {
            udpTSKeepLivePort = htons(atoi(keepAlivePort));
        }
    }

    XMLNode dsnNode = xMainNode.getChildNode("DsnServer");
    if (!dsnNode.isEmpty())
    {
        //<Trans sendPeriod="4" />
        XMLNode transNode = dsnNode.getChildNode("Trans");
        if (!transNode.isEmpty())
        {
            const char * period = transNode.getAttribute("sendPeriod");
            if (period != NULL)
            {
                PACKET_SEND_PERIOD = atoi(period);
            }
        }
    }

    return 0;
}

int LoadDBHost(char *pHost,UINT32 *pPort)
{
    //初始化基本配置信息
    strcpy(pHost,"localhost");
    *pPort=0;

    //读取XML文件，获取对应的配置信息
    XMLResults results;
    XMLNode xMainNode = XMLNode::parseFile("../config.xml","config",&results);

    if(xMainNode.isEmpty()|| results.error!=eXMLErrorNone)
    {
        logger(msgERROR,"LoadDBHost：获取根节点config出错");
        return -1;
    }

    //<Database serviceName="MySQL" dbHost="127.0.0.1" dbPort="3306"/>
    XMLNode tsNode = xMainNode.getChildNode("Database");
    if (!tsNode.isEmpty())
    {        
        const char* dbHost = tsNode.getAttribute("dbHost");
        const char* dbPort = tsNode.getAttribute("dbPort");

        if (dbHost != NULL)
        {
            strcpy(pHost,dbHost);
        }

        if (dbPort != NULL)
        {
            *pPort = atoi(dbPort);
        }
    }

    return 0;
}
