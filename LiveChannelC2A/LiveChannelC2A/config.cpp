#include "stdafx.h"
#include "config.h"
#include "xmlParser.h"
#include "message.h"

//��־����
LOGLEVEL logLevel=logDEBUG;//���������Ϣ,logDEBUG,logINFO,logWARN,logERROR,logFATAL
LONG MaxLogSize=1*1024*1024;//������־�ļ���С

UINT32 localIP = INADDR_ANY;

char strIP1[128];//�������õ�TS����
char strIP2[128];


int TIMEOUT_OF_SEND=5000;
int TIMEOUT_OF_RECV=5000;//TCP���պͷ��͵ĳ�ʱʱ��
int TIMEOUT_OF_NEIGHBOUR_CHECK=30000*1000;//Nbour��ʱ��������,50����

int TIMEOUT_OF_LOGIN=50000;//���룬��¼�ȴ���ʱʱ��
int COUNT_OF_RETRY=5;//���Դ���
int WAITTIME_OF_RETRY=1000;//���룬���Եȴ�ʱ��
int TIMEOUT_OF_CONNSN=5000;//���룬����SN�ĵȴ���ʱʱ��

int TIMEOUT_OF_DESTROY=5000;//���룬����ȴ���ʱʱ��

//TCP���ջ�����
int DEFAULT_RING_SIZE=1024*56;
int DEFAULT_POST_SIZE=DEFAULT_BUFFER_SIZE;
int MAX_PACKET_LENGTH=DEFAULT_BUFFER_SIZE;

int DEFAULT_NEIGHBOUR_COUNT=10;//Ĭ�ϵ�Neighbours���б����

UINT MAX_ACCEPT_CLIENT=10;//����δ����������

int TIMEOUT_OF_PAYLOAD=5000;//���룬�������ؼ�ʱ����ʱʱ��
int TIMEOUT_OF_KEEPLIVE=2000;//���룬�������ʱʱ��
int TIMEOUT_OF_MONITOR=2000;//���룬������ʱ��

UINT32 PACKET_SEND_PERIOD=4;//���룬ÿ���ڷ���һ��UDP��

//������ʱ����֤
CHAR g_guid[]={"00000000-0000-0000-0000-000000000000"};


UINT32 msSyncIP = inet_addr("127.0.0.1");
UINT16 tcpResSearchPort;//DSN�����ļ�����Ķ˿�
UINT16 tcpLivePort;//DSN����ֱ������Ķ˿�
UINT16 tcpSyncNotifyPort;//ͬ���������ͬ������ļ����˿�
UINT16 udpResSendPort;//DSN���ͱ�����Դ�ļ��Ķ˿�
UINT16 udpNATSendPort;//DSN����NAT��Ϣ�Ķ˿�


UINT32 mediaInfoReqIP = inet_addr("127.0.0.1");
UINT16 tcpMediaInfoReqPort;//ͬ���������ͬ������ļ����˿�

UINT16 tcpTSLoginPort;//TS���յ�¼����Ķ˿�
UINT16 tcpTSSearchPort;//TS������Դ��ѯ�Ķ˿�
UINT16 tcpTSLivePort;//TS����ֱ��������Ķ˿�,20100910����
UINT16 udpTSKeepLivePort;//TS���ձ�����Ķ˿�
UINT16 udpTSNatPort;//TS����NAT-SS1�Ķ˿�

//���������ļ�,����־�ļ�
int loadConfig()
{
    tcpResSearchPort = htons(5105);//DSN�����ļ�����Ķ˿�
    tcpLivePort = htons(5125);//DSN����ֱ������Ķ˿�
    tcpSyncNotifyPort = htons(5103);//ͬ���������ͬ������ļ����˿�
    udpResSendPort = htons(5101);//DSN���ͱ�����Դ�ļ��Ķ˿�
    udpNATSendPort = htons(5113);//DSN����NAT��Ϣ�Ķ˿�

    tcpMediaInfoReqPort = htons(5119);//ͬ���������ͬ������ļ����˿�
    tcpTSLoginPort = htons(5007);//TS���յ�¼����Ķ˿�
    tcpTSSearchPort = htons(5005);//TS������Դ��ѯ�Ķ˿�
    tcpTSLivePort = htons(5021);//TS����ֱ��������Ķ˿�,20100910����
    udpTSKeepLivePort=htons(5011);//TS���ձ�����Ķ˿�
    udpTSNatPort=htons(5013);//TS����NAT-SS1�Ķ˿�

    //��ʼ��������Ϣ
    strIP1[0] = 0;
    strIP2[0] = 0;
    PACKET_SEND_PERIOD = 4;
    
    //��ȡXML�ļ�����ȡ��Ӧ��������Ϣ
    XMLResults results;
    XMLNode xMainNode = XMLNode::parseFile("../config.xml","config",&results);

    if(xMainNode.isEmpty()|| results.error!=eXMLErrorNone)
    {
        logger(msgERROR,"1loadConfig����ȡ���ڵ�config����");
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
    //��ʼ������������Ϣ
    strcpy(pHost,"localhost");
    *pPort=0;

    //��ȡXML�ļ�����ȡ��Ӧ��������Ϣ
    XMLResults results;
    XMLNode xMainNode = XMLNode::parseFile("../config.xml","config",&results);

    if(xMainNode.isEmpty()|| results.error!=eXMLErrorNone)
    {
        logger(msgERROR,"LoadDBHost����ȡ���ڵ�config����");
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
