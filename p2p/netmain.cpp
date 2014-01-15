#include "stdafx.h"
#include "netmain.h"
#include "config.h"
#include "message.h"
#include "LiveChannel.h"
#include "myfile.h"
#include "tracker.h"
#include "upnpnat.h"
#include "getLocalIP.h"
#include "IPLocation.h"

#if defined(MOHO_WIN32)
#include <time.h>
#include <WinSock.h>
#endif

#ifndef MOHO_WIN32
#include <endian.h>
    #if __BYTE_ORDER == __LITTLE_ENDIAN
    #define _IS_LITTLE_ENDIAN 1
    #endif
#endif

#if defined(MOHO_X86)
timer_t g_neighbor_process_timeid = NULL;
timer_t g_consumer_process_timeid = NULL;
#elif defined(MOHO_WIN32)
extern HWND pMainWnd;
#endif

void InitTsSync();
BOOL InitLiveModule();
int uPnP(USHORT intraPort, USHORT interPort);

SOCKET sockPublicationSrv;
channelPublicationThreadInfo Info;
SOCKET sendSocket;
//tsResendTaskPara para;

//SOCKET sendSocketInConsumerTimer;

#if defined(MOHO_X86)
void* ftdsThreadProc1(LPVOID lpParameter);
#elif defined(MOHO_WIN32)
static DWORD WINAPI ftdsThreadProc1(LPVOID lpParameter);
#endif




int replyTo(pduHeader *hdr, void *pdu, SOCKADDR_IN *addr, SOCKET *sendSocket);


extern std::list<CLiveChannel *> globalLiveChannelList;
extern std::list<neighborNode> globalNeighborNodeList;
extern std::list<tsSupplierTask> globalTsSupplierTaskList;
extern std::list<tsConsumerTask> globalTsConsumerTaskList;
extern std::list<tsRelayTask> globalTsRelayTaskList;
extern std::list<tsQueryTask> globalTsQueryTaskList;
extern std::deque<trackerNode> globalTrackerDeque;
extern myConfig globalConfig;

extern CMyLock lockLiveChannelList;
extern CMyLock lockNeighborNodeList;
extern CMyLock lockTsSupplierTaskList;
//extern CRITICAL_SECTION lockTsConsumerTaskList;
extern CMyLock lockRelayTaskList;
extern CMyLock lockQueryTaskList;

extern void deleteOldFile(tsConsumerTask *pInfo);//删除接收端目录下所有ts类型的文件，在程序重新启动时调用
//CRITICAL_SECTION lockConsumerBuf;
//CRITICAL_SECTION lockWaitForSeqList;

//int consumerTimes;

int sendNextTransForLostPackets(tsConsumerTask *cIter);
int sendNextTransForSomeLost(tsConsumerTask *cIter, UINT64 startSeq, UINT64 stopSeq);


int updateNeighborList(pduHowAreYou *pHow, SOCKADDR_IN *addr)
{
    std::list<neighborNode>::iterator nIter;

    UINT ipAddress;
#if defined(MOHO_X86)
    ipAddress = ntohl(addr->sin_addr.s_addr);
#elif defined(MOHO_WIN32)
    ipAddress = ntohl(addr->sin_addr.S_un.S_addr);
#endif

    lockNeighborNodeList.lock();
    for(nIter = globalNeighborNodeList.begin(); nIter != globalNeighborNodeList.end(); nIter ++)
    {
        if(nIter->UID == pHow->myUID)
        {
            nIter->publicationAddr = *addr;
            if(nIter->Status < nnsAlive)
            {
                nIter->Status = nnsAlive;
            }
            messager(msgINFO, "update publication addr for %s, UID = %d", inet_ntoa(addr->sin_addr), pHow->myUID);
            break;
        }
        else if(nIter->Status == nnsUnknown && ipAddress == nIter->ipAddress)
        {
            nIter->publicationAddr = *addr;
            nIter->UID = pHow->myUID;
            break;
        }
    }
    if(nIter == globalNeighborNodeList.end())
    {
        neighborNode nn;
        nn.ipAddress =0;
        //nn.Status = nnsDead;
        nn.Status = nnsAlive;
        nn.Capability = 0;
        nn.currLoad = 0;
        nn.supplyPort = globalConfig.publicationPort;
        nn.UID = pHow->myUID;
        nn.zoneNumber = 0;
        nn.checked = true;
        nn.publicationAddr = *addr;
        globalNeighborNodeList.push_back(nn);
        messager(msgINFO, "insert new neighbor %s, UID = %d",  inet_ntoa(addr->sin_addr), pHow->myUID);
    }
    lockNeighborNodeList.unlock();
    return 1;
}


int getFileIndexFromFile(const char *channelName, const char *fileName)
{
    char index[20];
    //strncpy(index, file.name + 3 + 2 + strlen(channelname), 4);

    UINT j = 3 + strlen(channelName) + 8 + 6 + 6 + 5;
    UINT jIndex = 0;
    while(fileName[j] != '_' && j < strlen(fileName))
    {
        index[jIndex] = fileName[j];
        jIndex ++;
        j ++;
    }
    index[jIndex] = '\0';
    return atoi(index);
}

//删除消费者目录下的文件，在系统重新启动或自动恢复过程中调用
void deleteFtdsOldFile(tsConsumerTask *pInfo)
{
    return;

    CMyFile myfile;
    char filespec[256];

#if defined(MOHO_X86)
    mysprintf(filespec, 256, "%3d_(.*).ts", pInfo->channelID);
#elif defined(MOHO_WIN32)
    mysprintf(filespec, 256, "%3d_*.ts", pInfo->channelID);
#endif
    for(UINT i = 0; i<strlen(filespec); i++)
    {
        if(filespec[i] == ' ')
            filespec[i] = '0';
    }
    if(1 == myfile.findFirst((char*)pInfo->channelPath.c_str(),filespec))
    {
        do
        {
            char *name = myfile.getName();
            char filename[256];
            mysprintf(filename, 256, "%s%s", pInfo->channelPath.c_str(), name);
            if(remove(filename) == -1)
            {
                messager(msgERROR, "EXIT FTDS, file %s can not be removed", name);
            }
            else
            {
                messager(msgINFO, "EXIT FTDS, file %s be removed", name);
                //Sleep(16);
            }
        }while(myfile.findNext());
    }
    myfile.close();

    //std::list<std::string>::iterator sIter;
    //for(sIter = pInfo->savedFile.begin(); sIter != pInfo->savedFile.end(); sIter ++)
    //{
    //	if(remove(sIter->c_str()) == -1)
    //	{
    //		messager(msgERROR, "EXIT FTDS, file %s can not be removed", sIter->c_str());
    //	}
    //	else
    //	{
    //		messager(msgINFO, "EXIT FTDS, file %s be removed", sIter->c_str());
    //	}
    //	//Sleep(100);
    //}
}

void initConsumerSavedFiles(tsConsumerTask *pInfo)
{
    //return;

    CMyFile myfile;
    char filespec[256];

#if defined(MOHO_X86)
    mysprintf(filespec, 256, "%3d_(.*).ts", pInfo->channelID);
#elif defined(MOHO_WIN32)
    mysprintf(filespec, 256, "%3d_*.ts", pInfo->channelID);
#endif
    for(UINT i = 0; i<strlen(filespec); i++)
    {
        if(filespec[i] == ' ')
            filespec[i] = '0';
    }

    if(1 == myfile.findFirst((char*)pInfo->channelPath.c_str(),filespec))
    {
        char filename[256];
        do
        {
            char* name = myfile.getName();
            mysprintf(filename, 256, "%s%s", pInfo->channelPath.c_str(), name);
            pInfo->savedFile.push_back(filename);
            if(pInfo->savedFile.size() > MAX_FILE_SAVED)
            {
                remove(pInfo->savedFile.front().c_str());
                pInfo->savedFile.pop_front();
            }
            //if( remove(filename) == -1)
            //{
            //	messager(msgERROR, "EXIT FTDS, file %s can not be removed", file.name);
            //}
            //else
            //{
            //	messager(msgINFO, "EXIT FTDS, file %s be removed", file.name);
            //	//Sleep(16);
            //}
        }while(myfile.findNext());
    }
    myfile.close();

    messager(msgINFO, "%d files exist already", pInfo->savedFile.size());
}
#if defined(MOHO_WIN32)
typedef UINT64 (WINAPI GetTickCount64Proc)(void);
typedef ULONG (__stdcall * NTQUERYSYSTEMINFORMATION)(IN     /*SYSTEM_INFORMATION_CLASS*/int, IN OUT PVOID, INT    ULONG, OUT    PULONG OPTION);
GetTickCount64Proc* VistaGetTickCount64 = (GetTickCount64Proc*)GetProcAddress(GetModuleHandle(_T("kernel32.dll")), "GetTickCount64");

NTQUERYSYSTEMINFORMATION _NtQuerySystemInformation = (NTQUERYSYSTEMINFORMATION)GetProcAddress(GetModuleHandle(_T("ntdll.dll")), ("NtQuerySystemInformation"));
#endif

//64位GetTickCount，解决32位函数49天后回滚的问题
INT64 _GetTickCount64()
{
#if defined(MOHO_X86)
    INT64 time_ms;
    struct timeval ts;
    if(-1 == gettimeofday (&ts , NULL))
    {
        messager(msgINFO, "************gettimeofday failed");
    }
    time_ms = ts.tv_sec;
    time_ms = time_ms*1000 + ts.tv_usec/1000;
    return time_ms;
#elif defined(MOHO_WIN32)
    typedef struct _SYSTEM_TIME_OF_DAY_INFORMATION
    {
        LARGE_INTEGER BootTime;
        LARGE_INTEGER CurrentTime;
        LARGE_INTEGER TimeZoneBias;
        ULONG CurrentTimeZoneId;
    } SYSTEM_TIME_OF_DAY_INFORMATION, *PSYSTEM_TIME_OF_DAY_INFORMATION;
    //如果系统存在VistaGetTickCount64函数则调用系统的
    if (VistaGetTickCount64)
        return VistaGetTickCount64();
    SYSTEM_TIME_OF_DAY_INFORMATION  st ={0};
    ULONG                           oSize = 0;
    if((NULL == _NtQuerySystemInformation)||0 !=(_NtQuerySystemInformation(3, &st, sizeof(st), &oSize))||
       (oSize!= sizeof(st)))
        return GetTickCount();
    return (st.CurrentTime.QuadPart - st.BootTime.QuadPart)/10000;
#endif

}



//根据UID获取对应IP地址
//UINT getNeighborIP(UINT UID)
//{
//    std::list<neighborNode>::iterator nIter;
//    lockNeighborNodeList.lock();
//    for(nIter = globalNeighborNodeList.begin(); nIter != globalNeighborNodeList.end(); nIter ++)
//    {
//        if((*nIter).UID == UID)
//        {
//            lockNeighborNodeList.unlock();
//            return (*nIter).ipAddress;
//        }
//    }
//    lockNeighborNodeList.unlock();
//	return 0;
//}

////根据UID获取对应IP地址
//SOCKADDR_IN *getConsumerTaskNeighborAddr(UINT UID, UINT channelID)
//{
//    std::list<tsConsumerTask>::iterator cIter;
//    for(cIter = globalTsConsumerTaskList.begin(); cIter != globalTsConsumerTaskList.end(); cIter ++)
//    {
//		if(cIter->channelID == channelID)
//		{
//			for(UINT i = 0; i<cIter->neighborDeque.size(); i++)
//		   {
//				if(cIter->neighborDeque[i].UID == UID)
//				{
//					return &(cIter->neighborDeque[i].publicationAddr);
//				}
//			}
//		}
//	}
//    return NULL;
//}

SOCKADDR_IN *getNeighborPublicationAddr(UINT UID, SOCKADDR_IN *addClient)
{
    std::list<neighborNode>::iterator nIter;
    lockNeighborNodeList.lock();
    for(nIter = globalNeighborNodeList.begin(); nIter != globalNeighborNodeList.end(); nIter ++)
    {
        if((*nIter).UID == UID)
        {
            lockNeighborNodeList.unlock();
            return &(nIter->publicationAddr);
        }
    }
    lockNeighborNodeList.unlock();
    messager(msgERROR, "CAN NOT FOUND publicationAddr for UID %d", UID);
    return NULL;
}

//根据收到的心跳应答报文更新邻居状态
//int updateNeighborStatus(pduHeader* hdr, pduIAmOk* pOk, SOCKADDR_IN *addr)
int updateNeighborStatus(pduHeader* hdr, SOCKADDR_IN *addr)
{
    //return 0;
    lockNeighborNodeList.lock();
    int ret = 0;
    std::list<neighborNode>::iterator neighborIter;
    UINT ipAddress;
#if defined(MOHO_X86)
    ipAddress = ntohl(addr->sin_addr.s_addr);
#elif defined(MOHO_WIN32)
    ipAddress = ntohl(addr->sin_addr.S_un.S_addr);
#endif
    for(neighborIter = globalNeighborNodeList.begin(); neighborIter != globalNeighborNodeList.end(); neighborIter ++)
    {
        if((*neighborIter).UID == hdr->myUID || ( ipAddress == (*neighborIter).ipAddress && neighborIter->Status == nnsUnknown))
        {

#if defined(MOHO_X86)
            (*neighborIter).ipAddress = ntohl(addr->sin_addr.s_addr);
#elif defined(MOHO_WIN32)
            (*neighborIter).ipAddress = ntohl(addr->sin_addr.S_un.S_addr);
#endif
            if(neighborIter->Status < nnsAlive)
            {
                (*neighborIter).Status = nnsAlive;
            }
            (*neighborIter).UID = hdr->myUID;//UID changed or not initialized
            neighborIter->publicationAddr = *addr;
            lockNeighborNodeList.unlock();
            return ret;
        }
    }
    lockNeighborNodeList.unlock();
    return ret;
}

//收到资源查询回应报文后，更新消费者任务状态
int updateConsumerTask(pduHeader *hdr, pduIHave *pHave, SOCKADDR_IN *addr)
{
    //EnterCriticalSection(&lockTsConsumerTaskList);
    //EnterCriticalSection(&lockNeighborNodeList);
    int ret = 0;

    std::list<tsConsumerTask>::iterator consumerIter;
    for(consumerIter = globalTsConsumerTaskList.begin(); consumerIter != globalTsConsumerTaskList.end(); consumerIter ++)
    {
        //EnterCriticalSection(&consumerIter->lockConsumerThread);
        messager(msgDEBUG, "globalTsConsumerTaskList: channelID = %d, supplierUID = %d, lastSupplierUID = %d, pHave->channelID = %d, pHave->UID = %d, hdr->myUID = %d",
                 consumerIter->channelID,
                 consumerIter->supplierUID,
                 consumerIter->lastSupplierUID,
                 pHave->channelID,
                 pHave->UID,
                 hdr->myUID);
        if((*consumerIter).channelID == pHave->channelID &&
           (*consumerIter).supplierUID == 0 &&					//supplierUID == 0 means i have no supplier yet;
           (*consumerIter).lastSupplierUID != hdr->myUID &&
           consumerIter->Status == ctsNotStarted)
        {
            //std::list<neighborNode>::iterator neighborIter;
            if(hdr->myUID == pHave->UID)
            {
                consumerIter->consumerSrv = socket( AF_INET , SOCK_DGRAM , IPPROTO_UDP ) ;

#if defined(MOHO_X86)
                int optSO_REUSEADDR = 1;//设置soet可重用，否则重启程序后bind总是出错
                setsockopt(consumerIter->consumerSrv,SOL_SOCKET,SO_REUSEADDR,&optSO_REUSEADDR,sizeof(optSO_REUSEADDR));
#endif
                SOCKADDR_IN addrSrv ;

#if defined(MOHO_X86)
                addrSrv.sin_addr.s_addr = globalConfig.ipAddress ;
#elif defined(MOHO_WIN32)
                addrSrv.sin_addr.S_un.S_addr = globalConfig.ipAddress ;
#endif
                //addrSrv.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
                addrSrv.sin_family = AF_INET ;
                addrSrv.sin_port = htons(consumerIter->consumerPort) ;

                uPnP(consumerIter->consumerPort, consumerIter->consumerPort);

                if( bind( consumerIter->consumerSrv , (SOCKADDR*)&addrSrv , sizeof(SOCKADDR)) < 0)
                {
                    messager(msgFATAL, "bind error, port = %d, error number = %d", consumerIter->consumerPort, myGetLastError());
                    return -1;
                }
                else
                {
                    messager(msgINFO, "updateConsumerTask bind %s:%d_%d success", inet_ntoa(addrSrv.sin_addr), consumerIter->consumerPort,consumerIter->consumerSrv);
                }
                if((*consumerIter).supplierUID != hdr->myUID)
                {
                    (*consumerIter).supplierUID = hdr->myUID;
                    (*consumerIter).relayRealSupplierUID = pHave->UID;
                    (*consumerIter).lastPDUTime = 0;
                }
                (*consumerIter).bitRate = (*consumerIter).channel->channelBitRate = pHave->bitRate;
                (*consumerIter).pcrPID = (*consumerIter).channel->pcrPID = pHave->pcrPID;
                (*consumerIter).startTime = pHave->synStartTime;
                consumerIter->Status = ctsQueryFinished;
                messager(msgINFO, "update consumer task list, DIRECT : channelID = %d", pHave->channelID);



                {
                    consumerIter->consumerThread.terminate();
                    consumerIter->EXITconsumer = false;
                    addNewConsumer(&(*consumerIter));

                    consumerIter->ftdsThread.terminate();
                    consumerIter->EXITftds = false;

                    consumerIter->ftdsThread.creat(ftdsThreadProc1,(void*)&(*consumerIter));

                    ret = 0;
                    break;
                }
                ret = 1;
                break;
            }
            break;
        }
        else if((*consumerIter).channelID == pHave->channelID &&
                (*consumerIter).supplierUID > 0 &&					//supplierUID > 0 means change supplier;
                (*consumerIter).lastSupplierUID != hdr->myUID &&
                consumerIter->Status == ctsNotStarted)
        {
            if(hdr->myUID == pHave->UID)
            {
                if(consumerIter->nextPduSeq > 0 && consumerIter->tsUID == pHave->tsUID)
                {
                    if(consumerIter->nextPduSeq > pHave->mostRecentSeqNum)
                    {
                        messager(msgINFO, "channel %d supplier %d is slow than consumer, NOT CHOSEN", pHave->channelID, pHave->UID);
                        ret = 0;
                        return ret;
                    }
                }
                if((*consumerIter).supplierUID != hdr->myUID)
                {
                    (*consumerIter).supplierUID = hdr->myUID;
                }
                (*consumerIter).bitRate = (*consumerIter).channel->channelBitRate = pHave->bitRate;
                (*consumerIter).pcrPID = (*consumerIter).channel->pcrPID = pHave->pcrPID;
                (*consumerIter).startTime = pHave->synStartTime;
                consumerIter->Status = ctsQueryFinished;
                messager(msgINFO, "CONSUMER is running, update consumer task list, DIRECT : channelID = %d, supplierUID = %d", 
                         pHave->channelID,
                         consumerIter->supplierUID);
                ret = 1;
                //LeaveCriticalSection(&consumerIter->lockConsumerThread);
                break;
            }
        }
    }
    return ret;
}
//#define DUMPFILE

#ifdef DUMPFILE
FILE *g_dumpfile;
#endif

//向FTDS发包的算法1，线上系统目前采用这个算法
#if defined(MOHO_X86)
void* ftdsThreadProc1(LPVOID lpParameter)
#elif defined(MOHO_WIN32)
        static DWORD WINAPI ftdsThreadProc1(LPVOID lpParameter)
#endif
{
#if defined(MOHO_X86)
    signal(SIGUSR1, probeThreadExit);
#endif

    tsConsumerTask *pInfo = (tsConsumerTask *)lpParameter;

    messager(msgDEBUG, "ftdsThreadProc : channelID = %d, channel->channelID = %d", pInfo->channelID, pInfo->channel->channelID);

    if(pInfo->ftdsList.size() == 0)
    {
        messager(msgERROR, "channel %d has no ftds, EXIT THREAD", pInfo->channelID);
#if defined(MOHO_X86)
        return NULL;
#elif defined(MOHO_WIN32)
        return -1;
#endif
    }

    //deleteFtdsOldFile(pInfo);
    //cIter->savedFile.clear();

    SOCKET localSendSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    SOCKADDR_IN ipAddressFTDS;
    
    //ipAddressFTDS.sin_port = pInfo->portFTDS;
    //ipAddressFTDS.sin_addr.S_un.S_addr = pInfo->ipAddressFTDS;
    int tolen = sizeof(ipAddressFTDS);

    int fileIndex;

    CMyFile myfile;
    char filespec[256];
    char fileNameCache[260];

    BYTE buf[1500];


    //int sleepMS;
    //int pduSendPeriod = (1000 * 188 * TS_PDU_LENGTH * 8) / (pInfo->bitRate * 1024);
    //DWORD startTick;

    FILE *fp = NULL;

    time_t tmpT;
    DWORD ret = 0;
    time(&tmpT);

    //time_t endTimeOfLastFile;
    //time_t startTimeOfCurrFile;

    BYTE buf1[3000];
    int buf1Length = 0;
    INT64 totalPCR = 0;
    INT64 currPCR = 0;
    INT64 lastPCR = 0;

    UINT64 tick = 0;
    UINT64 tmpTick = _GetTickCount64();
    UINT64 errorTick = _GetTickCount64();
    BOOL firstPCR = true;
    UINT tsUID = pInfo->tsUID;

    while(true)
    {
        //if(pInfo->lastPDUTime > 0 &&
        //   pInfo->lastPDUTime - tmpT > 60 * TS_FTDS_TIME_OFFSET)
        if(pInfo->recvdFilesNumber > 1 && pInfo->fileIndex > 0)
        {
            //fileIndex = pInfo->channel->getTheFileIndex(&pInfo->startTime);
            //fileIndex = 1;
            fileIndex = pInfo->fileIndex;
            tsUID = pInfo->tsUID;
            //Sleep(5000);
            break;
        }
        else
        {
            mySleep(100);
        }
        if(pInfo->EXITftds)
        {
            goto endofftds;
        }
    }

    //fileIndex = 69;
    if(fp == NULL)
    {
        //001_ch0_20111220_122510_005600_899_A3B597CE.ts
        //snprintf(filespec, 256, "%s%3d_%s_%4d_*.ts"
        //	, pInfo->channelPath.c_str()
        //	, pInfo->channelID
        //	, pInfo->channelname
        //	, fileIndex);


#if defined(MOHO_X86)
        mysprintf(filespec, 256, "%3d_%s_(.?)(.?)(.?)(.?)(.?)(.?)(.?)(.?)_(.?)(.?)(.?)(.?)(.?)(.?)_(.?)(.?)(.?)(.?)(.?)(.?)_%d_(.*).(.*)"
                  , pInfo->channelID
                  , pInfo->channelName.c_str()
                  , fileIndex);
#elif defined(MOHO_WIN32)
        mysprintf(filespec, 256, "%3d_%s_????????_??????_??????_%d_*.*"
                  , pInfo->channelID
                  , pInfo->channelName.c_str()
                  , fileIndex);
#endif
        for(UINT i = 0; i<strlen(filespec); i++)
        {
            if(filespec[i] == ' ')
                filespec[i] = '0';
        }

        while(0 == myfile.findFirst((char*)pInfo->channelPath.c_str(),filespec))
        {
            myfile.close();
            messager(msgERROR, "ftdsThreadProc: %s has no such file", filespec);
            for(int i = 0; i<300; i++)
            {
                if(pInfo->EXITftds)
                {
                    goto endofftds;
                }
                mySleep(100);
            }
        }
        //else
        {
            char* name = myfile.getName();
            mysprintf(fileNameCache, 260, "%s",name);
            mysprintf(filespec, 256, "%s%s", pInfo->channelPath.c_str(), name);
            myfile.close();
            if((fp = fopen(filespec, "rb")) == NULL)
            {
                messager(msgERROR, "ftdsThreadProc: cannot open the file %s", filespec);
                  goto endofftds;
            }
            else
            {
                pInfo->channel->needToRestartConsumer = FALSE;
                //getEndTimeFromFileName(name, &endTimeOfLastFile);
                pInfo->channel->getPCRPidFromFile(name);
                messager(msgINFO, "ftdsThreadProc, channel %d open the FIRST file %d,  pcrPID = 0x%x",
                         pInfo->channelID,
                         fileIndex,
                         pInfo->channel->pcrPID);
            }
        }
    }

    while(!pInfo->EXITftds)
    {
        int	readLen = fread(buf, sizeof(char), TS_PDU_LENGTH * 188, fp);
        if(ferror(fp))
        {
            messager(msgERROR, "read file %s error", fileNameCache);
            goto endofftds;
        }
        else if(feof(fp))
        {
            fclose(fp);
            fp = NULL;
            fileIndex ++;
            int waitTimes = 0;
            waitfornextfile:
            if(pInfo->channel->needToRestartConsumer)
            {
                pInfo->channel->needToRestartConsumer = FALSE;
                messager(msgWARN, "ftds: TS for channel %d restart", pInfo->channelID);
                while(true)
                {
                    if(pInfo->recvdFilesNumber > 0)
                    {
                        time(&tmpT);

                        buf1Length = 0;
                        totalPCR = 0;
                        currPCR = 0;
                        lastPCR = 0;

                        tick = tmpTick = _GetTickCount64();
                        errorTick = _GetTickCount64();
                        firstPCR = true;

                        fileIndex = pInfo->fileIndex;
                        tsUID = pInfo->tsUID;
                        break;
                    }
                    else
                    {
                        mySleep(100);
                    }
                }
            }
#if defined(MOHO_X86)
            mysprintf(filespec, 256, "%3d_%s_(.?)(.?)(.?)(.?)(.?)(.?)(.?)(.?)_(.?)(.?)(.?)(.?)(.?)(.?)_(.?)(.?)(.?)(.?)(.?)(.?)_%d_(.*).(.*)"
                      , pInfo->channelID
                      , pInfo->channelName.c_str()
                      , fileIndex);
#elif defined(MOHO_WIN32)
            mysprintf(filespec, 256, "%3d_%s_????????_??????_??????_%d_*.*"
                      , pInfo->channelID
                      , pInfo->channelName.c_str()
                      , fileIndex);
#endif
            for(UINT i = 0; i<strlen(filespec); i++)
            {
                if(filespec[i] == ' ')
                    filespec[i] = '0';
            }


            while(0 == myfile.findFirst((char*)pInfo->channelPath.c_str(),filespec))
            {
                myfile.close();
				if(waitTimes % 10 == 0)
				{
					messager(msgERROR, "ftdsThreadProc: %s has no such file, TRY NEXT ID", filespec);
				}

                //messager(msgERROR, "ftdsThreadProc: %s has no such file, WAIT 30 seconds", filespec);

                //Sleep(30000);

                UINT64 t1 = _GetTickCount64();
                for(int j = 0; j<2; j++)
                {
                    //mySleep(1000);
					mySleep(100);
                    if(pInfo->EXITftds)
                    {
                        goto endofftds;
                    }
                }

                //continue;

                //fileIndex ++;
                waitTimes ++;

                UINT64 t2 = _GetTickCount64();
                totalPCR += (t2 - t1) * 90;//2013-1107

                if(waitTimes < 10000)
                {
                    goto waitfornextfile;
                }
                else
                {
                    messager(msgERROR, "ftdsThreadProc: channel %d try next id over 10 times, EXIT THREAD", pInfo->channelID);
                    goto endofftds;
                }
            }
            //else
            {
                char* name = myfile.getName();
                mysprintf(fileNameCache, 260, "%s",name);
                mysprintf(filespec, 256, "%s%s", pInfo->channel->channelFilePath.c_str(),name);
                myfile.close();

                waitfornextfilereadchance:
                if((fp = fopen(filespec, "rb")) == NULL)
                {
                    messager(msgERROR, "ftdsThreadProc: channel %d cannot open the NEXT file %d, EXIT THREAD", 
                             pInfo->channelID,
                             fileIndex);
                    UINT64 t1 = _GetTickCount64();
                    for(int j = 0; j<2; j++)
                    {
                        //mySleep(1000);
						mySleep(100);
                        if(pInfo->EXITftds)
                        {
                            goto endofftds;
                        }
                    }
                    UINT64 t2 = _GetTickCount64();
                    totalPCR += (t2 - t1) * 90;//2013-1107
                    goto waitfornextfilereadchance;
                }
                else
                {
                    UINT64 tmpTick = _GetTickCount64();
                    //if(pInfo->channelID == 1)
                    {

                        //getStartTimeFromFileName(name, &startTimeOfCurrFile);

                        //if(startTimeOfCurrFile - endTimeOfLastFile > 1)
                        //{
                        //    messager(msgERROR, "channel %d lost %d seconds", pInfo->channelID, (int)(startTimeOfCurrFile - endTimeOfLastFile));
                        //    //if(waitTimes > 0)
                        //    //{
                        //    //    messager(msgERROR, "ftdsThreadProc: channel %d need to adjust totalPCR", pInfo->channelID);
                        //    //    totalPCR += (startTimeOfCurrFile - endTimeOfLastFile) * 1000 * 90;
                        //    //}
                        //}

                        //getEndTimeFromFileName(name, &endTimeOfLastFile);

                        //pInfo->channel->getPCRPidFromFile(name);
                        messager(msgINFO, "ftdsThreadProc: channel %2d open the NEXT file %8d, totalPCR = %d ms, ftds = %d, diff = %d, pcrPID = 0x%x",
                                 pInfo->channelID,
                                 fileIndex,
                                 (int)totalPCR/90,
                                 (int)(tmpTick - tick),
                                 (int)(totalPCR/90 - (tmpTick - tick)),
                                 pInfo->channel->pcrPID);
                    }
                }
            }
        }

        if(readLen <= 0)
        {
            continue;
        }

        std::list<ftds>::iterator ftdsIter;
        for(ftdsIter = pInfo->ftdsList.begin(); ftdsIter != pInfo->ftdsList.end(); ftdsIter ++)
        {
            //if(ftdsIter->portFTDS == 0)
            {

                ipAddressFTDS = ftdsIter->recvAddr;
                ipAddressFTDS.sin_family = AF_INET;
                //messager(msgINFO, "ftds (%s:%d)", inet_ntoa(ipAddressFTDS.sin_addr), ntohs(ipAddressFTDS.sin_port));
            }
            //			else
            //			{
            //				ipAddressFTDS.sin_port = ftdsIter->portFTDS;
            //#if defined(MOHO_X86)
            //				ipAddressFTDS.sin_addr.s_addr = ftdsIter->ipAddressFTDS;
            //#elif defined(MOHO_WIN32)
            //				ipAddressFTDS.sin_addr.S_un.S_addr = ftdsIter->ipAddressFTDS;
            //#endif
            //			}
            int len = sendto(localSendSocket, (const char*)buf, readLen, 0,(SOCKADDR *)(&ipAddressFTDS), tolen);
         //   messager(msgERROR, "sendto %d__%d bytes to client %s:%d", readLen,len ,inet_ntoa(ipAddressFTDS.sin_addr),ntohs(ipAddressFTDS.sin_port));

            if(len == SOCKET_ERROR)
            {
                if(_GetTickCount64() - errorTick > 30 * 1000)
                {
                    messager(msgERROR, "channel %d send to FTDS %s failed, errno = %d",
                             pInfo->channelID,
                             inet_ntoa(ipAddressFTDS.sin_addr),
                             myGetLastError());
                    errorTick = _GetTickCount64();
                }
            }
            else if(len != readLen)
            {
                messager(msgERROR, "channel %d send to FTDS %s error, %d-%d",
                         pInfo->channelID,
                         inet_ntoa(ipAddressFTDS.sin_addr),
                         len,
                         readLen);
            }
            else
            {
            //    messager(msgDEBUG, "send data of channel %d to %s:%d", pInfo->channelID, inet_ntoa(ipAddressFTDS.sin_addr), ntohs(ipAddressFTDS.sin_port));
            }
        }

        memcpy(buf1 + buf1Length, buf, readLen);
        buf1Length += readLen;

        int cursor = 0;
        while(buf1Length >= 188)
        {
            if(buf1[cursor] == 0x47)
            {
                BYTE *inBuf = (BYTE *)(buf1 + cursor);
                if(inBuf[0] == 0x47 &&
                   (inBuf[1] & 0x1f) * 256 + inBuf[2] == pInfo->channel->pcrPID &&
                   inBuf[3] & 0x20 &&
                   inBuf[4] >= 0x07 &&
                   inBuf[5] & 0x10)
                {
                    lastPCR = currPCR;
                    currPCR = (inBuf[6] * 1677216 + inBuf[7] * 65536 + inBuf[8] * 256 + inBuf[9] ) * 2;
                    if(inBuf[10] & 0x80)
                    {
                        currPCR ++;
                    }
                    if(lastPCR != 0 && currPCR > lastPCR && currPCR - lastPCR < MAX_PCR_PERIOD_MS * 90)
                    {
                        totalPCR += (currPCR - lastPCR);
                    }
                }
                cursor += 188;
                buf1Length -= 188;
            }
            else
            {
                cursor ++;
                buf1Length --;
            }
        }
        memcpy(buf1, buf1+cursor, buf1Length);

        if(tick == 0)
        {
            tick = _GetTickCount64();
        }
        tmpTick = _GetTickCount64();

        int sleepMS = (int)(totalPCR / 90 - (tmpTick - tick));
        if(sleepMS > FTDS_SLEEP_TIME)
        {
            if(sleepMS >= 1000)
            {
                messager(msgERROR, "sleepMS__%d > FTDS_SLEEP_TIME__%d,name__%s", sleepMS, FTDS_SLEEP_TIME,fileNameCache);
            }
            //timeBeginPeriod(sleepMS);
            mySleep(sleepMS);
            //timeEndPeriod(sleepMS);
        }
    }

    endofftds:

    messager(msgERROR, "channel %d exit ftds for restart", pInfo->channelID);
    //std::list<std::string>::iterator sIter;
    //for(sIter = pInfo->savedFile.begin(); sIter != pInfo->savedFile.end(); sIter ++)
    //{
    //	remove(sIter->c_str());
    //}
    if(fp != NULL)
    {
        fclose(fp);
    }
    //deleteFtdsOldFile(pInfo);
    //pInfo->savedFile.clear();

    deleteFtdsOldFile(pInfo);
#if defined(MOHO_X86)
    close(localSendSocket);
#elif defined(MOHO_WIN32)
    closesocket(localSendSocket);
#endif
    //    pInfo->ftdsThread = NULL;
    //    pInfo->ftdsThreadID = 0;

#if defined(MOHO_X86)
    return NULL;
#elif defined(MOHO_WIN32)
    return ret;
#endif

}

////消费者线程，开启UDP端口，接收流媒体数据，存储，重发
//#if defined(MOHO_X86)
//void* consumerTaskThreadProc(LPVOID lpParameter)
//#elif defined(MOHO_WIN32)
//        static DWORD WINAPI consumerTaskThreadProc(LPVOID lpParameter)
//#endif
//{
//#if defined(MOHO_X86)
//    signal(SIGUSR1, probeThreadExit);
//#endif
//
//    tsConsumerTask *pInfo = (tsConsumerTask *)lpParameter;
//    pInfo->recvdFilesNumber = 0;
//    UINT64 firstPacketSeqNum = 1;
//    UINT lastTsUID = 0;
//
//    //pInfo->consumerSrv = socket( AF_INET , SOCK_DGRAM , IPPROTO_UDP ) ;
//    //
//    //SOCKADDR_IN addrSrv ;
//    //addrSrv.sin_addr.S_un.S_addr = globalConfig.ipAddress ;
//    ////addrSrv.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
//    //addrSrv.sin_family = AF_INET ;
//    //addrSrv.sin_port = htons(pInfo->consumerPort) ;
//    SOCKADDR_IN addrClient;
//    memset(&addrClient,0,sizeof(SOCKADDR_IN));
//    int addrClientLen = sizeof(addrClient);
//    //
//    //if( bind( pInfo->consumerSrv , (SOCKADDR*)&addrSrv , sizeof(SOCKADDR)) != 0)
//    //{
//    //	messager(msgFATAL, _T("bind error, port = %d, error number = %d"), pInfo->consumerPort, myGetLastError());
//    //	return -1;
//    //}
//    //else
//    //{
//    //	messager(msgINFO, "bind %s:%d success", inet_ntoa(addrSrv.sin_addr), pInfo->consumerPort);
//    //}
//
//    DWORD ret = 0;
//    //int fileNum = 0;
//    std::string newName;
//
//
//    std::string filePath = pInfo->channelPath;
//
//    FILE *myFile = NULL;
//    std::string fileName = pInfo->channelPath  +  pInfo->channelName + ".ts";
//
//    BYTE buf1[3000];
//    char recvBuf[1500];
//
//    int buf1Length = 0;
//
//    int readByte;
//
//    int cursor = 0;
//
//
//    time_t fileStartTime;
//    UINT64 fileTimeMS = 0;
//
//
//    //UINT64 lastPDUTime = 0;
//    //UINT64 linkTwoTime = 0;
//
//    //int linkNumber = 1;
//    //UINT resendBeginSeq = 0;
//    //int resendBeginTime = 0;
//
//
//    memset(buf1, 0, 3000);
//
//    UINT64 deleteNextTick = _GetTickCount64();
//    initConsumerSavedFiles(pInfo);
//    while(!pInfo->EXITconsumer)
//    {
//        //EnterCriticalSection(&pInfo->lockConsumerThread);
//        if(pInfo->arrivedBufList.size() == 0)
//        {
//            //sendNextTransTo(&(*cIter));
//        }
//        else
//        {
//            std::list<pduBuffer *>::iterator pIter;
//            UINT64 start = pInfo->waitForReplySeq.front();
//            UINT64 stop;
//            //int resendNumber = 0;
//            for(pIter = pInfo->arrivedBufList.begin(); pIter != pInfo->arrivedBufList.end(); pIter ++)
//            {
//                stop = (*pIter)->seqNum;
//                if(_GetTickCount64() - (*pIter)->arrivedTick > PDU_ROUND_TRIP)
//                {
//                    if(start < stop)
//                    {
//                        messager(msgINFO, "channel %d: PACKET LOST, from %lld to %lld, total = %lld, aSize = %d",
//                                 pInfo->channelID,
//                                 start,
//                                 stop - 1,
//                                 stop - start,
//                                 pInfo->arrivedBufList.size());
//                        //if(resendNumber + stop - start <= MAX_RESEND_NUMBER_PER_ROUND_TRIP)
//                        {
//                            sendNextTransForSomeLost(&(*pInfo), start, stop - 1);
//                            //resendNumber += (stop - start);
//                            (*pIter)->arrivedTick = _GetTickCount64() - PDU_ROUND_TRIP / 2;
//                            //(*pIter)->arrivedTick = _GetTickCount64();
//                            (*pIter)->resendTime ++;
//                        }
//                        //else if(resendNumber < MAX_RESEND_NUMBER_PER_ROUND_TRIP)
//                        //{
//                        //	sendNextTransForSomeLost(&(*pInfo), start, start + MAX_RESEND_NUMBER_PER_ROUND_TRIP - resendNumber - 1);
//                        //	resendNumber = MAX_RESEND_NUMBER_PER_ROUND_TRIP;
//                        //	(*pIter)->arrivedTick = _GetTickCount64() - PDU_ROUND_TRIP / 2;
//                        //	(*pIter)->resendTime ++;
//                        //}
//                        //else
//                        //{
//                        //	break;
//                        //}
//                    }
//                    start = stop + 1;
//                }
//                else
//                {
//                    //start = stop + 1;
//
//                    break;
//                }
//            }
//            //sendNextTransTo(&(*cIter));
//            //sendNextTransForSomeLost(&(*cIter), cIter->waitForReplySeq.front(), 0);//for delete
//        }
//
//
//
//        //cIter->consumerTimes ++;
//        //if(cIter->consumerTimes == 50)
//        //{
//        //	cIter->consumerTimes = 0;
//        //	//sendNextTransForSomeLost(&(*cIter), cIter->waitForReplySeq.front(), cIter->waitForReplySeq.front() - 1);//delete only
//        //}
//        if(_GetTickCount64() - deleteNextTick > DELETE_BUFFER_PERIOD && pInfo->waitForReplySeq.size() > 0)
//        {
//            sendNextTransForSomeLost(&(*pInfo), pInfo->waitForReplySeq.front(), pInfo->waitForReplySeq.front() - 1);
//            deleteNextTick = _GetTickCount64();
//        }
//
//        //LeaveCriticalSection(&cIter->lockConsumerThread);
//
//        pduHeader hdr;
//        memset(recvBuf, 0 , 1500);
//        //time(&pInfo->lastPDUTime);
//
//        pInfo->lastPDUTimeTick = _GetTickCount64();
//        memset(&addrClient,0,sizeof(SOCKADDR_IN));
//        int len = recvfrom(pInfo->consumerSrv, recvBuf, 1500, 0, (SOCKADDR*)&addrClient,(socklen_t*)&addrClientLen) ;
//        if(len == SOCKET_ERROR)
//        {
//            messager(msgERROR, "recvfrom error on port %d, error number: %d", globalConfig.publicationPort, myGetLastError());
//            //LeaveCriticalSection(&pInfo->lockConsumerThread);
//            continue;
//        }
//        if(len < sizeof(hdr))
//        {
//            messager(msgERROR, "recv data from %s, data length %d less than %d", inet_ntoa(addrClient.sin_addr), len, sizeof(hdr));
//            //LeaveCriticalSection(&pInfo->lockConsumerThread);
//            continue;
//        }
//        hdr = *((pduHeader *)recvBuf);
//        if(len != hdr.Length)
//        {
//            messager(msgERROR, "recvfrom %s len = %d, hdr.Length = %d, not equal!, msg = %s",inet_ntoa(addrClient.sin_addr), len, hdr.Length, recvBuf);
//            //if(!strcmp(recvBuf, "SEND TO END THE CONSUMER TASK"))
//            //{
//            //	messager(msgINFO, "channle %d recv SEND TO END THE CONSUMER TASK", pInfo->channelID);
//            //	goto endofconsumertask;
//            //}
//            //LeaveCriticalSection(&pInfo->lockConsumerThread);
//            continue;
//        }
//        if(hdr.Type != TRANS_DATA)
//        {
//            messager(msgERROR, "recv %d msg from %s at port %d", hdr.Type, inet_ntoa(addrClient.sin_addr), pInfo->consumerPort);
//            //LeaveCriticalSection(&pInfo->lockConsumerThread);
//            continue;
//        }
//
//
//        time(&pInfo->lastPDUTime);
//
//        pduTransData pData = *(pduTransData *)(recvBuf + sizeof(hdr));
//        if(pData.fileMS > 0)
//        {
//            messager(msgDEBUG, "channel %d recv the last packet of file, fileMS = %d, seq = %d", pData.channelID, pData.fileMS, hdr.seqNumber);
//        }
//
//        if(pData.channelID != pInfo->channelID)
//        {
//            messager(msgERROR, "recv TRANS_DATA of channel %d from %s, myChannel is %d",
//                     pData.channelID,
//                     inet_ntoa(addrClient.sin_addr),
//                     pInfo->channelID);
//            //LeaveCriticalSection(&pInfo->lockConsumerThread);
//            continue;
//        }
//
//        if(lastTsUID > 0 && pData.tsUID != lastTsUID)
//        {
//            if(pData.tsUID > 0)
//            {
//                messager(msgWARN, "TS changed for channel %d", pInfo->channelID);
//                lastTsUID = pData.tsUID;
//            }
//            else
//            {
//                messager(msgWARN, "OUT OF SUPPLIER BUFFER %d", pInfo->channelID);
//            }
//            std::list<pduBuffer *>::iterator pIter;
//            for(pIter = pInfo->arrivedBufList.begin(); pIter != pInfo->arrivedBufList.end(); pIter ++)
//            {
//                delete (*pIter);
//            }
//            pInfo->arrivedBufList.clear();
//            pInfo->waitForReplySeq.clear();
//            pInfo->lastPDUTimeTick = 0;
//            pInfo->recvdFilesNumber = 0;
//        }
//
//        if(pInfo->Status <= ctsNotStarted)
//        {
//            messager(msgDEBUG, "channel %d recv data before connected, seq = %lld", pInfo->channelID, hdr.seqNumber);
//            //LeaveCriticalSection(&pInfo->lockConsumerThread);
//            continue;
//        }
//
//
//
//        //if(pData.fileMS == LINK_ERROR && hdr.seqNumber == 0)
//        //{
//        //	messager(msgERROR, "channel %d link error, informed by %s", pInfo->channelID, inet_ntoa(addrClient.sin_addr));
//        //	pInfo->Status = ctsLinkError;
//        //}
//
//
//        //EnterCriticalSection(&pInfo->lockConsumerThread);
//        //if((fileNum + 1) % 7 > 0)
//        {
//            pInfo->lastPDUTimeTick = _GetTickCount64();
//        }
//        if(pInfo->waitForReplySeq.size() > 1)
//        {
//            messager(msgERROR, "size of waitForReplySeq MUST NOT MORE THAN ONE");
//
//            //LeaveCriticalSection(&pInfo->lockConsumerThread);
//            //LeaveCriticalSection(&pInfo->lockConsumerThread);
//            continue;
//        }
//        else if(pInfo->waitForReplySeq.size() == 0)
//        {
//            pInfo->waitForReplySeq.push_back( hdr.seqNumber + 1);
//            fileStartTime = pInfo->startTime;
//            firstPacketSeqNum = hdr.seqNumber;
//            pInfo->tsUID = pData.tsUID;
//            lastTsUID = pData.tsUID;
//            messager(msgINFO, "recvTheFirstPacket, channelID = %d, seq = %lld", pInfo->channelID, hdr.seqNumber);
//            //			fileNum = pInfo->fileIndex;
//            if(myFile != NULL)
//            {
//                fclose(myFile);
//            }
//
//            if((myFile = fopen(fileName.c_str(), "wb")) == NULL)
//            {
//                messager(msgFATAL, "consumerTaskThreadProc can not open the file %s", fileName.c_str());
//            }
//
//        }
//        else if((*pInfo).waitForReplySeq.size() == 1)
//        {
//            std::list<UINT64>::iterator uIter = pInfo->waitForReplySeq.begin();
//            if(*uIter == hdr.seqNumber)
//            {
//                (*pInfo).waitForReplySeq.pop_front();
//                pInfo->waitForReplySeq.push_back(hdr.seqNumber + 1);
//                //messager(msgDEBUG, "recvTheRightPacket channelID = %d, hdr->seqNumber = %d, *uIter = %d", pInfo->channelID, hdr.seqNumber, *uIter);
//                pInfo->resendTime = 0;
//                std::list<pduBuffer *>::iterator pIter;
//                for(pIter = pInfo->arrivedBufList.begin(); pIter != pInfo->arrivedBufList.end(); pIter ++)
//                {
//                    if((*pIter)->seqNum == hdr.seqNumber)
//                    {
//                        pInfo->arrivedBufList.erase(pIter);
//                        messager(msgWARN, "channel %d memeory leak here", pInfo->channelID);
//                        break;
//                    }
//                }
//            }
//            else if(hdr.seqNumber > *uIter)
//            {
//                //int pduPerPeriod = pInfo->bitRate * TIME_PERIOD_OF_CONSUMER_TASK / (8 * 188 * TS_PDU_LENGTH);
//                //if(pInfo->lastResentStartSeqNum != *uIter)
//                //{
//                //	messager(msgINFO, "PACKET LOST, channelID = %d, hdr->seqNumber = %d, *uIter = %d, aSize = %d", pInfo->channelID, hdr.seqNumber, *uIter, pInfo->arrivedBufList.size());
//                //	sendNextTransForSomeLost(pInfo, *uIter, hdr.seqNumber - 1);
//                //	pInfo->lastResentStartSeqNum = *uIter;
//                //	pInfo->lastResentStopSeqNum = hdr.seqNumber;
//                //	pInfo->resendTime = 1;
//                //}
//                //else if(pInfo->lastResentStartSeqNum == *uIter && pInfo->arrivedBufList.size() > pInfo->resendTime * pduPerPeriod * 2)
//                //{
//                //	messager(msgINFO, "LAST RESEND PACKET LOST, channelID = %d, hdr->seqNumber = %d, *uIter = %d, aSize = %d",
//                //		pInfo->channelID,
//                //		hdr.seqNumber,
//                //		*uIter,
//                //		pInfo->arrivedBufList.size());
//                //	sendNextTransForSomeLost(pInfo, pInfo->lastResentStartSeqNum, pInfo->lastResentStopSeqNum - 1);
//                //	pInfo->resendTime ++;
//                //}
//
//
//                //if(hdr.Length - sizeof(hdr) - sizeof(pduTransData) > 0)
//                {
//                    pduBuffer *p = new pduBuffer;
//                    p->resendTime = 0;
//                    p->seqNum = hdr.seqNumber;
//                    p->length = hdr.Length;
//                    memset(p->buf, 0, 1500);
//                    memcpy(p->buf, recvBuf, hdr.Length);
//                    p->arrivedTick = _GetTickCount64();
//                    std::list<pduBuffer *>::iterator pIter;
//                    BOOL inserted = false;
//                    if(pInfo->arrivedBufList.size() == 0)
//                    {
//                        pInfo->arrivedBufList.push_back(p);
//                    }
//                    else if(p->seqNum > pInfo->arrivedBufList.back()->seqNum)
//                    {
//                        pInfo->arrivedBufList.push_back(p);
//                    }
//                    else
//                    {
//                        for(pIter = pInfo->arrivedBufList.begin(); pIter != pInfo->arrivedBufList.end(); pIter ++)
//                        {
//                            if(p->seqNum < (*pIter)->seqNum)
//                            {
//                                pInfo->arrivedBufList.insert(pIter, p);
//                                inserted = true;
//                                break;
//                            }
//                            else if(p->seqNum == (*pIter)->seqNum)//already recved
//                            {
//                                messager(msgWARN, "RECV PACKET NOT IN ORDER, ALREADY RECVD, channelID = %d, hdr->seqNumber = %lld, *uIter = %d", pInfo->channelID, hdr.seqNumber, *uIter);
//                                inserted = true;
//                                delete p;
//                                break;
//                            }
//                        }
//                    }
//                    //if(!inserted)
//                    //{
//                    //	pInfo->arrivedBufList.push_back(p);
//                    //}
//                }
//                //LeaveCriticalSection(&pInfo->lockConsumerThread);
//                continue;
//            }
//            else if(hdr.seqNumber < *uIter )
//            {
//                messager(msgWARN, "RECV PACKET NOT IN ORDER, ALREADY RECVD, channelID = %d, hdr->seqNumber = %lld, *uIter = %d", pInfo->channelID, hdr.seqNumber, *uIter);
//
//                //LeaveCriticalSection(&pInfo->lockConsumerThread);
//                continue;
//                //sendNextTransForLostPackets(&(*cIter));
//            }
//        }
//
//
//        readByte = hdr.Length - sizeof(pduHeader) - sizeof(pduTransData);
//        if(readByte < 0)
//        {
//            //LeaveCriticalSection(&pInfo->lockConsumerThread);
//            //LeaveCriticalSection(&pInfo->lockConsumerThread);
//            continue;
//        }
//        pInfo->recvBytes += readByte;
//
//        int fileNameLengthFromSupplier;
//        char fileNameFromSupplier[256];
//
//        if(pData.fileMS == 0)
//        {
//            fwrite(recvBuf + sizeof(pduHeader) + sizeof(pduTransData), sizeof(char), readByte, myFile);
//        }
//        else
//        {
//            fileNameLengthFromSupplier = *( (int *) (recvBuf + (hdr.Length - sizeof(int))));
//            //goto endofconsumertask;
//            memset(fileNameFromSupplier, 0, 256);
//            strncpy(fileNameFromSupplier, recvBuf + (hdr.Length - fileNameLengthFromSupplier - sizeof(int)), fileNameLengthFromSupplier);
//            messager(msgDEBUG, "consumer %d fileNameLengthFromSupplier = %d, name = %s", pInfo->channelID, fileNameLengthFromSupplier, fileNameFromSupplier);
//            if(pInfo->fileIndex == 0)
//            {
//                pInfo->fileIndex = getFileIndexFromFile(pInfo->channelName.c_str(), fileNameFromSupplier);
//            }
//            fwrite(recvBuf + sizeof(pduHeader) + sizeof(pduTransData), sizeof(char), readByte - fileNameLengthFromSupplier - sizeof(int), myFile);
//        }
//
//        if( pData.fileMS > 0 )
//        {
//            fclose(myFile);
//            pInfo->recvdFilesNumber ++;
//            //			fileNum ++;
//            //if(fileNum > 9999)
//            //{
//            //	fileNum = 1;
//            //}
//            //char timeStamp[256];
//            //char str[128];
//            //struct tm * timeinfo;
//            //timeinfo = localtime ( &fileStartTime );
//            //strftime(str, 128, "%Y%m%d_%H%M%S", timeinfo);
//            //snprintf(timeStamp, 255, "%3d_%s_%4d_%s_%6d",
//            //			pInfo->channelID,
//            //			pInfo->channelname,
//            //			fileNum,
//            //			str,
//            //			pData.fileMS
//            //			);
//            //for(UINT i = 0; i<strlen(timeStamp); i++)
//            //{
//            //	if(timeStamp[i] == ' ')
//            //		timeStamp[i] = '0';
//            //}
//            newName = filePath +  fileNameFromSupplier;
//            rename(fileName.c_str(), newName.c_str());
//            if(pInfo->recvdFilesNumber == 1)
//            {
//                startOneSupplier(pInfo->channel, firstPacketSeqNum, pData.tsUID, pInfo->fileIndex);
//            }
//            messager(msgINFO, "channel %d file %s finished, size = %d", pInfo->channelID, newName.c_str(), pInfo->arrivedBufList.size());
//
//            pInfo->savedFile.push_back(newName);
//            if(pInfo->savedFile.size() > MAX_FILE_SAVED)
//            {
//                remove(pInfo->savedFile.front().c_str());
//                pInfo->savedFile.pop_front();
//            }
//
//
//            fileTimeMS += pData.fileMS;
//            fileStartTime = pInfo->startTime + fileTimeMS / 1000;
//            //fileStartTime += ( pData.fileMS/1000 );
//            if((myFile = fopen(fileName.c_str(), "wb")) == NULL)
//            {
//                messager(msgFATAL, "consumerTaskThreadProc can not open the file %s", fileName.c_str());
//
//                //LeaveCriticalSection(&pInfo->lockConsumerThread);
//                //return ret;
//                //LeaveCriticalSection(&pInfo->lockConsumerThread);
//                goto endofconsumertask;
//                //break;
//            }
//
//            //sendNextTransForSomeLost(&(*pInfo), pInfo->waitForReplySeq.front(), pInfo->waitForReplySeq.front());
//
//        }
//
//        if(pInfo->arrivedBufList.size() > 0 &&
//           pInfo->waitForReplySeq.size() > 0)
//        {
//            pduBuffer *pIter = pInfo->arrivedBufList.front();
//            UINT64 seq = *(pInfo->waitForReplySeq.begin());
//            pduTransData pData;
//            while(pIter->seqNum == seq)
//            {
//                messager(msgDEBUG, "The next PACKET %d of channel %d has arrived, save it now", seq, pInfo->channelID);
//                readByte = (pIter)->length - sizeof(pduHeader) - sizeof(pduTransData);
//                pData = *(pduTransData *)((pIter)->buf + sizeof(pduHeader));
//                pInfo->recvBytes += readByte;
//                if(readByte > 0)//
//                {
//                    //fwrite((pIter)->buf + sizeof(pduHeader) + sizeof(pduTransData), sizeof(char), readByte, myFile);
//                    if(pData.fileMS == 0)
//                    {
//                        fwrite((pIter)->buf + sizeof(pduHeader) + sizeof(pduTransData), sizeof(char), readByte, myFile);
//                    }
//                    else
//                    {
//                        fileNameLengthFromSupplier = *((int *)((pIter)->buf + ((pIter)->length - sizeof(int))));
//                        messager(msgDEBUG, "consumer %d fileNameLengthFromSupplier = %d", pInfo->channelID, fileNameLengthFromSupplier);
//                        //goto endofconsumertask;
//                        memset(fileNameFromSupplier, 0, 256);
//                        strncpy(fileNameFromSupplier, (pIter)->buf + (pIter)->length - fileNameLengthFromSupplier - sizeof(int), fileNameLengthFromSupplier);
//                        messager(msgDEBUG, "consumer %d fileNameLengthFromSupplier = %d, fileName1 = %s", pInfo->channelID, fileNameLengthFromSupplier, fileNameFromSupplier);
//                        if(pInfo->fileIndex == 0)
//                        {
//                            pInfo->fileIndex = getFileIndexFromFile(pInfo->channelName.c_str(), fileNameFromSupplier);
//                        }
//                        fwrite((pIter)->buf + sizeof(pduHeader) + sizeof(pduTransData), sizeof(char), readByte - fileNameLengthFromSupplier - sizeof(int), myFile);
//                    }
//                }
//                if( pData.fileMS > 0 )
//                {
//                    fclose(myFile);
//                    pInfo->recvdFilesNumber ++;
//                    //					fileNum ++;
//                    //if(fileNum > 9999)
//                    //{
//                    //	fileNum = 1;
//                    //}
//                    //char timeStamp[256];
//                    //char str[128];
//                    //struct tm * timeinfo;
//                    //timeinfo = localtime ( &fileStartTime );
//                    //strftime(str, 128, "%Y%m%d_%H%M%S", timeinfo);
//                    //snprintf(timeStamp, 255, "%3d_%s_%4d_%s_%6d",
//                    //			pInfo->channelID,
//                    //			pInfo->channelname,
//                    //			fileNum,
//                    //			str,
//                    //			pData.fileMS
//                    //			);
//                    //for(UINT i = 0; i<strlen(timeStamp); i++)
//                    //{
//                    //	if(timeStamp[i] == ' ')
//                    //		timeStamp[i] = '0';
//                    //}
//                    //newName = filePath +  timeStamp + ".ts";
//                    newName = filePath +  fileNameFromSupplier;
//                    rename(fileName.c_str(), newName.c_str());
//                    if(pInfo->recvdFilesNumber == 1)
//                    {
//                        startOneSupplier(pInfo->channel, firstPacketSeqNum, pData.tsUID, pInfo->fileIndex);
//                    }
//                    messager(msgINFO, "channel %d file %s finished, size = %d", pInfo->channelID, newName.c_str(), pInfo->arrivedBufList.size());
//
//                    pInfo->savedFile.push_back(newName);
//                    if(pInfo->savedFile.size() > MAX_FILE_SAVED)
//                    {
//                        remove(pInfo->savedFile.front().c_str());
//                        pInfo->savedFile.pop_front();
//                    }
//
//                    fileTimeMS += pData.fileMS;
//                    fileStartTime = pInfo->startTime + fileTimeMS / 1000;
//
//                    //fileStartTime += ( pData.fileMS/1000 );
//                    if((myFile = fopen(fileName.c_str(), "wb")) == NULL)
//                    {
//                        messager(msgFATAL, "consumerTaskThreadProc can not open the file %s", fileName.c_str());
//                        //LeaveCriticalSection(&pInfo->lockConsumerThread);
//                        goto endofconsumertask;
//                        //break;
//                    }
//
//                    //sendNextTransForSomeLost(&(*pInfo), pInfo->waitForReplySeq.front(), pInfo->waitForReplySeq.front());
//
//                }
//                seq++;
//
//                pInfo->waitForReplySeq.pop_front();
//                pInfo->waitForReplySeq.push_back(seq);
//
//                delete (*pInfo->arrivedBufList.begin());
//
//                //EnterCriticalSection(&pInfo->lockConsumerThread);
//                pInfo->arrivedBufList.pop_front();
//                //LeaveCriticalSection(&pInfo->lockConsumerThread);
//
//                if(pInfo->arrivedBufList.size() > 0)
//                {
//                    pIter = pInfo->arrivedBufList.front();
//                }
//                else
//                {
//                    break;
//                }
//            }
//        }
//        //LeaveCriticalSection(&pInfo->lockConsumerThread);
//    }
//    //closesocket(localSendSocket);
//    endofconsumertask:
//    //EnterCriticalSection(&pInfo->lockConsumerThread);
//    messager(msgINFO, "channel %d EXIT CONSUMER TASK THREAD for restart", pInfo->channelID);
//    if(myFile != NULL)
//    {
//        fclose(myFile);
//    }
//
//#if defined(MOHO_X86)
//    close(pInfo->consumerSrv);
//#elif defined(MOHO_WIN32)
//    closesocket(pInfo->consumerSrv);
//#endif
//    std::list<pduBuffer *>::iterator pIter;
//    for(pIter = pInfo->arrivedBufList.begin(); pIter != pInfo->arrivedBufList.end(); pIter ++)
//    {
//        if(*pIter != NULL)
//        {
//            delete (*pIter);
//        }
//    }
//    pInfo->arrivedBufList.clear();
//    pInfo->waitForReplySeq.clear();
//    pInfo->savedFile.clear();
//
//    pInfo->bitRate = 0;
//    pInfo->lastPDUTime = 0;
//    pInfo->pcrPID = 0;
//    pInfo->startTime = 0;
//    pInfo->supplierUID = 0;
//    pInfo->lastSupplierUID = 0;
//    pInfo->Status = ctsTrackerQuery;
//    pInfo->relayRealSupplierUID = 0;
//    //	pInfo->transDataEvent = NULL;
//    //	pInfo->threadHandle = NULL;
//    //  pInfo->tickCount = GetTickCount();
//    pInfo->recvBytes = 0;
//    pInfo->lastResentStartSeqNum = 0;
//    pInfo->lastResentStopSeqNum = 0;
//    pInfo->resendTime = 0;
//    //    pInfo->consumerThread = NULL;
//    //    pInfo->consumerThreadID = 0;
//    //pInfo->ftdsThread = NULL;
//    //pInfo->ftdsThreadID = 0;
//    pInfo->fileIndex = 0;
//    //pInfo->readyForFtds = false;
//    pInfo->consumerTimes = 0;
//    pInfo->lastPDUTimeTick = 0;
//    pInfo->linkNumber = 1;
//    pInfo->linkTwoTimeTick = 0;
//    pInfo->recvdFilesNumber = 0;
//    //    pInfo->consumerThread = NULL;
//    //    pInfo->consumerThreadID = 0;
//    //LeaveCriticalSection(&pInfo->lockConsumerThread);
//#if defined(MOHO_X86)
//    return NULL;
//#elif defined(MOHO_WIN32)
//    return ret;
//#endif
//}

//消费者线程，开启UDP端口，接收流媒体数据，存储，重发
#if defined(MOHO_X86)
void* consumerResendTaskThreadProc_CDN(LPVOID lpParameter)
#elif defined(MOHO_WIN32)
        static DWORD WINAPI consumerResendTaskThreadProc_CDN(LPVOID lpParameter)
#endif
{
#if defined(MOHO_X86)
    signal(SIGUSR1, probeThreadExit);
#endif
    tsConsumerTask *pInfo = (tsConsumerTask *)lpParameter;
    int id = pInfo->channelID;
    while(!pInfo->EXITconsumer)
    {
        UINT totalNumber = 0;
        if(pInfo->Status != ctsStarted)
        {
            mySleep(FTDS_SLEEP_TIME);
            continue;
        }
        pInfo->lockConsumerArrivedDeque.lock();
        if(pInfo->arrivedBufDeque.size() == 0)
        {
        }
        else
        {
            UINT k = 0;
            UINT64 start = 0;
            UINT64 stop = 0;
            pInfo->resendDeque.clear();
            UINT64 curTick = _GetTickCount64();
            pduResendPair pair;
            for(k = 0; k<pInfo->arrivedBufDeque.size(); k++)
            {
                if(pInfo->arrivedBufDeque[k] == NULL)
                {
                    if(start == 0)
                    {
                        start = pInfo->nextPduSeq + k;
                    }
                }
                else
                {
                    if(start > 0 && pInfo->arrivedBufDeque[k] != NULL)
                    {
                        if(curTick - pInfo->arrivedBufDeque[k]->arrivedTick > PDU_ROUND_TRIP)
                        {
                            stop = pInfo->arrivedBufDeque[k]->seqNum;

                            totalNumber += (UINT)(stop - start);
                            if(totalNumber > MAX_RESEND_PACKET_NUMBER)
                            {
                                stop -= (totalNumber - MAX_RESEND_PACKET_NUMBER);
                            }
							UINT64 tmpTick = pInfo->arrivedBufDeque[k]->arrivedTick;
                            pInfo->arrivedBufDeque[k]->arrivedTick = curTick;
                            if(stop > start)
                            {
                                pair.resendStartSeq = start;
                                pair.resendStopSeq = stop - 1;
                                pInfo->resendDeque.push_back(pair);

                                messager(msgINFO, "channel %d: PACKET LOST, from %lld to %lld, total = %lld, aSize = %d, tick = %lld",
                                         pInfo->channelID,
                                         start,
                                         stop - 1,
                                         stop - start,
                                         pInfo->arrivedBufDeque.size(),
                                         curTick - tmpTick);
                            }
                            start = 0;
                            if(totalNumber > MAX_RESEND_PACKET_NUMBER || pInfo->resendDeque.size() >= MAX_RESEND_PAIR_NUMBER)
                            {
								if(totalNumber > MAX_RESEND_PACKET_NUMBER)
								{
									totalNumber = MAX_RESEND_PACKET_NUMBER;
								}
                                break;
                            }
                            //break;
                        }
                        else
                        {
                            start = 0;
                        }
                    }
                    else if(start > 0 && curTick - pInfo->arrivedBufDeque[k]->arrivedTick <= PDU_ROUND_TRIP)
                    {
                        start = 0;
                        //break;
                    }
                }
            }
            if(pInfo->resendDeque.size() > 0)
            {
                messager(msgINFO, "channel %d: PACKET LOST, pairNumber = %d, total = %d, aSize = %d",
                         pInfo->channelID,
                         pInfo->resendDeque.size(),
                         totalNumber,
                         pInfo->arrivedBufDeque.size());
                sendNextTransForSomeLost(&(*pInfo), 2, 3);
            }
        }
        pInfo->lockConsumerArrivedDeque.unlock();
        mySleep(FTDS_SLEEP_TIME * 16);
    }
    messager(msgINFO, "channel %d EXIT CONSUMER RESEnD TASK THREAD for restart", id);
    return 0;
}


//消费者线程，开启UDP端口，接收流媒体数据，存储，重发
#if defined(MOHO_X86)
void* consumerTaskThreadProc_CDN(LPVOID lpParameter)
#elif defined(MOHO_WIN32)
        static DWORD WINAPI consumerTaskThreadProc_CDN(LPVOID lpParameter)
#endif
{
#if defined(MOHO_X86)
    signal(SIGUSR1, probeThreadExit);
#endif

    tsConsumerTask *pInfo = (tsConsumerTask *)lpParameter;
    pInfo->recvdFilesNumber = 0;
    pInfo->nextPduSeq = 0;
    UINT64 firstPacketSeqNum = 1;
    UINT lastTsUID = 0;
    int fileNameLengthFromSupplier;
    char fileNameFromSupplier[256];


    SOCKADDR_IN addrClient;
    memset(&addrClient,0,sizeof(SOCKADDR_IN));
    int addrClientLen = sizeof(addrClient);

    DWORD ret = 0;
    std::string newName;


    std::string filePath = pInfo->channelPath;

    FILE *myFile = NULL;
    std::string fileName = pInfo->channelPath  +  pInfo->channelName + ".ts";

    BYTE buf1[3000];
    char recvBuf[1500];

    int buf1Length = 0;

    int readByte;

    int cursor = 0;


    time_t fileStartTime;
    UINT64 fileTimeMS = 0;
    memset(buf1, 0, 3000);

    UINT64 deleteNextTick = _GetTickCount64();
    UINT64 resendCheckTick = _GetTickCount64();
    initConsumerSavedFiles(pInfo);
    while(!pInfo->EXITconsumer)
    {
		pInfo->lockConsumerArrivedDeque.lock();
        if(_GetTickCount64() - deleteNextTick > DELETE_BUFFER_PERIOD && pInfo->nextPduSeq > 0)
        {
            //pInfo->lockConsumerArrivedDeque.lock();
            //pInfo->resendDeque.clear();
            //pInfo->lockConsumerArrivedDeque.unlock();
            sendNextTransForSomeLost(&(*pInfo), pInfo->nextPduSeq, pInfo->nextPduSeq - 1);
            deleteNextTick = _GetTickCount64();
        }
        pduHeader hdr;
        memset(recvBuf, 0 , 1500);
        pInfo->lastPDUTimeTick = _GetTickCount64();
		pInfo->lockConsumerArrivedDeque.unlock();

        int len = recvfrom(pInfo->consumerSrv, recvBuf, 1500, 0, (SOCKADDR*)&addrClient,(socklen_t*)&addrClientLen) ;	

        pInfo->lockConsumerArrivedDeque.lock();
        if(len == SOCKET_ERROR)
        {
            messager(msgERROR, "recvfrom error on port %d, error number: %d", globalConfig.publicationPort, myGetLastError());
            pInfo->lockConsumerArrivedDeque.unlock();
            continue;
        }
        if(len < sizeof(hdr))
        {
            messager(msgERROR, "recv data from %s, data length %d less than %d", inet_ntoa(addrClient.sin_addr), len, sizeof(hdr));
            pInfo->lockConsumerArrivedDeque.unlock();
            continue;
        }
        hdr = *((pduHeader *)recvBuf);
        if(len != hdr.Length)
        {
            messager(msgERROR, "recvfrom %s len = %d, hdr.Length = %d, not equal!, msg = %s",inet_ntoa(addrClient.sin_addr), len, hdr.Length, recvBuf);
            pInfo->lockConsumerArrivedDeque.unlock();
            continue;
        }
        if(hdr.Type != TRANS_DATA)
        {
            messager(msgERROR, "recv msg 0x%x  from %s at port %d", hdr.Type, inet_ntoa(addrClient.sin_addr), pInfo->consumerPort);
            switch(hdr.Type)
            {
            case HOW_ARE_YOU:
                {
                    pduHowAreYou pHow = *(pduHowAreYou *)(recvBuf + sizeof(hdr));
                    messager(msgINFO, "recv HOW_ARE_YOU from %s:%d, pHow->UID = %d, pHeader->UID = %d, SET to dataTransferAddr",
                             inet_ntoa(addrClient.sin_addr),
                             ntohs(addrClient.sin_port),
                             pHow.myUID,
                             hdr.myUID);
                    //updateNeighborList(&pHow, &addrClient);

                    replyTo(&hdr, &pHow, &addrClient, &(pInfo->consumerSrv));
                    pInfo->setDataTransferAddr(hdr.myUID, &addrClient);
                    break;
                }
            case LIVECDN_REPLY_CHANNEL_QUERY:
                {
                    pduReplyQuery pRQ;
                    pRQ = *(pduReplyQuery *)(recvBuf + sizeof(hdr));
                    std::string from;
                    std::string internetAddr;
                    from = inet_ntoa(addrClient.sin_addr);
                    internetAddr = inet_ntoa(pRQ.queryNode.internetAddr.sin_addr);
                    messager(msgINFO, "recv LIVECDN_REPLY_CHANNEL_QUERY from %s, channel = %d, number = %d, queryNode(%s, %d)",
                             from.c_str(),
                             pRQ.channelID,
                             pRQ.supplierNumber,
                             internetAddr.c_str(),
                             ntohs(pRQ.queryNode.internetAddr.sin_port));
                    break;
                }
            default:
                {
                    break;
                }
            }
            pInfo->lockConsumerArrivedDeque.unlock();
            continue;
        }


        time(&pInfo->lastPDUTime);

        pduTransData pData = *(pduTransData *)(recvBuf + sizeof(hdr));
        if(pData.fileMS > 0)
        {
            messager(msgDEBUG, "channel %d recv the last packet of file, fileMS = %x__%d, seq = %lld", pData.channelID, pData.fileMS,pData.fileMS, hdr.seqNumber);
        }

        if(pData.channelID != pInfo->channelID)
        {
            messager(msgERROR, "recv TRANS_DATA of channel %d from %s, myChannel is %d",
                     pData.channelID,
                     inet_ntoa(addrClient.sin_addr),
                     pInfo->channelID);
            pInfo->lockConsumerArrivedDeque.unlock();
            continue;
        }

        if(lastTsUID > 0 && pData.tsUID != lastTsUID)
        {
            messager(msgWARN, "TS changed for channel %d", pInfo->channelID);
            lastTsUID = pData.tsUID;
            std::deque<pduBuffer *>::iterator pIter;
            for(pIter = pInfo->arrivedBufDeque.begin(); pIter != pInfo->arrivedBufDeque.end(); pIter ++)
            {
				if(*pIter != NULL)
				{
					delete (*pIter);
				}
            }
            pInfo->arrivedBufDeque.clear();
            pInfo->lastPDUTimeTick = 0;
            pInfo->recvdFilesNumber = 0;
            pInfo->nextPduSeq = 0;
        }

        if(pInfo->Status <= ctsNotStarted)
        {
            messager(msgINFO, "channel %d recv data before connected, seq = %lld,pInfo->nextPduSeq = %lld", pInfo->channelID, hdr.seqNumber,pInfo->nextPduSeq);
            pInfo->lockConsumerArrivedDeque.unlock();
            continue;
        }

        pInfo->lastPDUTimeTick = _GetTickCount64();
        if(pInfo->nextPduSeq == 0)
        {
            pInfo->nextPduSeq = hdr.seqNumber;
            fileStartTime = pInfo->startTime;
            firstPacketSeqNum = hdr.seqNumber;
            pInfo->tsUID = pData.tsUID;
            lastTsUID = pData.tsUID;
            messager(msgINFO, "recvTheFirstPacket, channelID = %d, seq = %lld", pInfo->channelID, hdr.seqNumber);
            if(myFile != NULL)
            {
                fclose(myFile);
            }

            if((myFile = fopen(fileName.c_str(), "wb")) == NULL)
            {
                messager(msgFATAL, "consumerTaskThreadProc can not open the file %s", fileName.c_str());
            }
        }
        if(pInfo->nextPduSeq > 0)
        {
            if(hdr.seqNumber == pInfo->nextPduSeq)
            {
                readByte = hdr.Length - sizeof(pduHeader) - sizeof(pduTransData);
                if(pData.fileMS == 0)
                {
                    fwrite(recvBuf + sizeof(pduHeader) + sizeof(pduTransData), sizeof(char), readByte, myFile);
                }
                else
                {
                    fileNameLengthFromSupplier = pData.fileMS;

                    //#ifdef _IS_LITTLE_ENDIAN
                    //                    char*p = recvBuf + (hdr.Length - sizeof(int));
                    //                    fileNameLengthFromSupplier = p[3] << 4 | p[2] << 3 | p[1] << 2 | p[0];
                    //
                    //#else
                    //                    fileNameLengthFromSupplier = *( (int *)(recvBuf + (hdr.Length - sizeof(int))));
                    //#endif

//                    if(hdr.Length - fileNameLengthFromSupplier <= 0)
//                    {
//                        for(int i =0 ;i<len;i++)
//                        {
//                            printf("%x__",recvBuf[i]);
//                        }
//                        printf("\n__len2=%d__%d\n",hdr.Length,fileNameLengthFromSupplier);
//                    }
//                    else if(fileNameLengthFromSupplier <= 0)
//                    {
//                        printf("fileNameLengthFromSupplier is invalid,len222=%d__%d\n",hdr.Length,fileNameLengthFromSupplier);
//                    }

                    memset(fileNameFromSupplier, 0, 256);

                    mysprintf(fileNameFromSupplier,256, recvBuf + (hdr.Length - fileNameLengthFromSupplier), fileNameLengthFromSupplier);
                   // strncpy(fileNameFromSupplier, recvBuf + (hdr.Length - fileNameLengthFromSupplier - sizeof(int)), fileNameLengthFromSupplier);

                    messager(msgDEBUG, "consumer %d fileNameLengthFromSupplier = %d, hdr.Length = %d__%d, name = %s", pInfo->channelID, fileNameLengthFromSupplier, hdr.Length,readByte,fileNameFromSupplier);

                    if(pInfo->fileIndex == 0)
                    {
                        pInfo->fileIndex = getFileIndexFromFile(pInfo->channelName.c_str(), fileNameFromSupplier);
                    }
                    fwrite(recvBuf + sizeof(pduHeader) + sizeof(pduTransData), sizeof(char), readByte - fileNameLengthFromSupplier, myFile);
                 //   fwrite(recvBuf + sizeof(pduHeader) + sizeof(pduTransData), sizeof(char), readByte - fileNameLengthFromSupplier - sizeof(int), myFile);
                    fclose(myFile);
                    newName = filePath +  fileNameFromSupplier;
                    rename(fileName.c_str(), newName.c_str());
                    pInfo->recvdFilesNumber ++;
                    if(pInfo->recvdFilesNumber == 2)
                    {
                        pInfo->channel->needToRestartConsumer = TRUE;
                    }
					if(pInfo->recvdFilesNumber == SUPPLIER_START_NUMBER)
					{
                        pInfo->channel->needToRestartSupplier = TRUE;
                        startOneSupplier(pInfo->channel, firstPacketSeqNum, pData.tsUID, pInfo->fileIndex);
					}
                    messager(msgINFO, "channel %d file %d finished, sizeDeque = %d",
                             pInfo->channelID,
                             getFileIndexFromFile(pInfo->channelName.c_str(), fileNameFromSupplier),
                             pInfo->arrivedBufDeque.size());
                    pInfo->savedFile.push_back(newName);
                    if(pInfo->savedFile.size() > MAX_FILE_SAVED)
                    {
                        remove(pInfo->savedFile.front().c_str());
                        pInfo->savedFile.pop_front();
                    }

                    if((myFile = fopen(fileName.c_str(), "wb")) == NULL)
                    {
                        messager(msgFATAL, "consumerTaskThreadProc can not open the file %s", fileName.c_str());
						pInfo->lockConsumerArrivedDeque.unlock();
                        goto endofconsumertask;
                    }
                }
                if(pInfo->arrivedBufDeque.size() > 0)
                {
                    if(pInfo->arrivedBufDeque[0] != NULL)
                    {
                        messager(msgWARN, "LIKE IMPOSSIBLE");
                        if(pInfo->arrivedBufDeque[0]->seqNum == hdr.seqNumber)
                        {
                            pduBuffer *pIter = pInfo->arrivedBufDeque[0];
                            delete pIter;
                            pInfo->arrivedBufDeque.erase(pInfo->arrivedBufDeque.begin());
                        }
                        else if(pInfo->arrivedBufDeque[0]->seqNum < hdr.seqNumber)
                        {
                            messager(msgFATAL, "IMPOSSIBLE");
                        }
                        else
                        {
                        }
                    }
                    else
                    {
                        pInfo->arrivedBufDeque.erase(pInfo->arrivedBufDeque.begin());
                    }
                }
                pInfo->nextPduSeq ++;
                //continue;
            }
            else if(hdr.seqNumber > pInfo->nextPduSeq)
            {
                UINT64 i = 0;
                UINT32 dequeSize = pInfo->arrivedBufDeque.size();
                pduBuffer *p = NULL;
                UINT aSize = pInfo->arrivedBufDeque.size();
                for(i = pInfo->nextPduSeq + aSize; i<=hdr.seqNumber; i++)
                {
                    pInfo->arrivedBufDeque.push_back(p);
                }
                UINT32 bufIndex = (UINT32)(hdr.seqNumber - pInfo->nextPduSeq);
                p = pInfo->arrivedBufDeque[bufIndex];
                if(p == NULL)
                {
                    p = new pduBuffer;
                    p->resendTime = 0;
                    p->seqNum = hdr.seqNumber;
                    p->length = hdr.Length;
                    p->arrivedTick = _GetTickCount64();
                    memset(p->buf, 0, 1500);
                    memcpy(p->buf, recvBuf, hdr.Length);
                    pInfo->arrivedBufDeque[bufIndex] = p;
                }
                else
                {
                    messager(msgWARN, "channel %d seq %lld recvd more than one time", pInfo->channelID, hdr.seqNumber);
                }
            }
            else if(hdr.seqNumber < pInfo->nextPduSeq - 1)
            {
                messager(msgWARN, "RECV PACKET NOT IN ORDER, ALREADY RECVD, channelID = %d, hdr->seqNumber = %lld, nextPduSeq = %lld,pInfo->Status__%d", pInfo->channelID, hdr.seqNumber, pInfo->nextPduSeq,pInfo->Status);
                pInfo->lockConsumerArrivedDeque.unlock();
                continue;
            }
        }

        UINT32 j = 0;
        for(j = 0; j<pInfo->arrivedBufDeque.size(); j++)
        {
            pduBuffer *pIter = pInfo->arrivedBufDeque[j];
            if(pIter == NULL)
            {
                break;
            }
            pInfo->nextPduSeq ++;
            pduTransData pData;
            //messager(msgINFO, "The next PACKET %d of channel %d has arrived, save it now", pIter->seqNum, pInfo->channelID);
            readByte = (pIter)->length - sizeof(pduHeader) - sizeof(pduTransData);
            pData = *(pduTransData *)((pIter)->buf + sizeof(pduHeader));
            pInfo->recvBytes += readByte;
            if(pData.fileMS == 0)
            {
                fwrite((pIter)->buf + sizeof(pduHeader) + sizeof(pduTransData), sizeof(char), readByte, myFile);
            }
            else
            {
                fileNameLengthFromSupplier = pData.fileMS;

                //#ifdef _IS_LITTLE_ENDIAN
                //                char*p = (pIter)->buf + ((pIter)->length - sizeof(int));
                //                fileNameLengthFromSupplier = p[3] << 4 | p[2] << 3 | p[1] << 2 | p[0];
                //#else
                //                fileNameLengthFromSupplier = *((int *)((pIter)->buf + ((pIter)->length - sizeof(int))));
                //#endif

//                if(hdr.Length - fileNameLengthFromSupplier <= 0)
//                {
//                    for(int i =0 ;i<len;i++)
//                    {
//                        printf("%x__",pIter->buf[i]);
//                    }
//                    printf("\n__len1=%d__%d\n",pIter->length,fileNameLengthFromSupplier);
//                }
//                else if(fileNameLengthFromSupplier <= 0)
//                {
//                    printf("fileNameLengthFromSupplier is invalid,len111=%d__%d\n",pIter->length,fileNameLengthFromSupplier);
//                }
                memset(fileNameFromSupplier, 0, 256);

                mysprintf(fileNameFromSupplier,256, pIter->buf + pIter->length - fileNameLengthFromSupplier, fileNameLengthFromSupplier);
             //   strncpy(fileNameFromSupplier, (pIter)->buf + (pIter)->length - fileNameLengthFromSupplier - sizeof(int), fileNameLengthFromSupplier);

                messager(msgDEBUG, "consumer %d fileNameLengthFromSupplier = %d, fileName1 = %s", pInfo->channelID, fileNameLengthFromSupplier, fileNameFromSupplier);
                if(pInfo->fileIndex == 0)
                {
                    pInfo->fileIndex = getFileIndexFromFile(pInfo->channelName.c_str(), fileNameFromSupplier);
                }
                fwrite((pIter)->buf + sizeof(pduHeader) + sizeof(pduTransData), sizeof(char), readByte - fileNameLengthFromSupplier, myFile);
               // fwrite((pIter)->buf + sizeof(pduHeader) + sizeof(pduTransData), sizeof(char), readByte - fileNameLengthFromSupplier - sizeof(int), myFile);
                fclose(myFile);
                newName = filePath +  fileNameFromSupplier;
                rename(fileName.c_str(), newName.c_str());

                pInfo->recvdFilesNumber ++;
                if(pInfo->recvdFilesNumber == 2)
                {
                    pInfo->channel->needToRestartConsumer = TRUE;
                }
				if(pInfo->recvdFilesNumber == SUPPLIER_START_NUMBER)
				{
                    pInfo->channel->needToRestartSupplier = TRUE;
                    startOneSupplier(pInfo->channel, firstPacketSeqNum, pData.tsUID, pInfo->fileIndex);
				}
                messager(msgINFO, "channel %d file %d finished, size = %d",
                         pInfo->channelID,
                         getFileIndexFromFile(pInfo->channelName.c_str(), fileNameFromSupplier),
                         pInfo->arrivedBufDeque.size());
                pInfo->savedFile.push_back(newName);
                if(pInfo->savedFile.size() > MAX_FILE_SAVED)
                {
                    remove(pInfo->savedFile.front().c_str());
                    pInfo->savedFile.pop_front();
                }
                if((myFile = fopen(fileName.c_str(), "wb")) == NULL)
                {
                    messager(msgFATAL, "consumerTaskThreadProc can not open the file %s", fileName.c_str());
                    pInfo->lockConsumerArrivedDeque.unlock();
                    goto endofconsumertask;
                }
            }
            delete pIter;
        }
        if(j > 0)
        {
            pInfo->arrivedBufDeque.erase(pInfo->arrivedBufDeque.begin(), pInfo->arrivedBufDeque.begin() + j);
        }
        pInfo->lockConsumerArrivedDeque.unlock();
    }
    endofconsumertask:
    messager(msgINFO, "channel %d EXIT CONSUMER TASK THREAD for restart", pInfo->channelID);
    if(myFile != NULL)
    {
        fclose(myFile);
    }
#if defined(MOHO_X86)
    close(pInfo->consumerSrv);
#elif defined(MOHO_WIN32)
    closesocket(pInfo->consumerSrv);
#endif

    pInfo->lockConsumerArrivedDeque.lock();
    std::deque<pduBuffer *>::iterator pIter;
    for(pIter = pInfo->arrivedBufDeque.begin(); pIter != pInfo->arrivedBufDeque.end(); pIter ++)
    {
        if(*pIter != NULL)
        {
            delete (*pIter);
        }
    }
    pInfo->arrivedBufDeque.clear();
    pInfo->lockConsumerArrivedDeque.unlock();

    pInfo->waitForReplySeq.clear();
    pInfo->savedFile.clear();

    pInfo->bitRate = 0;
    pInfo->lastPDUTime = 0;
    pInfo->pcrPID = 0;
    pInfo->startTime = 0;
    pInfo->supplierUID = 0;
    pInfo->lastSupplierUID = 0;
    pInfo->Status = ctsTrackerQuery;
    pInfo->relayRealSupplierUID = 0;
    pInfo->recvBytes = 0;
    pInfo->lastResentStartSeqNum = 0;
    pInfo->lastResentStopSeqNum = 0;
    pInfo->resendTime = 0;
    pInfo->fileIndex = 0;
    pInfo->consumerTimes = 0;
    pInfo->lastPDUTimeTick = 0;
    pInfo->linkNumber = 1;
    pInfo->linkTwoTimeTick = 0;
    pInfo->recvdFilesNumber = 0;
    pInfo->lockConsumerArrivedDeque.destroy();
#if defined(MOHO_X86)
    return NULL;
#elif defined(MOHO_WIN32)
    return ret;
#endif
}


//从提供者收到开始传送应答报文后，更新消费者状态，并启动向FTDS发包的线程
int updateConsumerTaskA(pduHeader *ppHeader, pduStartTrans *ppStart, SOCKADDR_IN *pAddrClient)
{
    int ret = -1;
    std::list<tsConsumerTask>::iterator cIter;


    //EnterCriticalSection(&lockTsConsumerTaskList);
    for(cIter = globalTsConsumerTaskList.begin(); cIter != globalTsConsumerTaskList.end(); cIter ++)
    {
        //EnterCriticalSection(&cIter->lockConsumerThread);
        if((*cIter).supplierUID == ppHeader->myUID &&
           (*cIter).channelID == ppStart->channelID &&
           cIter->ftdsThread.isCreat() == 0)
        {
            cIter->consumerThread.terminate();
            cIter->EXITconsumer = false;
            addNewConsumer(&(*cIter));

            cIter->ftdsThread.terminate();
            cIter->EXITftds = false;

            cIter->ftdsThread.creat(ftdsThreadProc1,(void*)&(*cIter));

            cIter->Status = ctsStarted;
            ret = 0;
            break;
        }
        else if((*cIter).supplierUID == ppHeader->myUID &&
                (*cIter).channelID == ppStart->channelID)
        {
            cIter->Status = ctsStarted;
            break;
        }
        //LeaveCriticalSection(&cIter->lockConsumerThread);
    }
    //LeaveCriticalSection(&lockTsConsumerTaskList);


    return ret;
}



/*
static DWORD WINAPI supplierTaskThreadProc3(LPVOID lpParameter)
{
	DWORD ret = 0;
	tsSupplierTask *pSupplierTask = (tsSupplierTask *)lpParameter;

	SOCKET localSendSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	FILE *fp = NULL;
	
	int filePosition;
	int fileSize;
	_finddata_t file; 
	long lf; 
	char *filespec = new char[256];
	char *buf = new char[1500];
	char *sendBuf = new char[1500];
	pduHeader pHeader;
	pduTransData pData;
	int readLen ;
	int len;
	int tolen;
	int pduNumber = 0;
	int tmpLen;
	int fileLength = 0;
	int sendLength = 0;
	int freadLength = 0;
	UINT fileMS = 0;

	BYTE buf1[3000];
	int buf1Length = 0;
	INT64 totalPCR = 0;
	INT64 currPCR = 0;
	INT64 lastPCR = 0;
	DWORD tmpTick;

	tolen = sizeof(pSupplierTask->consumerIP);
	pSupplierTask->consumerIP.sin_port = htons(pSupplierTask->consumerPort);

	DWORD tick = GetTickCount();

	//pduBuffer *localBuf[1000];
	//for(int k = 0; k<1000; k++)
	//{
	//	localBuf[k] = new pduBuffer;
	//}

	int offset = 0;
	while(true)
	{
		DWORD dwWaitResult;
		dwWaitResult = WaitForSingleObject( 
			pSupplierTask->nextTransEvent,		// event handle
			INFINITE);							// indefinite wait
		switch (dwWaitResult) 
		{
			case WAIT_OBJECT_0: 
			{
				ResetEvent(pSupplierTask->nextTransEvent);
				EnterCriticalSection(&pSupplierTask->lockSupplierThread);
				messager(msgDEBUG, "INTO supplier task");
				std::list<pduNextTrans>::iterator nIter;
				for(nIter = pSupplierTask->waitNextTrans.begin(); nIter != pSupplierTask->waitNextTrans.end(); nIter ++)
				{
					(*pSupplierTask).lastIndex = (*nIter).lastIndex;
					(*pSupplierTask).lastRecvBytes = (*nIter).nextByte;
					pSupplierTask->resendStartSeq = (*nIter).resendStartSeq;
					pSupplierTask->resendStopSeq = (*nIter).resendStopSeq;

					if(pSupplierTask->resendStartSeq > 0 && pSupplierTask->resendStopSeq > 0)
					{
						messager(msgDEBUG, "channel %d recv resend of %d - %d, total = %d", 
							pSupplierTask->channelID,
							pSupplierTask->resendStartSeq, 
							pSupplierTask->resendStopSeq,
							pSupplierTask->resendStopSeq - pSupplierTask->resendStartSeq + 1);
						UINT i = pSupplierTask->resendStartSeq;
						bool found = true;
						
						
						if(pSupplierTask->resendStartSeq < pSupplierTask->sentBufQue[offset]->seqNum)
						{
							messager(msgERROR, "channel %d can not find the lost packet of seq %d,EXIT THREAD",pSupplierTask->channelID, pSupplierTask->resendStartSeq);
							LeaveCriticalSection(&pSupplierTask->lockSupplierThread);
							pSupplierTask->streamSend = false;
							goto endofresendproc;
						}

						int queIndex = pSupplierTask->resendStartSeq - pSupplierTask->sentBufQue[offset]->seqNum;
						for(i = pSupplierTask->resendStartSeq; i<= pSupplierTask->resendStopSeq; i++)
						{
							pHeader.Type = TRANS_DATA;
							pHeader.myUID = globalConfig.UID;
							pHeader.seqNumber = i;
							pHeader.Length = sizeof(pHeader) + sizeof(pData) + pSupplierTask->sentBufQue[queIndex + offset]->length;
							pData.channelID = pSupplierTask->channelID;
							pData.fileMS = pSupplierTask->sentBufQue[queIndex + offset]->fileMS;
							memcpy(sendBuf, &pHeader, sizeof(pHeader));
							memcpy(sendBuf + sizeof(pHeader), &pData, sizeof(pData));
							memcpy(sendBuf + sizeof(pHeader) + sizeof(pData), pSupplierTask->sentBufQue[queIndex + offset]->buf, pSupplierTask->sentBufQue[queIndex + offset]->length);

							len = sendto(localSendSocket, sendBuf, pHeader.Length, 0, (SOCKADDR *)(&(pSupplierTask->consumerIP)), tolen) ;
							if(len == SOCKET_ERROR)
							{
                                                                messager(msgERROR, _T("send to failed, dest ip = %s, msgtype = %d, error number = %d"), inet_ntoa(pSupplierTask->consumerIP.sin_addr), pHeader.Type, myGetLastError());
							}
							else if(len != pHeader.Length)
							{
								messager(msgERROR, _T("send to error, dest ip = %s, msgtype = %d, msglen = %d, sendlen = %d"), inet_ntoa(pSupplierTask->consumerIP.sin_addr), pHeader.Type, pHeader.Length, len);
							}
							else
							{
								messager(msgDEBUG, "resend TRANS_DATA of seqNum %d to %s, channelID = %d, dataLen = %d", 
									(pSupplierTask->sentBufQue[queIndex + offset])->seqNum,
									inet_ntoa(pSupplierTask->consumerIP.sin_addr),
									pData.channelID,
									(pSupplierTask->sentBufQue[queIndex + offset])->length);
								timeBeginPeriod(SUPPLIER_SEND_SLEEP_TIME);
								Sleep(SUPPLIER_SEND_SLEEP_TIME);
								timeEndPeriod(SUPPLIER_SEND_SLEEP_TIME);
							}
							queIndex ++;
						}
						if(pSupplierTask->lastIndex == NEXT_TRANS_LAST_INDEX_FOR_DELETE &&
							pSupplierTask->resendStartSeq - pSupplierTask->sentBufQue[offset]->seqNum > 1000)
						{
							//EnterCriticalSection(&pSupplierTask->lockSentBufQue);
							for(i = pSupplierTask->sentBufQue[offset]->seqNum; i< pSupplierTask->resendStartSeq; i++)
							{
                                                                //delete (*pSupplierTask->sentBufQue->begin());
								delete (pSupplierTask->sentBufQue[offset]);
								offset ++;
                                                                //pSupplierTask->sentBufQue->pop_front();
							}
							//LeaveCriticalSection(&pSupplierTask->lockSentBufQue);
						}
						if(offset > 1000000)
						{
							messager(msgINFO, "DELETE PDU BUFFER POINT, channelID = %d", pSupplierTask->channelID);
							EnterCriticalSection(&pSupplierTask->lockSentBufQue);
                                                        pSupplierTask->sentBufQue->erase(pSupplierTask->sentBufQue->begin(), pSupplierTask->sentBufQue->begin() + offset);
							LeaveCriticalSection(&pSupplierTask->lockSentBufQue);
							offset = 0;
						}
					}//if(pSupplierTask->resendStartSeq > 0 && pSupplierTask->resendStopSeq > 0)
				}//for(nIter = pSupplierTask->waitNextTrans.begin(); nIter != pSupplierTask->waitNextTrans.end(); nIter ++)
				pSupplierTask->waitNextTrans.clear();
				LeaveCriticalSection(&pSupplierTask->lockSupplierThread);
			}//case WAIT_OBJECT_0: 
		}//switch (dwWaitResult) 
	}//while(true)

endofresendproc:
	delete filespec;
	delete sendBuf;
	delete buf;

        for(int i = 0; i<pSupplierTask->sentBufQue->size(); i++)
	{
                delete (*pSupplierTask->sentBufQue->begin());
		EnterCriticalSection(&pSupplierTask->lockSentBufQue);
                pSupplierTask->sentBufQue->pop_front();
		LeaveCriticalSection(&pSupplierTask->lockSentBufQue);
	}
	return ret;
}
*/


int getIDFromFile(const char *channelName, char *fileName)
{
    char index[20];
    //strncpy(index, file.name + 3 + 2 + strlen(channelname), 4);

    UINT j = 3 + strlen(channelName) + 8 + 6 + 6 + 5;
    UINT jIndex = 0;
    while(fileName[j] != '_' && j < strlen(fileName))
    {
        index[jIndex] = fileName[j];
        jIndex ++;
        j ++;
    }
    index[jIndex] = '\0';
    return atoi(index);
}

#if defined(MOHO_X86)
void* supplierResendThreadProc(LPVOID lpParameter)
#elif defined(MOHO_WIN32)
        static DWORD WINAPI supplierResendThreadProc(LPVOID lpParameter)
#endif
{
#if defined(MOHO_X86)
    signal(SIGUSR1, probeThreadExit);
#endif


    DWORD ret = 0;

    tsResendTaskPara *pResendTask = (tsResendTaskPara *)lpParameter;
    tsSupplierTask *pSupplierTask = (tsSupplierTask *)pResendTask->task;
    tsConsumer *pConsumer = (tsConsumer *)pResendTask->consumer;

    messager(msgINFO, "start thread supplierResendThreadProc");

	
	//SOCKET localSendSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    pduBuffer buf;
    char sendBuf[1500];
    pduHeader pHeader;
    pduTransData pData;
    int tolen;
    tolen = sizeof(pConsumer->ipAddr);
    //pSupplierTask->consumerIP.sin_port = htons(pSupplierTask->consumerPort);

    int sendNum = 0;
    UINT64 tmpTick = _GetTickCount64();
    int len = 0;

    UINT64 errorTick = _GetTickCount64();

    std::deque<nextTransWithPair> localNextTrans;

    //   BYTE buf1[3000];
    //   int buf1Length = 0;
    //   INT64 totalPCR = 0;
    //   INT64 currPCR = 0;
    //   INT64 lastPCR = 0;
    //UINT64 tick;
    //UINT64 tmpSleepTick;

    while(!pSupplierTask->EXITsupplier)
    {
		int sendNumber = 0;
        if(_GetTickCount64() - tmpTick > DELETE_BUFFER_PERIOD*5)//last 10 second recv no nexttrans
        {
            messager(msgINFO, "consumer %s UID %d no longer exist", inet_ntoa(pConsumer->ipAddr.sin_addr), pConsumer->UID);
            break;
        }
        pConsumer->lockConsumerNextTransDeque.lock();
        localNextTrans.clear();
        localNextTrans = pConsumer->nextTrans;
        pConsumer->nextTrans.clear();
        pConsumer->lockConsumerNextTransDeque.unlock();

        std::deque<nextTransWithPair>::iterator nIter;
        for(UINT j = 0; j<localNextTrans.size(); j++)
        {
            if(pSupplierTask->sentBufQue->size() == 0)
            {
                break;
            }

            nIter =  localNextTrans.begin() + j;
            if(nIter->waitNextTrans.tsUID != pSupplierTask->tsUID)
            {
                continue;
            }
            messager(msgINFO, "UID %d : channel %d resend pair number = %d", pConsumer->UID,pSupplierTask->channelID, nIter->resendDeque.size());
            for(UINT kk = 0; kk<nIter->resendDeque.size(); kk ++)
            {
                messager(msgINFO, "UID %d : channel %d RESEND from %lld to %lld, total = %d",
                         pConsumer->UID,
                         pSupplierTask->channelID,
                         nIter->resendDeque[kk].resendStartSeq,
                         nIter->resendDeque[kk].resendStopSeq,
                         (UINT)(nIter->resendDeque[kk].resendStopSeq - nIter->resendDeque[kk].resendStartSeq + 1));
                if(nIter->resendDeque[kk].resendStartSeq > 0 && nIter->resendDeque[kk].resendStopSeq > 0)
                {
                    //buf1Length = 0;
                    //totalPCR = 0;
                    //currPCR = 0;
                    //lastPCR = 0;
                    //tick = 0;
                    UINT64 i = nIter->resendDeque[kk].resendStartSeq;
                    UINT queIndex;
                    for(i =  nIter->resendDeque[kk].resendStartSeq; i<=  nIter->resendDeque[kk].resendStopSeq; i++)
                    {
                        if(pSupplierTask->sentBufQue->size() == 0)
                        {
                            break;
                        }
                        pSupplierTask->lockSentBufQue.lock();
                        if( i >= pSupplierTask->sentBufQue->at(0)->seqNum && i<= pSupplierTask->sentBufQue->back()->seqNum)
                        {
                            queIndex = (UINT)((i - pSupplierTask->sentBufQue->at(0)->seqNum));
                        }
                        else
                        {
                            messager(msgERROR, "supplier for consumer %s out of buffer range, SHOULD RESTART CONSUMER now",
                                     inet_ntoa(pConsumer->ipAddr.sin_addr));
                            pSupplierTask->lockSentBufQue.unlock();
                            goto outofresendsupplier;
                        }
                        memcpy(&buf, pSupplierTask->sentBufQue->at(queIndex), sizeof(pduBuffer));
                        pSupplierTask->lockSentBufQue.unlock();


                        pHeader.Type = TRANS_DATA;
                        pHeader.myUID = globalConfig.UID;
                        pHeader.seqNumber = i;
                        pHeader.Length = sizeof(pHeader) + sizeof(pData) + buf.length;
                        pData.channelID = pSupplierTask->channelID;
                        pData.tsUID = pSupplierTask->tsUID;
                        pData.fileMS = buf.fileMS;
                        memcpy(sendBuf, &pHeader, sizeof(pHeader));
                        memcpy(sendBuf + sizeof(pHeader), &pData, sizeof(pData));
                        memcpy(sendBuf + sizeof(pHeader) + sizeof(pData), buf.buf, buf.length);
                        len = sendto(*(Info.sendSocket), sendBuf, pHeader.Length, 0, (SOCKADDR *)(&(pConsumer->ipAddr)), tolen) ;
                        if(len == SOCKET_ERROR)
                        {
                            messager(msgERROR, "send to failed, dest ip = %s, msgtype = %d, error number = %d", inet_ntoa(pConsumer->ipAddr.sin_addr), pHeader.Type, myGetLastError());
                            errorTick = _GetTickCount64();
                        }
                        else if(len != pHeader.Length)
                        {
                            messager(msgERROR, "send to error, dest ip = %s, msgtype = %d, msglen = %d, sendlen = %d", inet_ntoa(pConsumer->ipAddr.sin_addr), pHeader.Type, pHeader.Length, len);
                        }
                        else
                        {
							sendNumber ++;
							if(sendNumber % 5 == 0)
							{
								mySleep(FTDS_SLEEP_TIME);
							}
                            //memcpy(buf1 + buf1Length, buf.buf, buf.length);
                            //buf1Length += buf.length;

                            //int cursor = 0;
                            //while(buf1Length >= 188)
                            //{
                            //	if(buf1[cursor] == 0x47)
                            //	{
                            //		BYTE *inBuf = (BYTE *)(buf1 + cursor);
                            //		if(inBuf[0] == 0x47 &&
                            //		   (inBuf[1] & 0x1f) * 256 + inBuf[2] == pSupplierTask->channel->pcrPID &&
                            //		   inBuf[3] & 0x20 &&
                            //		   inBuf[4] >= 0x07 &&
                            //		   inBuf[5] & 0x10)
                            //		{
                            //			lastPCR = currPCR;
                            //			currPCR = (inBuf[6] * 1677216 + inBuf[7] * 65536 + inBuf[8] * 256 + inBuf[9] ) * 2;
                            //			if(inBuf[10] & 0x80)
                            //			{
                            //				currPCR ++;
                            //			}
                            //			if(lastPCR != 0 && currPCR > lastPCR && currPCR - lastPCR < MAX_PCR_PERIOD_MS * 90)
                            //			{
                            //				totalPCR += (currPCR - lastPCR);
                            //			}
                            //		}
                            //		cursor += 188;
                            //		buf1Length -= 188;
                            //	}
                            //	else
                            //	{
                            //		cursor ++;
                            //		buf1Length --;
                            //	}
                            //}
                            //memcpy(buf1, buf1+cursor, buf1Length);

                            //if(tick == 0)
                            //{
                            //	tick = _GetTickCount64();
                            //}
                            //tmpSleepTick = _GetTickCount64();

                            //int sleepMS = (int)(totalPCR / 90 - (tmpSleepTick - tick));
                            //if(sleepMS > FTDS_SLEEP_TIME)
                            //{
                            //	//timeBeginPeriod(sleepMS);
                            //	mySleep(sleepMS);
                            //	//timeEndPeriod(sleepMS);
                            //}
                        }
                    }
                }
                else
                {
                    messager(msgERROR, "NEXT_TRANS pdu error! %lld-%lld",
                             nIter->resendDeque[kk].resendStartSeq,
                             nIter->resendDeque[kk].resendStopSeq);
                }
            }
            //pConsumer->nextTrans[j].resendDeque.clear();
            tmpTick = _GetTickCount64();
            if(nIter->waitNextTrans.lastIndex == NEXT_TRANS_LAST_INDEX_FOR_DELETE)
            {
                tmpTick = _GetTickCount64();
            }
        }
        mySleep(FTDS_SLEEP_TIME * 8);
    }
    outofresendsupplier:
    pSupplierTask->lockSupplierThread.lock();
    pSupplierTask->lockDeleteQuitConsumer.lock();
    for(UINT i = 0; i<pSupplierTask->consumerDeque.size(); i++)
    {
        if(pSupplierTask->consumerDeque[i].UID == pConsumer->UID)
        {
            messager(msgERROR, "ERASE consumer UID %d, IP %s from channel %d supplier consumerDeque",
                     pSupplierTask->consumerDeque[i].UID,
                     inet_ntoa(pSupplierTask->consumerDeque[i].ipAddr.sin_addr),
                     pSupplierTask->channelID);
            pSupplierTask->consumerDeque[i].lockConsumerNextTransDeque.destroy();
            pSupplierTask->consumerDeque.erase(pSupplierTask->consumerDeque.begin() + i);
            break;
        }
    }
    pSupplierTask->lockDeleteQuitConsumer.unlock();
    pSupplierTask->lockSupplierThread.unlock();


#if defined(MOHO_X86)
    return NULL;
#elif defined(MOHO_WIN32)
    return 0;
#endif
}

#if defined(MOHO_X86)
void* supplierTaskThreadProc_CDN(LPVOID lpParameter)
#elif defined(MOHO_WIN32)
        static DWORD WINAPI supplierTaskThreadProc_CDN(LPVOID lpParameter)
#endif
{
#if defined(MOHO_X86)
    signal(SIGUSR1, probeThreadExit);
#endif

    messager(msgINFO, "start thread supplierTaskThreadProc_CDN");

    UINT RESTART_CHANNEL_CONDITION_2 = globalConfig.maxSupplyBuffer;
    if(RESTART_CHANNEL_CONDITION_2 < MAX_ARRIVED_BUF_LENGTH)
    {
        RESTART_CHANNEL_CONDITION_2 = MAX_ARRIVED_BUF_LENGTH;
    }
    DWORD ret = 0;
    tsSupplierTask *pSupplierTask = (tsSupplierTask *)lpParameter;

    //UINT64 seqNum = pSupplierTask->seqNum;
    UINT tsUID = pSupplierTask->tsUID;
    UINT fileIndex = pSupplierTask->fileIndex;


    //SOCKET localSendSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    FILE *fp = NULL;

    int filePosition;
    int fileSize;

    CMyFile myfile;
    char filespec[256];

    char buf[TS_READ_SIZE * 188];
    int bufIndex = 0;
    int firstIndex;
    int localReadLen = 0;
    char sendBuf[1500];
    pduHeader pHeader;
    pduTransData pData;

    char sendBufOffset[1500];
    pduHeader pHeaderOffset;
    pduTransData pDataOffset;


    int readLen ;
    int len;
    int tolen;
    int pduNumber = 0;
    //	int tmpLen;
    int fileLength = 0;
    int sendLength = 0;
    int freadLength = 0;
    UINT fileMS = 0;

    BYTE buf1[3000];
    int buf1Length = 0;
    INT64 totalPCR = 0;
    INT64 currPCR = 0;
    INT64 lastPCR = 0;
    UINT64 tmpTick;
    UINT64 behindTick = 0;

    char fileNameCache[260];

    tolen = sizeof(SOCKADDR_IN);
    //pSupplierTask->consumerIP.sin_port = htons(pSupplierTask->consumerPort);

    UINT64 tick = _GetTickCount64();

    UINT64 debugTick = _GetTickCount64();

    UINT64 errorTick = _GetTickCount64();

    int linkNumber = 1;
    //int offset = 0;
    //int nextOffset = 0;

    //pSupplierTask->streamSend = true;
    int sendNum = 0;

    //time_t endTimeOfLastFile;
    //time_t startTimeOfCurrFile;


    //Sleep(5000); //wait for consumer thread ready

    while(!pSupplierTask->EXITsupplier)
    {
        if(fp == NULL && pSupplierTask->sendBytes == 0)
        {
            //001_CCTV1_0001_20110704_093806_060033.ts
            //pSupplierTask->fileIndex += 5;
            //snprintf(filespec, 256, "%s%3d_%s_%4d_*.ts"
            //	, pSupplierTask->channel->channelFilePath.c_str()
            //	, pSupplierTask->channel->channelID
            //	, pSupplierTask->channel->channelname
            //	, pSupplierTask->fileIndex);

#if defined(MOHO_X86)
            mysprintf(filespec, 256, "%3d_%s_(.?)(.?)(.?)(.?)(.?)(.?)(.?)(.?)_(.?)(.?)(.?)(.?)(.?)(.?)_(.?)(.?)(.?)(.?)(.?)(.?)_%d_(.*).ts"
                      , pSupplierTask->channel->channelID
                      , pSupplierTask->channel->channelName.c_str()
                      , fileIndex);
#elif defined(MOHO_WIN32)
            mysprintf(filespec, 256, "%3d_%s_????????_??????_??????_%d_*.ts"
                      , pSupplierTask->channel->channelID
                      , pSupplierTask->channel->channelName.c_str()
                      , fileIndex);
#endif
            for(UINT i = 0; i<strlen(filespec); i++)
            {
                if(filespec[i] == ' ')
                    filespec[i] = '0';
            }

            if(0 == myfile.findFirst((char*)pSupplierTask->channel->channelFilePath.c_str(),filespec))
            {
                myfile.close();
                messager(msgERROR, "supplierTaskThreadProc: %s has no such file, EXIT THREAD", filespec);
                goto endofsupplier;
            }
            else
            {

                char* name = myfile.getName();
                memcpy(fileNameCache, name, 260);
                mysprintf(filespec, 256, "%s%s", pSupplierTask->channel->channelFilePath.c_str(), name);
                myfile.close();
                if((fp = fopen(filespec, "rb")) == NULL)
                {
                    messager(msgERROR, "supplierTaskThreadProc: cannot open the file %s, EXIT THREAD", filespec);
                    goto endofsupplier;
                }
                else
                {
                    pSupplierTask->channel->needToRestartSupplier = FALSE;
                    //getEndTimeFromFileName(name, &endTimeOfLastFile);
                    pSupplierTask->channel->getPCRPidFromFile(name);

                    fseek(fp,0L,SEEK_END);
                    fileLength = ftell(fp);
                    fseek(fp,0L,SEEK_SET);

                    freadLength = 0;
                    sendLength = 0;

                    fileSize = fileLength;
                    filePosition = 0;
                    firstIndex = fileIndex;

                    messager(msgINFO, "supplierTaskThreadProc, channel %d open the FIRST file %d, fileLength = %d, pcrPID = 0x%x", 
                             pSupplierTask->channelID,
                             fileIndex,
                             fileLength,
                             pSupplierTask->channel->pcrPID);
                }
            }
        }

        if(fp == NULL)
        {
            readLen = 0;
        }
        else if(bufIndex == 0)
        {
            if(fileSize - freadLength >= TS_READ_SIZE * 188)
            {
                readLen = fread(buf, sizeof(char), TS_READ_SIZE * 188, fp);
            }
            else if(fileSize - freadLength > 0)
            {
                readLen = fread(buf, sizeof(char), fileSize - freadLength, fp);
            }
            else
            {
                readLen = fread(buf, sizeof(char), 1, fp);
            }
            if(readLen + freadLength >= fileSize)
            {
                char ms[256];
                //001_ch0_20111220_122510_005600_899_A3B597CE.ts
                strncpy(ms, filespec + pSupplierTask->channel->channelFilePath.length()
                        + 3 + 1
                        + pSupplierTask->channel->channelName.length() + 1
                        + 8 + 1
                        + 6 + 1
                        , 6);
                fileMS = atoi(ms);
            }
            //    messager(msgERROR, "read file %d,   %d_%d_%d", readLen,fileSize,freadLength,fileSize - freadLength);
            if(ferror(fp))
            {
                messager(msgERROR, "read file %s error", fileNameCache);
            }
            else if(feof(fp))
            {
                //char ms[256];
                ////001_CCTV1_0001_20110804_121841_060034
                //strncpy(ms, filespec + pSupplierTask->channel->channelFilePath.length()
                //					 + 3 + 1
                //					 + pSupplierTask->channel->channelName.length() + 1
                //					 + 4 + 1
                //					 + 8 + 1
                //					 + 6 + 1
                //					 , 6);
                //fileMS = atoi(ms);

                fclose(fp);

                //messager(msgINFO, "File %s last %d ms", filespec, fileMS);

                fp = NULL;

                fileIndex ++;

                //if(pSupplierTask->fileIndex > 9999)
                //{
                //	pSupplierTask->fileIndex = 1;
                //}

                //pSupplierTask->fileIndex ++;
                //if(pSupplierTask->fileIndex - firstIndex > 29)
                //{
                //	pSupplierTask->fileIndex = firstIndex;
                //}

                //snprintf(filespec, 256, "%s%3d_%s_%4d_*.ts"
                //	, pSupplierTask->channel->channelFilePath.c_str()
                //	, pSupplierTask->channel->channelID
                //	, pSupplierTask->channel->channelname
                //	, pSupplierTask->fileIndex);




                int waitTimes = 0;
                waitfornextfile:
                //if(pSupplierTask->tsUID != tsUID)
                if(pSupplierTask->channel->needToRestartSupplier)
                {
                    pSupplierTask->channel->needToRestartSupplier = FALSE;
                    messager(msgWARN, "channel %d supplier restart", pSupplierTask->channelID);
                    pSupplierTask->lockSupplierThread.lock();
                    pSupplierTask->lockSentBufQue.lock();

                    std::deque<pduBuffer *>::iterator pIter;
                    for(pIter = pSupplierTask->sentBufQue->begin(); pIter != pSupplierTask->sentBufQue->end(); pIter ++)
                    {
                        delete (*pIter);
                    }
                    pSupplierTask->sentBufQue->clear();
                    pSupplierTask->waitNextTrans.clear();
                    fileIndex = pSupplierTask->fileIndex;
                    tsUID = pSupplierTask->tsUID;
                    //					seqNum = pSupplierTask->seqNum;

                    pSupplierTask->lockSentBufQue.unlock();
                    pSupplierTask->lockSupplierThread.unlock();
                    pSupplierTask->update2TrackerEvent = evtQuit;
                    fileLength = 0;
                    sendLength = 0;
                    freadLength = 0;
                    fileMS = 0;

                    buf1Length = 0;
                    totalPCR = 0;
                    currPCR = 0;
                    lastPCR = 0;
                    behindTick = 0;
                    tick = _GetTickCount64();

                    debugTick = _GetTickCount64();

                    errorTick = _GetTickCount64();

                    linkNumber = 1;

                }

#if defined(MOHO_X86)
                mysprintf(filespec, 256, "%3d_%s_(.?)(.?)(.?)(.?)(.?)(.?)(.?)(.?)_(.?)(.?)(.?)(.?)(.?)(.?)_(.?)(.?)(.?)(.?)(.?)(.?)_%d_(.*).ts"
                          , pSupplierTask->channel->channelID
                          , pSupplierTask->channel->channelName.c_str()
                          , fileIndex);
#elif defined(MOHO_WIN32)
                mysprintf(filespec, 256, "%3d_%s_????????_??????_??????_%d_*.ts"
                          , pSupplierTask->channel->channelID
                          , pSupplierTask->channel->channelName.c_str()
                          ,fileIndex);
#endif
                for(UINT i = 0; i<strlen(filespec); i++)
                {
                    if(filespec[i] == ' ')
                        filespec[i] = '0';
                }

                if(0 == myfile.findFirst((char*)pSupplierTask->channel->channelFilePath.c_str(),filespec))
                {
                    myfile.close();
					if(waitTimes % 10 == 0)
					{
						messager(msgERROR, "supplierTaskThreadProc: %s has no such file, TRY NEXT ID", filespec);
					}

                    INT64 t1 = _GetTickCount64();
                    //for(int i = 0; i<20; i++)
					for(int i = 0; i<2; i++)
                    {
                        //pSupplierTask->update2TrackerEvent = evtQuit;
                        if(pSupplierTask->EXITsupplier)
                        {
                            goto endofsupplier;
                        }
                        mySleep(100);
                    }

                    //UINT64 t2 = _GetTickCount64();

                    //pSupplierTask->fileIndex ++;//xieshengluo 2013-0718
#if defined(MOHO_X86)
                    mysprintf(filespec, 256, "%3d_%s_(.?)(.?)(.?)(.?)(.?)(.?)(.?)(.?)_(.?)(.?)(.?)(.?)(.?)(.?)_(.?)(.?)(.?)(.?)(.?)(.?)_%d_(.*).ts"
                              , pSupplierTask->channel->channelID
                              , pSupplierTask->channel->channelName.c_str()
                              , fileIndex);
#elif defined(MOHO_WIN32)
                    mysprintf(filespec, 256, "%3d_%s_????????_??????_??????_%d_*.ts"
                              , pSupplierTask->channel->channelID
                              , pSupplierTask->channel->channelName.c_str()
                              , fileIndex);
#endif
                    for(UINT i = 0; i<strlen(filespec); i++)
                    {
                        if(filespec[i] == ' ')
                            filespec[i] = '0';
                    }
                    waitTimes ++;

                    INT64 t2 = _GetTickCount64();
                    //totalPCR += (t2 - t1) * 90;//2013-1107

                    if(waitTimes > WAIT_FOR_ID_TIMES)
                    {
                        messager(msgERROR, "supplierTaskThreadProc: channel %d has no data, EXIT THREAD", pSupplierTask->channelID);
                        goto endofsupplier;
                    }
                    else
                    {
                        goto waitfornextfile;
                    }
                    //messager(msgERROR, "supplierTaskThreadProc: %s has no such file, EXIT THREAD", filespec);
                    //goto endofsupplier;
                    //fp = NULL;
                    //UINT64 t1 = _GetTickCount64();
                    //Sleep(60 * 1000);
                    //UINT64 t2 = _GetTickCount64();
                    //totalPCR += (t2 - t1) * 90;
                    //goto waitfornextfile;
                    //goto endofsupplier;
                }
                else
                {
                    char *name = myfile.getName();
                    memcpy(fileNameCache, name, 260);
                    mysprintf(filespec, 256, "%s%s", pSupplierTask->channel->channelFilePath.c_str(), name);
                    myfile.close();
                    reopenthefile:
                    if((fp = fopen(filespec, "rb")) == NULL)
                    {
                        messager(msgERROR, "channel %d cannot open the NEXT file %s, REOPEN it 200ms later",
                                 pSupplierTask->channelID,
                                 filespec);
                        fp = NULL;
                        INT64 t1 = _GetTickCount64();
                        //for(int i = 0; i<20; i++)
						for(int i = 0; i<2; i++)
                        {
                            pSupplierTask->update2TrackerEvent = evtQuit;
                            if(pSupplierTask->EXITsupplier)
                            {
                                goto endofsupplier;
                            }
                            mySleep(100);
                        }
                        INT64 t2 = _GetTickCount64();
                        //totalPCR += (t2 - t1) * 90;//20131107
                        goto reopenthefile;
                        //LeaveCriticalSection(&pSupplierTask->lockSupplierThread);
                        //goto endofsupplier;
                        //return ret;
                    }
                    else
                    {
                        UINT64 tmpTick = _GetTickCount64();
                        //if(pSupplierTask->channelID == 1)
                        //if((tmpTick - tick) - (totalPCR/90) > 1000 * 15)
                        {
                            //getStartTimeFromFileName(name, &startTimeOfCurrFile);

                            //if(startTimeOfCurrFile - endTimeOfLastFile > 1)
                            //{
                            //    messager(msgERROR, "channel %d lost %d seconds", pSupplierTask->channelID, (int)(startTimeOfCurrFile - endTimeOfLastFile));
                            //    //if(!pSupplierTask->channel->isVOD)
                            //    //{
                            //    //    totalPCR += (startTimeOfCurrFile - endTimeOfLastFile) * 1000 * 90;
                            //    //}
                            //    //else if(startTimeOfCurrFile - endTimeOfLastFile < 50)
                            //    //{
                            //    //    totalPCR += (startTimeOfCurrFile - endTimeOfLastFile) * 1000 * 90;
                            //    //}
                            //}

                            //getEndTimeFromFileName(name, &endTimeOfLastFile);

                            //pSupplierTask->channel->getPCRPidFromFile(name);

                        }
                        fseek(fp,0L,SEEK_END);
                        fileLength = ftell(fp);
                        fseek(fp,0L,SEEK_SET);

                        freadLength = 0;
                        sendLength = 0;

                        fileSize = fileLength;
                        filePosition = 0;

                        messager(msgINFO, "channel %d open the NEXT file %d, size = %d, totalPCR = %lld, sendClock = %lld, diff = %lld, pcrPID = 0x%x,fileSize:%d,readLen__%d,bufIndex_%d",
                                 pSupplierTask->channelID,
                                 fileIndex,
                                 pSupplierTask->sentBufQue->size(),
                                 (totalPCR/90),
                                 (tmpTick - tick),
                                 (totalPCR/90 - (tmpTick - tick) + behindTick),
                                 pSupplierTask->channel->pcrPID,
                                 fileSize,
                                 readLen,
                                 bufIndex);
                    }
                }
            }
        }
        if(readLen > 0)						//readLen = 0 means read nothing, maybe the file is not ready, consumer FAST than supplier.
        {
            if(readLen - bufIndex >= TS_PDU_LENGTH * 188)
            {
                localReadLen = TS_PDU_LENGTH * 188;
            }
            else
            {
                localReadLen = readLen - bufIndex;
            }

            freadLength += localReadLen;
            pHeader.Type = TRANS_DATA;
            pHeader.myUID = globalConfig.UID;
            //#if defined(MOHO_X86)
            //            pHeader.seqNumber = __sync_add_and_fetch(&pSupplierTask->seqNum,1);
            //#elif defined(MOHO_WIN32)
            //            pHeader.seqNumber = InterlockedIncrement(&pSupplierTask->seqNum);
            //#endif
            //pHeader.seqNumber = pSupplierTask->seqNum;
            //pSupplierTask->seqNum ++;
            pHeader.seqNumber = pSupplierTask->seqNum;
            pSupplierTask->seqNum ++;
            pHeader.Length = sizeof(pHeader) + sizeof(pData) + localReadLen;
            pData.channelID = pSupplierTask->channelID;
            pData.tsUID = tsUID;
//            if(freadLength == fileSize)
//            {
//                pData.fileMS = fileMS;
//                fileMS = 0;
//                //pHeader.Length += (strlen(file.name) + sizeof(int));
//                pHeader.Length += (strlen(fileNameCache) + sizeof(int));
//            }
//            else
//            {
//                pData.fileMS = 0;
//                //fileMS = 0;
//            }
            if(freadLength == fileSize)
            {
                pHeader.Length += strlen(fileNameCache);
                pData.fileMS = strlen(fileNameCache);
            }
			else
			{
				 pData.fileMS = 0;
			}
            memcpy(sendBuf, &pHeader, sizeof(pHeader));
            memcpy(sendBuf + sizeof(pHeader), &pData, sizeof(pData));
            memcpy(sendBuf + sizeof(pHeader) + sizeof(pData), buf + bufIndex, localReadLen);

            if(pData.fileMS != 0)
            {
                memcpy(sendBuf  + sizeof(pHeader) + sizeof(pData) + localReadLen, fileNameCache, pData.fileMS);
            }
//            if(pData.fileMS != 0)
//            {
//                int j = strlen(fileNameCache);
//                memcpy(sendBuf  + sizeof(pHeader) + sizeof(pData) + localReadLen, fileNameCache, j);
//                memcpy(sendBuf  + sizeof(pHeader) + sizeof(pData) + localReadLen + j, &j, sizeof(int));
//                //memcpy(sendBuf  + sizeof(pHeader) + sizeof(pData) + localReadLen, file.name, strlen(file.name));
//                //memcpy(sendBuf  + sizeof(pHeader) + sizeof(pData) + localReadLen + strlen(file.name), &j, sizeof(int));
//                messager(msgDEBUG, "channel %d filename = %s, filename_length = %d, fileMS = %d",
//                         pSupplierTask->channelID, fileNameCache, j, pData.fileMS);
//            }

            pSupplierTask->update2TrackerEvent = evtAlive;
            std::deque<tsConsumer>::iterator tcIter;
            pSupplierTask->lockSupplierThread.lock();
            for(tcIter = pSupplierTask->consumerDeque.begin(); tcIter != pSupplierTask->consumerDeque.end(); tcIter ++)
            {
                if(tcIter->UID > 0)
                {
                    if(tcIter->seqOffset == 0)
                    {
                        len = sendto(/*localSendSocket*/*(Info.sendSocket), sendBuf, pHeader.Length, 0, (SOCKADDR *)(&(tcIter->ipAddr)), tolen) ;
                        if(len == SOCKET_ERROR)
                        {
                            if(_GetTickCount64() - errorTick  > 30 * 1000)
                            {
                                messager(msgERROR, "send to failed, dest ip = %s, msgtype = %d, error number = %d", inet_ntoa(tcIter->ipAddr.sin_addr), pHeader.Type, myGetLastError());
                                errorTick = _GetTickCount64();
                            }
                        }
                        else if(len != pHeader.Length)
                        {
                            messager(msgERROR, "send to error, dest ip = %s, msgtype = %d, msglen = %d, sendlen = %d", inet_ntoa(tcIter->ipAddr.sin_addr), pHeader.Type, pHeader.Length, len);
                        }
                    }
                    else
                    {

                        pHeaderOffset.Type = TRANS_DATA;
                        pHeaderOffset.myUID = globalConfig.UID;

                        pSupplierTask->lockSentBufQue.lock();
                        UINT index = pSupplierTask->sentBufQue->size() - tcIter->seqOffset;
                        pHeaderOffset.seqNumber = pSupplierTask->sentBufQue->at(index)->seqNum;
                        pHeaderOffset.Length = sizeof(pHeaderOffset) + sizeof(pDataOffset) + pSupplierTask->sentBufQue->at(index)->length;
                        pDataOffset.channelID = pSupplierTask->channelID;
                        pDataOffset.tsUID = pSupplierTask->tsUID;
                        pDataOffset.fileMS = pSupplierTask->sentBufQue->at(index)->fileMS;
                        memcpy(sendBufOffset, &pHeaderOffset, sizeof(pHeaderOffset));
                        memcpy(sendBufOffset + sizeof(pHeaderOffset), &pDataOffset, sizeof(pDataOffset));
                        memcpy(sendBufOffset + sizeof(pHeaderOffset) + sizeof(pDataOffset), pSupplierTask->sentBufQue->at(index)->buf, pSupplierTask->sentBufQue->at(index)->length);
                        pSupplierTask->lockSentBufQue.unlock();
                        len = sendto(*(Info.sendSocket), sendBufOffset, pHeaderOffset.Length, 0, (SOCKADDR *)(&(tcIter->ipAddr)), tolen) ;
                        if(len == SOCKET_ERROR)
                        {
                            messager(msgERROR, "offsetsend to failed, dest ip = %s, msgtype = %d, error number = %d", inet_ntoa(tcIter->ipAddr.sin_addr), pHeaderOffset.Type, myGetLastError());
                        }
                        else if(len != pHeaderOffset.Length)
                        {
                            messager(msgERROR, "offsetsend to error, dest ip = %s, msgtype = %d, msglen = %d, sendlen = %d", inet_ntoa(tcIter->ipAddr.sin_addr), pHeaderOffset.Type, pHeaderOffset.Length, len);
                        }
                    }
                }
            }
            pSupplierTask->lockSupplierThread.unlock();
            //else
            {
                sendNum ++;
                sendLength += (len - sizeof(pHeader) - sizeof(pData));
                pSupplierTask->sendBytes += localReadLen;
                //messager(msgDEBUG, "send TRANS_DATA to %s, channelID = %d, dataLen = %d, seqNum = %d",
                //	inet_ntoa(pSupplierTask->consumerIP.sin_addr),
                //	pData.channelID,
                //	readLen,
                //	pHeader.seqNumber);
		
                if(pData.fileMS > 0)
                {
                    //messager(msgDEBUG, "send TRANS_DATA to %s, channelID = %d, dataLen = %d, seqNum = %d, fileMS = %d",
                    //	inet_ntoa(pSupplierTask->consumerIP.sin_addr),
                    //	pData.channelID,
                    //	readLen,
                    //	pHeader.seqNumber,
                    //	pData.fileMS);
                }

                pduBuffer *p = new pduBuffer;

                if(p == NULL)
                {
                    messager(msgFATAL, "channelID %d: NO MEMORY,  EXIT THREAD.", pSupplierTask->channelID);
                    std::deque<pduBuffer *>::iterator pIter;
                    for(pIter = pSupplierTask->sentBufQue->begin(); pIter != pSupplierTask->sentBufQue->end(); pIter ++)
                    {
                        delete (*pIter);
                    }

                    pSupplierTask->sentBufQue->clear();
                    goto endofsupplier;
                }

                p->seqNum = pHeader.seqNumber;
                //p->length = localReadLen;
                p->length = pHeader.Length - sizeof(pHeader) - sizeof(pData);
                p->fileMS = pData.fileMS;
                p->arrivedTick = _GetTickCount64();
                //memcpy(p->buf, buf + bufIndex, localReadLen);
                memcpy(p->buf, sendBuf + sizeof(pHeader) + sizeof(pData), p->length);

                pSupplierTask->lockSentBufQue.lock();

                //  messager(msgINFO, "push.at(%d)__%p",pSupplierTask->sentBufQue->size(),p);

                pSupplierTask->sentBufQue->push_back(p);


                if(pSupplierTask->sentBufQue->size() > RESTART_CHANNEL_CONDITION_2)
                {
                    //delete pSupplierTask->sentBufQue[0]->buf;
                    delete (pSupplierTask->sentBufQue->at(0));
                    pSupplierTask->sentBufQue->erase(pSupplierTask->sentBufQue->begin());
                }
                pSupplierTask->lockSentBufQue.unlock();

                memcpy(buf1 + buf1Length, buf + bufIndex, localReadLen);
                buf1Length += localReadLen;

                int cursor = 0;
                while(buf1Length >= 188)
                {
                    if(buf1[cursor] == 0x47)
                    {
                        BYTE *inBuf = (BYTE *)(buf1 + cursor);
                        if(inBuf[0] == 0x47 &&
                           (inBuf[1] & 0x1f) * 256 + inBuf[2] == pSupplierTask->channel->pcrPID &&
                           inBuf[3] & 0x20 &&
                           inBuf[4] >= 0x07 &&
                           inBuf[5] & 0x10)
                        {
                            lastPCR = currPCR;
                            currPCR = (inBuf[6] * 1677216 + inBuf[7] * 65536 + inBuf[8] * 256 + inBuf[9] ) * 2;
                            if(inBuf[10] & 0x80)
                            {
                                currPCR ++;
                            }
                            if(lastPCR != 0 && currPCR > lastPCR && currPCR - lastPCR < MAX_PCR_PERIOD_MS * 90)
                            {
                                totalPCR += (currPCR - lastPCR);
                            }
                        }
                        cursor += 188;
                        buf1Length -= 188;
                    }
                    else
                    {
                        cursor ++;
                        buf1Length --;
                    }
                }
                memcpy(buf1, buf1+cursor, buf1Length);
                tmpTick = _GetTickCount64();
                //if( totalPCR / 90 - (tmpTick - tick) >=  FTDS_SLEEP_TIME * 2 )
                //{
                //	//startPCR = currPCR;
                //	int sleepMS = totalPCR / 90 - (tmpTick - tick);
                //	//if(sleepMS > 0)
                //	{
                //		timeBeginPeriod(sleepMS);
                //		Sleep(sleepMS);
                //		timeEndPeriod(sleepMS);
                //	}
                //}
                //else
                //{
                //	timeBeginPeriod(1);
                //	Sleep(1);
                //	timeEndPeriod(1);
                //}
                int sleepMS = (int)(totalPCR / 90 - (tmpTick - tick));
                //if( sleepMS >  SUPPLIER_SEND_SLEEP_TIME * 16 )
                //{
                //	timeBeginPeriod(SUPPLIER_SEND_SLEEP_TIME * 16);
                //	Sleep(SUPPLIER_SEND_SLEEP_TIME * 16);
                //	timeEndPeriod(SUPPLIER_SEND_SLEEP_TIME * 16);
                //}
                //else if(sleepMS > SUPPLIER_SEND_SLEEP_TIME * 8 && sleepMS <= SUPPLIER_SEND_SLEEP_TIME * 16)
                //{
                //	timeBeginPeriod(SUPPLIER_SEND_SLEEP_TIME * 8);
                //	Sleep(SUPPLIER_SEND_SLEEP_TIME * 8);
                //	timeEndPeriod(SUPPLIER_SEND_SLEEP_TIME * 8);
                //}
                //else if(sleepMS > SUPPLIER_SEND_SLEEP_TIME * 4 && sleepMS <= SUPPLIER_SEND_SLEEP_TIME * 8)
                //{
                //	timeBeginPeriod(SUPPLIER_SEND_SLEEP_TIME * 4);
                //	Sleep(SUPPLIER_SEND_SLEEP_TIME * 4);
                //	timeEndPeriod(SUPPLIER_SEND_SLEEP_TIME * 4);
                //}
                //else if(sleepMS > SUPPLIER_SEND_SLEEP_TIME * 2 && sleepMS <= SUPPLIER_SEND_SLEEP_TIME * 4)
                //{
                //	timeBeginPeriod(SUPPLIER_SEND_SLEEP_TIME * 2);
                //	Sleep(SUPPLIER_SEND_SLEEP_TIME * 2);
                //	timeEndPeriod(SUPPLIER_SEND_SLEEP_TIME * 2);
                //}
                //else
                //{
                //	timeBeginPeriod(SUPPLIER_SEND_SLEEP_TIME);
                //	Sleep(SUPPLIER_SEND_SLEEP_TIME);
                //	timeEndPeriod(SUPPLIER_SEND_SLEEP_TIME);
                //}

                if(sleepMS > SUPPLIER_SEND_SLEEP_TIME_1)
                {
                    //timeBeginPeriod(sleepMS);

                    //mySleep(sleepMS);
//                    if(sleepMS >= 1000)
//                    {
//                        messager(msgERROR, "supplyer sleepMS__%d > SUPPLIER_SEND_SLEEP_TIME_1__%d", sleepMS, SUPPLIER_SEND_SLEEP_TIME_1);
//                    }
					mySleep(SUPPLIER_SEND_SLEEP_TIME_1);

                    //timeEndPeriod(sleepMS);
                }

                //if( sleepMS >=  SUPPLIER_SEND_SLEEP_TIME_1 )
                //{
                //	//startPCR = currPCR;
                //	//int sleepMS = totalPCR / 90 - (tmpTick - tick);
                //	//if(sleepMS > 0)
                //	if(behindTick == 0)
                //	{
                //		//timeBeginPeriod(sleepMS);
                //		//Sleep(SUPPLIER_SEND_SLEEP_TIME_1 / 2);
                //		Sleep(sleepMS);
                //		sendNum = 0;
                //		//timeEndPeriod(sleepMS);
                //	}
                //	else
                //	{
                //		Sleep(SUPPLIER_SEND_SLEEP_TIME_1 / 2);
                //		if(behindTick > sleepMS - SUPPLIER_SEND_SLEEP_TIME_1 / 2)
                //		{
                //			behindTick -= (sleepMS - SUPPLIER_SEND_SLEEP_TIME_1 / 2);
                //			tick -= (sleepMS - SUPPLIER_SEND_SLEEP_TIME_1 / 2);
                //		}
                //		else
                //		{
                //			tick -= behindTick;
                //			behindTick = 0;
                //		}
                //	}
                //}
                //else if(sendNum > MAX_SEND_NUM)
                //{
                //	Sleep(FTDS_SLEEP_TIME * 2);
                //	sendNum = 0;
                //}
                //else
                //{
                //	Sleep(0);
                //}
            }//else, send success

            bufIndex += localReadLen;
            if(bufIndex == readLen)
            {
                bufIndex = readLen = 0;
            }

        }//if(readLen >= 0)
        else
        {
            if(fp == NULL)
            {
                //if(pSupplierTask->sentBufQue->size() == offset)
                if(pSupplierTask->sentBufQue->size() == 0)
                {
                    messager(msgINFO, "channel %d finished all send task", pSupplierTask->channelID);
                    pSupplierTask->sentBufQue->clear();

                    pSupplierTask->lockSupplierThread.lock();
                    pSupplierTask->waitNextTrans.clear();
                    pSupplierTask->lockSupplierThread.unlock();

                    goto endofsupplier;
                }
                else
                {
                    //pHeader.Type = TRANS_DATA;
                    //pHeader.myUID = globalConfig.UID;
                    //pHeader.seqNumber = pSupplierTask->sentBufQue[offset]->seqNum;
                    //pHeader.Length = sizeof(pHeader) + sizeof(pData) + pSupplierTask->sentBufQue[offset]->length;
                    //pData.channelID = pSupplierTask->channelID;
                    //pData.fileMS = pSupplierTask->sentBufQue[offset]->fileMS;
                    //memcpy(sendBuf, &pHeader, sizeof(pHeader));
                    //memcpy(sendBuf + sizeof(pHeader), &pData, sizeof(pData));
                    //memcpy(sendBuf + sizeof(pHeader) + sizeof(pData), pSupplierTask->sentBufQue[offset]->buf, pSupplierTask->sentBufQue[offset]->length);

                    //len = sendto(localSendSocket, sendBuf, pHeader.Length, 0, (SOCKADDR *)(&(pSupplierTask->consumerIP)), tolen) ;
                    //if(len == SOCKET_ERROR)
                    //{
                    //	messager(msgERROR, _T("send to failed, dest ip = %s, msgtype = %d, error number = %d"), inet_ntoa(pSupplierTask->consumerIP.sin_addr), pHeader.Type, myGetLastError());
                    //}
                    //else if(len != pHeader.Length)
                    //{
                    //	messager(msgERROR, _T("send to error, dest ip = %s, msgtype = %d, msglen = %d, sendlen = %d"), inet_ntoa(pSupplierTask->consumerIP.sin_addr), pHeader.Type, pHeader.Length, len);
                    //}
                    //timeBeginPeriod(SUPPLIER_SEND_SLEEP_TIME * 2);
                    //Sleep(SUPPLIER_SEND_SLEEP_TIME * 2);
                    //timeEndPeriod(SUPPLIER_SEND_SLEEP_TIME * 2);
                }
            }
            //			if(fp == NULL)
            //			{
            //				EnterCriticalSection(&pSupplierTask->lockSupplierThread);
            //				pSupplierTask->waitNextTrans.clear();
            //				LeaveCriticalSection(&pSupplierTask->lockSupplierThread);
            ////				pSupplierTask->streamSend = false;
            //				goto endofsupplier;
            //			}
        }
        //delete seqNumber;
        //delete recvBytes;
        //delete sendBytes;
        //endofwhile:
      //  messager(msgDEBUG, "OUT supplier task");


        tmpTick = _GetTickCount64();
        if((int)((tmpTick - tick) - totalPCR / 90) > RESTART_CHANNEL_CONDITION * 1000)
        {
            messager(msgDEBUG, "channel %d diff = %d, size = %d, NEED TO ADJUST totalPCR,REALLY?",
                     pSupplierTask->channelID,
                     (int)((tmpTick - tick) - totalPCR / 90),
                     pSupplierTask->sentBufQue->size());
			//totalPCR = (tmpTick - tick) * 90;//2013-1107
            //goto endofsupplier;
        }
    }//while(true)

    endofsupplier:
    messager(msgINFO, "channel %d EXIT Supplier TASK THREAD for restart", pSupplierTask->channel->channelID);
    pSupplierTask->EXITsupplier = true;
    BOOL allExited = FALSE;
    while(!allExited)
    {
        allExited = TRUE;
        pSupplierTask->lockSupplierThread.lock();
        for(UINT i = 0; i<pSupplierTask->consumerDeque.size(); i++)
        {
            if(pSupplierTask->consumerDeque[i].UID > 0)
            {
                allExited = FALSE;
                break;
            }
        }
        pSupplierTask->lockSupplierThread.unlock();
        mySleep(100);
    }
    mySleep(200);
    lockTsSupplierTaskList.lock();
    //pSupplierTask->lockSupplierThread.lock();
    //pSupplierTask->lockSentBufQue.lock();
    std::deque<pduBuffer *>::iterator pIter;

    //    for(pIter = pSupplierTask->sentBufQue->begin(); pIter != pSupplierTask->sentBufQue->end(); pIter ++)
    //    {
    //        delete (*pIter);
    //    }

    messager(msgINFO, "pSupplierTask->sentBufQue->size()__%d",pSupplierTask->sentBufQue->size());
    for(UINT jj = 0; jj<pSupplierTask->sentBufQue->size(); jj++)
    {
        //  messager(msgINFO, "pSupplierTask->sentBufQue->at(%d)__%p",jj,pSupplierTask->sentBufQue->at(jj));

        delete pSupplierTask->sentBufQue->at(jj);
    }
    messager(msgINFO, "delete pSupplierTask->sentBufQue ok");

    pSupplierTask->sentBufQue->clear();
    pSupplierTask->waitNextTrans.clear();
    //pSupplierTask->lockSentBufQue.unlock();
    //pSupplierTask->lockSupplierThread.unlock();

    pSupplierTask->lockSupplierThread.destroy();
    pSupplierTask->lockSentBufQue.destroy();

    std::list<tsSupplierTask>::iterator sIter;
    //EnterCriticalSection(&lockTsSupplierTaskList);

//    for(sIter = globalTsSupplierTaskList.begin(); sIter != globalTsSupplierTaskList.end(); sIter ++)
//    {
//        if(sIter->channelID == pSupplierTask->channelID)
//        {
//            messager(msgERROR, "erase channel %d from supplier list", pSupplierTask->channelID);
//            globalTsSupplierTaskList.erase(sIter);
//            break;
//        }
//    }
    lockTsSupplierTaskList.unlock();

    if(fp != NULL)
    {
        fclose(fp);
    }

    //closesocket(localSendSocket);

    //delete []sendBuf;
    //delete []filespec;
    //delete []buf;

    //for(int i = 0; i<pSupplierTask->sentBufQue->size(); i++)
    //{
    //	delete (*pSupplierTask->sentBufQue->begin());
    //	//EnterCriticalSection(&pSupplierTask->lockSentBufQue);
    //	pSupplierTask->sentBufQue->pop_front();
    //	//LeaveCriticalSection(&pSupplierTask->lockSentBufQue);
    //}

    //std::deque<pduBuffer *>::iterator pIter;
    //for(pIter = (pSupplierTask->sentBufQue).begin(); pIter != (pSupplierTask->sentBufQue).end(); pIter ++)
    //{
    //	delete (*pIter);
    //}

    //pSupplierTask->sentBufQue->clear();

    messager(msgINFO, "out from supplier task thread");
#if defined(MOHO_X86)
    return NULL;
#elif defined(MOHO_WIN32)
    return 0;
#endif
}


#if defined(MOHO_X86)
void* moveFile2Exception(LPVOID lpParameter)
#elif defined(MOHO_WIN32)
        static DWORD WINAPI moveFile2Exception(LPVOID lpParameter)
#endif
{
    DWORD ret = 0;
    tsSupplierTask *sIter = (tsSupplierTask *)lpParameter;

    std::string succeededIDFile = sIter->channel->channelFilePath + "succeededID.txt";
    FILE *fp = fopen(succeededIDFile.c_str(), "rb");
    int succeededID = -1;
    if(fp != NULL)
    {
        fscanf(fp, "%d", &succeededID);
        fclose(fp);
    }
    else
    {
        messager(msgERROR, "open %s ERROR", succeededIDFile.c_str());
        ret = -1;
#if defined(MOHO_X86)
        return NULL;
#elif defined(MOHO_WIN32)
        return ret;
#endif
    }

    int start = succeededID + 1;
    int end = sIter->fileIndex - 1;
    if(end - start > 60)
    {
        start = end - 60;
    }

    for(int i = start; i<=end; i++)
    {
        char filename[256];
        char destfilename[256];

        CMyFile myfile;
        char filespec[256];

#if defined(MOHO_X86)
        mysprintf(filespec, 256, "(.*)%d(.*).ts", i);
#elif defined(MOHO_WIN32)
        mysprintf(filespec, 256, "*%d*.ts", i);
#endif
        if(1 == myfile.findFirst((char*)sIter->channel->channelFilePath.c_str(),filespec))
        {
            do
            {
                char* name = myfile.getName();

                //remove(filename);

                mysprintf(destfilename, 256, "%s%s", sIter->channel->exceptionFilePath.c_str(), name);

                //CopyFile(filename, destfilename, false);
                rename(filename, destfilename);
            }while(myfile.findNext());
        }
        myfile.close();
    }
#if defined(MOHO_X86)
    return NULL;
#elif defined(MOHO_WIN32)
    return ret;
#endif
}

//提供者收到请求开始传递报文后，更新提供者状态，启动提供者线程
int updateSupplierTask(pduHeader *ppHeader, pduPleaseStartTrans *ppStart, SOCKADDR_IN *pAddrClient)
{
    int ret = 0;
    std::list<tsSupplierTask>::iterator sIter;
    char tmp[256];

    lockTsSupplierTaskList.lock();
    //supplier already exist, insert consumer to supplier's consumer deque
    for(sIter = globalTsSupplierTaskList.begin(); sIter != globalTsSupplierTaskList.end(); sIter ++)
    {
        if(sIter->channelID == ppStart->channelID)
        {

            tsConsumer consumer;
            consumer.ipAddr = *pAddrClient;
            consumer.UID = ppHeader->myUID;
            consumer.nextTrans.clear();

            sIter->lockSupplierThread.lock();
            sIter->lockSentBufQue.lock();
            if(ppStart->expectedSeqNum > 0 &&
               ppStart->expectedSeqNum < sIter->seqNum &&
               ppStart->expectedSeqNum > sIter->sentBufQue->front()->seqNum + 1000)
            {
                consumer.seqOffset = (UINT)(sIter->seqNum - ppStart->expectedSeqNum);
            }
            else
            {
                consumer.seqOffset = 0;
            }
            UINT j = 0;
            for(j = 0; j<sIter->consumerDeque.size(); j++)
            {
                if(sIter->consumerDeque[j].UID == ppHeader->myUID)
                {
                    break;
                }
            }
            if(j >= sIter->consumerDeque.size())
            {
                sIter->consumerDeque.push_back(consumer);
                messager(msgINFO, "channel %d has new consumer %d, ip = %s, seqOffset = %d",
                         sIter->channelID,
                         ppHeader->myUID,
                         inet_ntoa(pAddrClient->sin_addr),
                         sIter->consumerDeque[j].seqOffset);
            }
            else
            {
                sIter->consumerDeque[j].ipAddr = *pAddrClient;
                if(ppStart->expectedSeqNum > 0 &&
                   ppStart->expectedSeqNum < sIter->seqNum &&
                   ppStart->expectedSeqNum > sIter->sentBufQue->front()->seqNum + 1000)
                {
                    sIter->consumerDeque[j].seqOffset = (UINT)(sIter->seqNum - ppStart->expectedSeqNum);
                }
                else
                {
                    sIter->consumerDeque[j].seqOffset = 0;
                }
                messager(msgINFO, "channel %d already had consumer %d, ip = %s already, seqOffset = %d",
                         sIter->channelID,
                         ppHeader->myUID,
                         inet_ntoa(pAddrClient->sin_addr),
                         sIter->consumerDeque[j].seqOffset);
                sIter->lockSentBufQue.unlock();
                sIter->lockSupplierThread.unlock();
                lockTsSupplierTaskList.unlock();
                ret = 4;
                return ret;
            }
            sIter->lockSentBufQue.unlock();
            sIter->lockSupplierThread.unlock();

            sIter->consumerDeque[sIter->consumerDeque.size() - 1].lockConsumerNextTransDeque.init();
            sIter->para.task = &(*sIter);
            sIter->para.consumer = &(sIter->consumerDeque[sIter->consumerDeque.size() - 1]);
            mysprintf(tmp, 256, "supplierResendThreadProc");
            CMyThread localThread;
            if(0 == localThread.creat(supplierResendThreadProc,(void*)(&(sIter->para))))
            {
                messager(msgFATAL, "CreateThread %s failed (%d)", tmp, myGetLastError());
            }
            ret = 3;
            lockTsSupplierTaskList.unlock();
            return ret;
        }
    }


    //supplier not exist
    std::list<CLiveChannel *>::iterator channelIter;
    lockLiveChannelList.lock();
    for(channelIter = globalLiveChannelList.begin(); channelIter != globalLiveChannelList.end(); channelIter ++)
    {
        if(ppStart->channelID == (*channelIter)->channelID)
        {
            tsSupplierTask tTask;
            tTask.channelID = ppStart->channelID;
            tTask.channel = (*channelIter);
            tTask.fileIndex = (*channelIter)->getTheFileIndex(&(ppStart->s));//startTime modified to the indexfile's starttime;
            //tTask.consumerIP = NULL;
            //tTask.consumerUID = 0;
            tTask.startTime = ppStart->s;
            tTask.lastIndex = 0;
            tTask.sendBytes = 0;
            tTask.lastRecvBytes = 0;
            tTask.seqNum = 0;
            tTask.consumerPort = ppStart->consumerPort;
            tTask.EXITsupplier = false;
            tTask.waitNextTrans.clear();


            tsConsumer consumer;
            consumer.ipAddr = *pAddrClient;
            consumer.UID = ppHeader->myUID;
            consumer.nextTrans.clear();
            consumer.seqOffset = 0;

            globalTsSupplierTaskList.push_back(tTask);
            for(sIter = globalTsSupplierTaskList.begin(); sIter != globalTsSupplierTaskList.end(); sIter ++)
            {
                if((*sIter).channelID == ppStart->channelID)
                {
                    sIter->consumerDeque.push_back(consumer);
                    
                    mysprintf(tmp, 256, "supplierTaskThreadProc_CDN");

                   // tTask.thread.creat(supplierTaskThreadProc_CDN,(void*) &(*sIter));
                    sIter->thread.creat(supplierTaskThreadProc_CDN,(void*) &(*sIter));
                    sIter->lockSupplierThread.init();
                    sIter->lockSentBufQue.init();
                    sIter->consumerDeque[0].lockConsumerNextTransDeque.init();

                    sIter->para.task = &(*sIter);
                    sIter->para.consumer = &(sIter->consumerDeque[0]);
                    
                    mysprintf(tmp, 256, "supplierResendThreadProc");

                    CMyThread localThread;
                    localThread.creat(supplierResendThreadProc,(void*)&(sIter->para));
                    break;
                }
            }
            messager(msgINFO, "Start new supplier, channelID = %d, fileIndex = %d, startTime = %ld",
                     tTask.channelID,
                     tTask.fileIndex,
                     tTask.startTime);

            ret = 2;
            break;
        }
    }
    lockLiveChannelList.unlock();
    lockTsSupplierTaskList.unlock();
    return ret;
}

int startOneSupplier(CLiveChannel *channel, UINT64 firstPacketSeqNum, UINT tsUID, UINT fileIndex)
{
    int ret = -1;
    std::list<tsSupplierTask>::iterator sIter;
    lockTsSupplierTaskList.lock();
    for(sIter = globalTsSupplierTaskList.begin(); sIter != globalTsSupplierTaskList.end(); sIter ++)
    {
        if(sIter->channelID == channel->channelID)
        {
            messager(msgWARN, "channel %d:%s already started up,TS changed", sIter->channelID, sIter->channel->channelName.c_str());
            sIter->lockSupplierThread.lock();
            sIter->tsUID = tsUID;
            sIter->seqNum = firstPacketSeqNum;
            sIter->fileIndex = fileIndex;
            sIter->lockSupplierThread.unlock();
            lockTsSupplierTaskList.unlock();
            return ret;
        }
    }
    tsSupplierTask tTask;
    tTask.channelID = channel->channelID;
    tTask.channel = channel;
    if(fileIndex == 0)
    {
        tTask.fileIndex = (channel)->getTheMaxFileIndex();//startTime modified to the indexfile's starttime;
    }
    else
    {
        tTask.fileIndex = fileIndex;
    }
    //tTask.consumerIP = *pAddrClient;
    //tTask.consumerUID = 0;
    tTask.startTime = 0;
    tTask.lastIndex = 0;
    tTask.sendBytes = 0;
    tTask.lastRecvBytes = 0;
    tTask.seqNum = firstPacketSeqNum;
    tTask.consumerPort = 0;
    tTask.EXITsupplier = false;
    tTask.waitNextTrans.clear();
    tTask.tsUID = tsUID;
    tTask.update2TrackerEvent = evtQuit;
    tTask.sentBufQue = new std::deque<pduBuffer *>;
    globalTsSupplierTaskList.push_back(tTask);
    for(sIter = globalTsSupplierTaskList.begin(); sIter != globalTsSupplierTaskList.end(); sIter ++)
    {
        if((*sIter).channelID == channel->channelID)
        {

            sIter->consumerDeque.clear();
            char tmp[256];
            mysprintf(tmp, 256, "supplierTaskThreadProc_CDN");
         // tTask.thread.creat(supplierTaskThreadProc_CDN,(void*) &(*sIter));
            sIter->thread.creat(supplierTaskThreadProc_CDN,(void*) &(*sIter));
            sIter->lockSupplierThread.init();
            sIter->lockSentBufQue.init();
            sIter->lockDeleteQuitConsumer.init();
            break;
        }
    }
    lockTsSupplierTaskList.unlock();
    return 0;
}

//资源发布的报文应答
int replyTo(pduHeader *hdr, void *pdu, SOCKADDR_IN *addr, SOCKET *sendSocket)
{
    if(addr == NULL)
    {
        messager(msgERROR, "reply for msg %x, dest = NULL", hdr->Type);
        return -1;
    }
    int ret = 0;
    pduHeader pHeader;
    pduIAmOk pOk;
    pduIHave pHave;
    pduStartTrans pStartTrans;

    pduDoYouHave *pYouHave;
    pduPleaseStartTrans *ppStart;

    std::list<CLiveChannel *>::iterator channelIter;
    std::list<neighborNode>::iterator neighborIter;
    std::list<tsQueryTask>::iterator qIter;
    time_t searchResult;

    char msg[1500];

    int len;
    //SOCKADDR_IN destIP;
    //destIP.sin_family = addr->sin_family;
    //destIP.sin_addr = addr->sin_addr;
    //destIP.sin_port = htons(globalConfig.publicationPort);
    int tolen = sizeof(*addr);
    switch(hdr->Type)
    {
    case HOW_ARE_YOU:
        {
            pHeader.Type = I_AM_OK;
            pHeader.myUID = globalConfig.UID;
            pHeader.seqNumber = hdr->seqNumber;
            pHeader.Length = sizeof(pHeader) + sizeof(pOk);
            pOk.mySupplyPort = globalConfig.supplyPort;
            pOk.myZoneNumber = globalConfig.zoneNumber;
            pOk.myCapability = globalConfig.Capability;
            pOk.myLoad = globalConfig.currLoad;
            memcpy(msg, &pHeader, sizeof(pHeader));
            memcpy(msg + sizeof(pHeader), &pOk, sizeof(pOk));//???big or litte???
            //len = sendto(*sendSocket, msg, pHeader.Length, 0, (SOCKADDR *)(&(destIP)), tolen) ;
            len = sendto(*sendSocket, msg, pHeader.Length, 0, (SOCKADDR *)(addr), tolen) ;
            if(len == SOCKET_ERROR)
            {
                messager(msgERROR, "send to failed, dest ip = %s, msgtype = %d, error number = %d", inet_ntoa(addr->sin_addr), pHeader.Type, myGetLastError());
            }
            else if(len != pHeader.Length)
            {
                messager(msgERROR, "send to error, dest ip = %s, msgtype = %d, msglen = %d, sendlen = %d", inet_ntoa(addr->sin_addr), pHeader.Type, pHeader.Length, len);
            }
            else
            {
                //messager(msgINFO, "send I_AM_OK to %s", inet_ntoa(destIP.sin_addr));
                messager(msgINFO, "send I_AM_OK to %s:%d",
                         inet_ntoa(addr->sin_addr),
                         ntohs(addr->sin_port));
            }
            break;
        }
    case DO_YOU_HAVE:
        {
            pYouHave = (pduDoYouHave *)(pdu);
            if(pYouHave->supplierUID != globalConfig.UID)
            {
                messager(msgWARN, "recv DO_YOU_HAVE not for my UID, the UID maybe not in service");
                break;
            }
            searchResult = 0;
            std::list<tsSupplierTask>::iterator sIter;
            lockTsSupplierTaskList.lock();
            for(sIter = globalTsSupplierTaskList.begin(); sIter != globalTsSupplierTaskList.end(); sIter ++)
            {
                if(sIter->channelID == pYouHave->channelID && sIter->update2TrackerEvent == evtAlive)
                {
                    messager(msgINFO, "channel %d:%s FOUND", sIter->channelID, sIter->channel->channelName.c_str());
                    searchResult = 1;
                    break;
                }
            }
            lockTsSupplierTaskList.unlock();
            if(searchResult > 0)
            {
                pHeader.Type = I_HAVE;
                pHeader.myUID = globalConfig.UID;
                pHeader.seqNumber = hdr->seqNumber;
                pHeader.Length = sizeof(pHeader) + sizeof(pduIHave);
                pHave.bitRate = sIter->channel->channelBitRate;
                pHave.channelID = pYouHave->channelID;
                pHave.ipAddress = globalConfig.ipAddress;
                pHave.pcrPID = sIter->channel->pcrPID;
                pHave.synStartTime = searchResult;
                pHave.UID = globalConfig.UID;
                pHave.YerOrNo = 1;
                pHave.tsUID = sIter->tsUID;
                pHave.mostRecentSeqNum = sIter->seqNum;
                memcpy(msg, &pHeader, sizeof(pHeader));
                memcpy(msg + sizeof(pHeader), &pHave, sizeof(pHave));//???big or litte???
                //len = sendto(*sendSocket, msg, pHeader.Length, 0, (SOCKADDR *)(&(destIP)), tolen) ;
                len = sendto(*sendSocket, msg, pHeader.Length, 0, (SOCKADDR *)(addr), tolen) ;
                if(len == SOCKET_ERROR)
                {
                    messager(msgERROR, "send to failed, dest ip = %s, msgtype = %d, error number = %d", inet_ntoa(addr->sin_addr), pHeader.Type, myGetLastError());
                }
                else if(len != pHeader.Length)
                {
                    messager(msgERROR, "send to error, dest ip = %s, msgtype = %d, msglen = %d, sendlen = %d", inet_ntoa(addr->sin_addr), pHeader.Type, pHeader.Length, len);
                }
                else
                {
                    messager(msgINFO, "send I_HAVE to %s:%d, consumerUID=%d",
                             inet_ntoa(addr->sin_addr),
                             ntohs(addr->sin_port),
                             hdr->myUID);
                }
            }
            else
            {
            }
            break;
        }
    case PLEASE_START_TRANS:
        {
            ppStart = (pduPleaseStartTrans *)pdu;
            pHeader.Type = START_TRANS;
            pHeader.myUID = globalConfig.UID;
            pHeader.seqNumber = hdr->seqNumber;
            pHeader.Length = sizeof(pHeader) + sizeof(pStartTrans);

            pStartTrans.synStartTransTime = ppStart->s;//s modified by getTheFileIndex;
            pStartTrans.channelID = ppStart->channelID;

            memcpy(msg, &pHeader, sizeof(pHeader));
            memcpy(msg + sizeof(pHeader), &pStartTrans, sizeof(pStartTrans));//???big or litte???
            //len = sendto(*sendSocket, msg, pHeader.Length, 0, (SOCKADDR *)(&(destIP)), tolen) ;
            len = sendto(*sendSocket, msg, pHeader.Length, 0, (SOCKADDR *)(addr), tolen) ;
            if(len == SOCKET_ERROR)
            {
                messager(msgERROR, "send to failed, dest ip = %s, msgtype = %d, error number = %d", inet_ntoa(addr->sin_addr), pHeader.Type, myGetLastError());
            }
            else if(len != pHeader.Length)
            {
                messager(msgERROR, "send to error, dest ip = %s, msgtype = %d, msglen = %d, sendlen = %d", inet_ntoa(addr->sin_addr), pHeader.Type, pHeader.Length, len);
            }
            else
            {
                messager(msgINFO, "send START_TRANS to %s:%d, consumerUID=%d",
                         inet_ntoa(addr->sin_addr),
                         ntohs(addr->sin_port),
                         hdr->myUID
                         );
            }
            break;
        }
    default:
        break;
    }

    return ret;
}



//向提供者或中转者发送重传请求
int sendNextTransForSomeLost(tsConsumerTask *cIter, UINT64 startSeq, UINT64 stopSeq)
{
    int ret = 0;
    char msg[1500];
    int len;
    int tolen = sizeof(SOCKADDR_IN);

    pduHeader pHeader;
    pduNextTrans pNextTrans;
    pHeader.myUID = globalConfig.UID;
#if defined(MOHO_X86)
    pHeader.seqNumber = __sync_add_and_fetch(&globalConfig.seqNumber,1);
#elif defined(MOHO_WIN32)
    pHeader.seqNumber = InterlockedIncrement(&globalConfig.seqNumber);
#endif

    pHeader.Type = NEXT_TRANS;
    pHeader.Length = sizeof(pHeader) + sizeof(pNextTrans) + sizeof(pduResendPair) * cIter->resendDeque.size();

    pNextTrans.channelID = (*cIter).channelID;
    if(startSeq > stopSeq)
    {
        pNextTrans.lastIndex = NEXT_TRANS_LAST_INDEX_FOR_DELETE; //NEED RESEND, PACKET LOST, DELETE IN SUPPLIER TASK;
    }
    else
    {
        pNextTrans.lastIndex = 0;
    }


	if(startSeq > stopSeq)
	{
		pNextTrans.pairNumber = 0;
	}
	else
	{
		pNextTrans.pairNumber = cIter->resendDeque.size();
	}
    pNextTrans.tsUID = cIter->tsUID;

    memcpy(msg, &pHeader, sizeof(pHeader));
    memcpy(msg + sizeof(pHeader), &pNextTrans, sizeof(pNextTrans));
    for(UINT i = 0; i<pNextTrans.pairNumber; i++)
    {
        memcpy(msg + sizeof(pHeader) + sizeof(pNextTrans)+ i*sizeof(pduResendPair), &(cIter->resendDeque[i]), sizeof(pduResendPair));
    }


    if(cIter->Status == ctsStarted)
    {
        SOCKADDR_IN *dest = cIter->getSupplierAddr();
        if(dest != NULL)
        {
            len = sendto(/*localSendSocket*/*(Info.sendSocket), msg, pHeader.Length, 0, (SOCKADDR *)(dest), tolen);
            if(len == SOCKET_ERROR)
            {
                messager(msgERROR, "send to failed, dest ip = %s, msgtype = %d, error number = %d", inet_ntoa(dest->sin_addr), pHeader.Type, myGetLastError());
            }
            else if(len != pHeader.Length)
            {
                messager(msgERROR, "send to error, dest ip = %s, msgtype = %d, msglen = %d, sendlen = %d", inet_ntoa(dest->sin_addr), pHeader.Type, pHeader.Length, len);
            }
            else
            {
                //cIter->tickCount = GetTickCount();
                //cIter->lastResentSeqNum = pNextTrans.resendSeqNum;

                //timeBeginPeriod(SUPPLIER_SEND_SLEEP_TIME);
                //Sleep(FTDS_SLEEP_TIME * 2);
                //timeEndPeriod(SUPPLIER_SEND_SLEEP_TIME);

                messager(msgDEBUG, "send NEXT_TRANS to %s for SOME LOST PACKTET, channelID = %d, pairNumber = %d",
                         inet_ntoa(dest->sin_addr),
                         pNextTrans.channelID,
                         pNextTrans.pairNumber);
            }
        }
    }
    //closesocket(localSendSocket);
    return ret;
}

void sendChannelUpdate(tsSupplierTask *task, int channelEvent)
{
	return;
    SOCKADDR_IN destIP;
    destIP.sin_family = AF_INET;
    char msg[1500];
    int len;
    int tolen = sizeof(destIP);
    pduHeader pHeader;
    pduUpdate pU;
    pU.nodeInfo.UID = globalConfig.UID;
    pU.nodeInfo.type = globalConfig.nodeType;
    pU.nodeInfo.capability = globalConfig.Capability;
    pU.nodeInfo.intranetAddr = globalConfig.addrSrv;

#if defined(MOHO_X86)
    pU.nodeInfo.intranetAddr.sin_addr.s_addr =  globalConfig.ipAddress;
#elif defined(MOHO_WIN32)
    pU.nodeInfo.intranetAddr.sin_addr.S_un.S_addr = globalConfig.ipAddress ;
#endif

    strcpy(pU.nodeInfo.carrier, globalConfig.carrier.c_str());
    strcpy(pU.nodeInfo.province, globalConfig.province.c_str());
    strcpy(pU.nodeInfo.city, globalConfig.city.c_str());
    pU.nodeInfo.currLoad = 0;
#if defined(MOHO_X86)
    pU.nodeInfo.platform = OS_LINUX;
#elif defined(MOHO_WIN32)
    pU.nodeInfo.platform = OS_WINDOWS;
#endif
    pU.channelNumber = 1;
    pduUpdateSingleChannel sc;
    sc.channelID = task->channelID;
    sc.updateEvent = channelEvent;
    pU.nodeInfo.currLoad += task->consumerDeque.size();
    memset(msg, 0, 1500);
    pHeader.Type = LIVECDN_CHANNEL_UPDATE;
#if defined(MOHO_X86)
    pHeader.seqNumber = __sync_add_and_fetch(&globalConfig.seqNumber,1);
#elif defined(MOHO_WIN32)
    pHeader.seqNumber = InterlockedIncrement(&globalConfig.seqNumber);
#endif
    pHeader.myUID = globalConfig.UID;
    pHeader.Length = sizeof(pHeader) + sizeof(pduUpdate) + pU.channelNumber * sizeof(pduUpdateSingleChannel);
    memcpy(msg, &pHeader, sizeof(pHeader));
    memcpy(msg + sizeof(pHeader), &pU, sizeof(pU));
    memcpy(msg + sizeof(pHeader) + sizeof(pU) + sizeof(pduUpdateSingleChannel), &sc, sizeof(pduUpdateSingleChannel));

    for(UINT i = 0; i<globalTrackerDeque.size(); i++)
    {
        destIP.sin_port = htons(globalTrackerDeque[i].servicePort);
#if defined(MOHO_X86)
        pHeader.seqNumber = __sync_add_and_fetch(&globalConfig.seqNumber,1);
        destIP.sin_addr.s_addr = htonl(globalTrackerDeque[i].ipAddress);
#elif defined(MOHO_WIN32)
        pHeader.seqNumber = InterlockedIncrement(&globalConfig.seqNumber);
        destIP.sin_addr.S_un.S_addr = htonl(globalTrackerDeque[i].ipAddress);
#endif
        #if defined(MOHO_WIN32)
        len = sendto(sockPublicationSrv, msg, pHeader.Length, 0, (SOCKADDR *)(&(destIP)), tolen) ;
        if(len == SOCKET_ERROR)
        {
            messager(msgERROR, "send to failed, dest ip = %s, msgtype = %d, error number = %d", inet_ntoa(destIP.sin_addr), pHeader.Type, myGetLastError());
        }
        else if(len != pHeader.Length)
        {
            messager(msgERROR, "send to error, dest ip = %s, msgtype = %d, msglen = %d, sendlen = %d", inet_ntoa(destIP.sin_addr), pHeader.Type, pHeader.Length, len);
        }
        else
        {
            messager(msgINFO, "send update to tracker (%s:%d)", inet_ntoa(destIP.sin_addr), ntohs(destIP.sin_port));
        }
#endif
    }
}

int ReleaseALLChannelFromConsumerTask()
{
    while(1)
    {
        std::list<tsSupplierTask>::iterator sIter;
        for(sIter = globalTsSupplierTaskList.begin(); sIter != globalTsSupplierTaskList.end(); sIter ++)
        {
            int id = sIter->channelID;

            sendChannelUpdate(&(*sIter), evtQuit);
            sendChannelUpdate(&(*sIter), evtQuit);
            sendChannelUpdate(&(*sIter), evtQuit);

            sIter->EXITsupplier = TRUE;
            while(sIter->thread.isCreat() != 0)
            {
                mySleep(100);
            }
             globalTsSupplierTaskList.erase(sIter);
            messager(msgINFO, "All ERASE channel %d from supplier task list", id);
            break;
        }
        break;
    }

    std::list<tsConsumerTask>::iterator cIter;
    for(cIter = globalTsConsumerTaskList.begin(); cIter != globalTsConsumerTaskList.end(); cIter ++)
    {
        int id = cIter->channelID;
        if(cIter->channelID > 0)
        {
            cIter->EXITconsumer = TRUE;
            cIter->EXITftds = TRUE;
            while(cIter->ftdsThread.isCreat() != 0 ||
                  cIter->consumerThread.isCreat() != 0)
            {
                SOCKADDR_IN localIP;
                int len;
                int tolen = sizeof(SOCKADDR_IN);
                char msg[1500];
                localIP.sin_family = AF_INET;
#if defined(MOHO_X86)
                localIP.sin_addr.s_addr = globalConfig.ipAddress;
#elif defined(MOHO_WIN32)
                localIP.sin_addr.S_un.S_addr = globalConfig.ipAddress;
#endif

                localIP.sin_port = htons(cIter->consumerPort);
                strcpy(msg, "SEND TO END THE CONSUMER TASK");
                len = sendto(sockPublicationSrv, msg, strlen(msg), 0, (SOCKADDR *)(&localIP), tolen);
                mySleep(100);
            }
            deleteOldFile((tsConsumerTask*)&(*cIter));
            globalTsConsumerTaskList.erase(cIter++);
            messager(msgINFO, "All ERASE channel %d from consumer task list", id);
        }
    }
    return 0;
}

int ReleaseChannelFromConsumerTask(pduReleaseChannel *ppRelease)
{
    std::list<tsSupplierTask>::iterator sIter;
    for(sIter = globalTsSupplierTaskList.begin(); sIter != globalTsSupplierTaskList.end(); sIter ++)
    {
        if(sIter->channelID == ppRelease->channelID)
        {
            sendChannelUpdate(&(*sIter), evtQuit);
            sendChannelUpdate(&(*sIter), evtQuit);
            sendChannelUpdate(&(*sIter), evtQuit);

            sIter->EXITsupplier = TRUE;
            while(sIter->thread.isCreat() != 0)
            {
                mySleep(100);
            }
             globalTsSupplierTaskList.erase(sIter);
            messager(msgINFO, "ERASE channel %d from supplier task list", ppRelease->channelID);
            break;
        }
    }

    std::list<tsConsumerTask>::iterator cIter;
    for(cIter = globalTsConsumerTaskList.begin(); cIter != globalTsConsumerTaskList.end(); cIter ++)
    {
        if(cIter->channelID == ppRelease->channelID)
        {
            cIter->EXITconsumer = TRUE;
            cIter->EXITftds = TRUE;
            while(cIter->ftdsThread.isCreat() != 0 ||
                  cIter->consumerThread.isCreat() != 0)
            {
                SOCKADDR_IN localIP;
                int len;
                int tolen = sizeof(SOCKADDR_IN);
                char msg[1500];
                localIP.sin_family = AF_INET;
#if defined(MOHO_X86)
                localIP.sin_addr.s_addr = globalConfig.ipAddress;
#elif defined(MOHO_WIN32)
                localIP.sin_addr.S_un.S_addr = globalConfig.ipAddress;
#endif

                localIP.sin_port = htons(cIter->consumerPort);
                strcpy(msg, "SEND TO END THE CONSUMER TASK");
                len = sendto(sockPublicationSrv, msg, strlen(msg), 0, (SOCKADDR *)(&localIP), tolen);
                mySleep(100);
            }
#if defined(MOHO_X86)
            deleteOldFile((tsConsumerTask*)&(*cIter));
#endif
            globalTsConsumerTaskList.erase(cIter);
            messager(msgINFO, "ERASE channel %d from consumer task list", ppRelease->channelID);
            break;
        }
    }
    return 0;
}

int addOneChannel2ConsumerTask(pduGetChannel *ppGet)
{
    std::list<tsConsumerTask>::iterator cIter;
    for(cIter = globalTsConsumerTaskList.begin(); cIter != globalTsConsumerTaskList.end(); cIter ++)
    {
        if(cIter->channelID == ppGet->channelID)
        {
            int64_t diff = _GetTickCount64();
            diff -= cIter->lastPDUTimeTick;

            messager(msgINFO, "channel %d is in consumer task list, add ftds only, diff__%lld,size__%d", ppGet->channelID,diff,cIter->arrivedBufDeque.size());
            ftds ftdsItem;
            ftdsItem.portFTDS = 0;
            //TODO: SHOULD check the same recvAddr
            std::list<ftds>::iterator fIter;
            for(fIter = cIter->ftdsList.begin(); fIter != cIter->ftdsList.end(); fIter ++)
            {
                if(fIter->recvAddr.sin_port == ppGet->recvAddr.sin_port &&
#if defined(MOHO_X86)
                   fIter->recvAddr.sin_addr.s_addr == ppGet->recvAddr.sin_addr.s_addr)
#elif defined(MOHO_WIN32)
                    fIter->recvAddr.sin_addr.S_un.S_addr == ppGet->recvAddr.sin_addr.S_un.S_addr)
#endif
{
                    messager(msgINFO, "channel %d is in ftds task list", ppGet->channelID);
                    break;
                }
            }
            if(fIter == cIter->ftdsList.end())
            {
                messager(msgINFO, "channel %d is not in ftds task list,add ftds", ppGet->channelID);
                ftdsItem.recvAddr = ppGet->recvAddr;
                cIter->ftdsList.push_back(ftdsItem);
            }
            return 1;
        }
    }

#if defined(MOHO_X86)
    ReleaseALLChannelFromConsumerTask();
#endif

    tsConsumerTask tsCT;
    tsCT.channelID = ppGet->channelID;
    char channelName[256];
    char channelPath[256];
    mysprintf(channelName, 256, "%d", ppGet->channelID);

    std::string AppPath;
    AppPath= g_localPath;
    AppPath.append("LiveChannelFiles");
#if defined(MOHO_X86)
    if(-1 == mkdir(AppPath.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) && myGetLastError()!= 17)
    {
        messager(msgERROR, "create directory %s FAILED,error__%d", AppPath.c_str(),myGetLastError());
        AppPath=".";
    }
    else
    {
        messager(msgINFO, "create directory %s SUCCESS or ALREADY_EXISTS", AppPath.c_str());
    }
#elif defined(MOHO_WIN32)
    if (CreateDirectory(AppPath.c_str(),NULL)==0 && myGetLastError()!=ERROR_ALREADY_EXISTS)
    {
        messager(msgERROR, "create directory %s FAILED", AppPath.c_str());
        AppPath=".";
    }
    else
    {
        messager(msgINFO, "create directory %s SUCCESS or ALREADY_EXISTS", AppPath.c_str());
    }
#endif

    AppPath.append("/");
    AppPath.append(channelName);
    AppPath.append("/");
#if defined(MOHO_X86)
    if(-1 == mkdir(AppPath.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) && myGetLastError()!= 17)
    {
        messager(msgERROR, "create directory %s FAILED,error__%d", AppPath.c_str(),myGetLastError());
        AppPath=".";
    }
    else
    {
        messager(msgINFO, "create directory %s SUCCESS or ALREADY_EXISTS", AppPath.c_str());
    }
#elif defined(MOHO_WIN32)
    if (CreateDirectory(AppPath.c_str(),NULL)==0 && myGetLastError()!=ERROR_ALREADY_EXISTS)
    {
        AppPath=".";
        messager(msgERROR, "create directory %s FAILED", AppPath.c_str());
    }
    else
    {
        messager(msgINFO, "create directory %s SUCCESS or ALREADY_EXISTS", AppPath.c_str());
    }
#endif

    mysprintf(channelPath, 256, "%s", AppPath.c_str());


    tsCT.consumerPort = globalConfig.publicationPort + 10000 + ppGet->channelID;
    tsCT.linkNumberReadFromConfigFile = 1;
    tsCT.linkNumberLast = tsCT.linkNumberReadFromConfigFile;
    tsCT.channelName = channelName;
    tsCT.channelPath = channelPath;
    tsCT.bitRate = 0;
    tsCT.lastPDUTime = 0;
    tsCT.pcrPID = 0;
    tsCT.startTime = 0;
    tsCT.supplierUID = 0;
    tsCT.lastSupplierUID = 0;
    tsCT.Status = ctsTrackerQuery;
    tsCT.relayRealSupplierUID = 0;
    tsCT.recvBytes = 0;
    tsCT.lastResentStartSeqNum = 0;
    tsCT.lastResentStopSeqNum = 0;
    tsCT.resendTime = 0;
    tsCT.recvBuf = new char[1500];
    tsCT.arrivedBufList.clear();
    tsCT.fileIndex = 0;
    tsCT.savedFile.clear();
    tsCT.readyForFtds = false;
    tsCT.consumerTimes = 0;
    tsCT.lastPDUTimeTick = 0;
    tsCT.linkNumber = 1;
    tsCT.linkTwoTimeTick = 0;
    tsCT.EXITftds = false;
    tsCT.EXITconsumer = false;
    tsCT.ftdsList.clear();
    tsCT.waitForReplySeq.clear();
    tsCT.recvdFilesNumber = 0;//zhanghh
    tsCT.supplierDeque.clear();
    tsCT.timesInCtsNotStartedStatus = 0;
    tsCT.nextPduSeq = 0;
    deleteOldFile(&tsCT);

    //InitializeCriticalSection(&tsCT.lockConsumerThread);

    CLiveChannel *newChannel = new CLiveChannel(tsCT.channelID, channelName, channelPath, "", 0, 0, channelNormal);
    newChannel->channelBitRate = 0;
    newChannel->pcrPID = 0;
    globalLiveChannelList.push_back(newChannel);
    tsCT.channel = newChannel;

    globalTsConsumerTaskList.push_back(tsCT);
    globalTsConsumerTaskList.back().lockConsumerArrivedDeque.init();
    logger(msgINFO, "ADD channel %d to consumer task list, size = %d", globalTsConsumerTaskList.back().channelID, globalTsConsumerTaskList.size());

    ftds ftdsItem;
    //ftdsItem.ipAddressFTDS = ppGet->recvAddr.sin_addr.S_un;
    //ftdsItem.portFTDS = ppGet->recvAddr.sin_port;
    ftdsItem.portFTDS = 0;
    ftdsItem.recvAddr = ppGet->recvAddr;
    for(cIter = globalTsConsumerTaskList.begin(); cIter != globalTsConsumerTaskList.end(); cIter ++)
    {
        if(cIter->channelID == ppGet->channelID)
        {
            cIter->ftdsList.push_back(ftdsItem);
            break;
        }
    }
    if(cIter == globalTsConsumerTaskList.end())
    {

        logger(msgERROR, "FTDS (%d, %s, %s) has no channel", ppGet->channelID, inet_ntoa(ppGet->recvAddr.sin_addr), ntohs(ppGet->recvAddr.sin_port));
    }
    return 0;
}

//资源发布线程，信令交互
#if defined(MOHO_X86)
void* channelPublicationThreadProc(LPVOID lpParameter)
#elif defined(MOHO_WIN32)
        static DWORD WINAPI channelPublicationThreadProc(LPVOID lpParameter)
#endif
{
#if defined(MOHO_X86)
    signal(SIGUSR1, probeThreadExit);
#endif
    channelPublicationThreadInfo * Info = (channelPublicationThreadInfo *)lpParameter;
    char recvBuf[1500] ;


    SOCKADDR_IN addrClient ;
    int addrClientLen = sizeof(addrClient);

    pduIAmOk pOk;
    pduIHave pHave;
    pduHowAreYou pHow;
    pduDoYouHave pYouHave;
    pduPleaseStartTrans pPleaseStart;
    pduStartTrans pStart;
    pduNextTrans pNextTrans;
    std::list<tsSupplierTask>::iterator sIter;

    while (1)
    {
        pduHeader hdr;
        memset(recvBuf, 0 , 1500);
        int len = recvfrom(*Info->sockSrv, recvBuf, 1500, 0, (SOCKADDR*)&addrClient,(socklen_t*)&addrClientLen) ;
        if(len == SOCKET_ERROR)
        {
            messager(msgERROR, "recvfrom %s error on port %d, error number: %d", 
                     inet_ntoa(addrClient.sin_addr),
                     globalConfig.publicationPort,
                     myGetLastError());
        }
        if(len >= sizeof(hdr))
        {
            hdr = *((pduHeader *)recvBuf);
            if(len != hdr.Length)
            {
                messager(msgERROR, "recvfrom %s len = %d, hdr.Length = %d, not equal!",inet_ntoa(addrClient.sin_addr), len, hdr.Length);
            }
            else if(len == hdr.Length)
            {
                switch(hdr.Type)
                {
                case GET_CHANNEL:
                    {
                        pduGetChannel pGet;
                        pGet = *(pduGetChannel *)(recvBuf + sizeof(hdr));
                        std::string from = inet_ntoa(addrClient.sin_addr);
                        std::string ipRecvAddr = inet_ntoa(pGet.recvAddr.sin_addr);
                        messager(msgINFO, "recv GET_CHANNEL from %s, channelID = %d, (%s:%d)",
                                 from.c_str(),
                                 pGet.channelID,
                                 ipRecvAddr.c_str(),
                                 ntohs(pGet.recvAddr.sin_port));
                        addOneChannel2ConsumerTask(&pGet);
                        break;
                    }
                case RELEASE_CHANNEL:
                    {
                        pduReleaseChannel pRelease;
                        pRelease = *(pduReleaseChannel *)(recvBuf + sizeof(hdr));
                        std::string from = inet_ntoa(addrClient.sin_addr);
                        std::string ipRecvAddr = inet_ntoa(pRelease.recvAddr.sin_addr);
                        messager(msgINFO, "recv RELEASE_CHANNEL from %s, channelID = %d, (%s:%d)",
                                 from.c_str(),
                                 pRelease.channelID,
                                 ipRecvAddr.c_str(),
                                 ntohs(pRelease.recvAddr.sin_port));
                        ReleaseChannelFromConsumerTask(&pRelease);
                        break;
                    }
                case LIVECDN_NOTIFY_CHANNEL_QUERY:
                    {
                        pduNotifyQuery pNQ;
                        pNQ = *(pduNotifyQuery *)(recvBuf + sizeof(hdr));
                        std::string from = inet_ntoa(addrClient.sin_addr);
                        std::string queryNode = inet_ntoa(pNQ.queryNode.internetAddr.sin_addr);
                        messager(msgINFO, "recv LIVECDN_NOTIFY_CHANNEL_QUERY from %s, queryNode(%s:%d), channelID is %d",
                                 from.c_str(),
                                 queryNode.c_str(),
                                 ntohs(pNQ.queryNode.internetAddr.sin_port),
                                 pNQ.channleID);
                        std::string internetAddr = inet_ntoa(globalConfig.internetAddr.sin_addr);
                        if(queryNode.compare(internetAddr) == 0)
                        {
                            sayHello(&(pNQ.queryNode.intranetAddr));
                        }
                        else
                        {
                            sayHello(&(pNQ.queryNode.internetAddr));
                        }
                        break;
                    }
                case LIVECDN_REPLY_CHANNEL_QUERY:
                    {
                        pduReplyQuery pRQ;
                        pduReplyQuerySingleSupplier pS;
                        pRQ = *(pduReplyQuery *)(recvBuf + sizeof(hdr));
                        std::string from;
                        std::string internetAddr;
                        from = inet_ntoa(addrClient.sin_addr);
                        internetAddr = inet_ntoa(pRQ.queryNode.internetAddr.sin_addr);
                        messager(msgINFO, "recv LIVECDN_REPLY_CHANNEL_QUERY from %s, channel = %d, number = %d, queryNode(%s, %d)",
                                 from.c_str(),
                                 pRQ.channelID,
                                 pRQ.supplierNumber,
                                 internetAddr.c_str(),
                                 ntohs(pRQ.queryNode.internetAddr.sin_port));
                        globalConfig.internetAddr = pRQ.queryNode.internetAddr;
                        std::list<tsConsumerTask>::iterator cIter;
                        for(cIter = globalTsConsumerTaskList.begin(); cIter != globalTsConsumerTaskList.end(); cIter ++)
                        {
                            if(cIter->channelID == pRQ.channelID && cIter->Status == ctsTrackerQuery)
                            {

                                cIter->supplierDeque.clear();
                                for(int i = 0; i<pRQ.supplierNumber; i++)
                                {
                                    pS = *(pduReplyQuerySingleSupplier *)(recvBuf + sizeof(hdr) + sizeof(pRQ) + i * sizeof(pduReplyQuerySingleSupplier));
                                    neighborNode nn;
                                    nn.UID = pS.supplierUID;
                                    nn.publicationAddr = pS.internetAddr;
                                    nn.intranetAddr = pS.intranetAddr;
                                    nn.dataTransferAddr = pS.internetAddr;
                                    nn.ipAddress = 0;
                                    nn.Status = nnsUnknown;
                                    cIter->supplierDeque.push_back(nn);
                                    std::string sIntra = inet_ntoa(nn.intranetAddr.sin_addr);
                                    std::string sInter = inet_ntoa(nn.publicationAddr.sin_addr);
                                    messager(msgINFO, "insert new neighbor UID = %d, internetIP = (%s : %d), intranetIP = (%s : %d)",
                                             nn.UID,
                                             sInter.c_str(),
                                             ntohs(nn.publicationAddr.sin_port),
                                             sIntra.c_str(),
                                             ntohs(nn.intranetAddr.sin_port));
                                }

                                if(cIter->supplierDeque.size() > 0)
                                {
                                    cIter->Status = ctsNotStarted;
                                    cIter->sayHello2Supplier();
                                }
                                break;
                            }
                            else if(cIter->channelID == pRQ.channelID && cIter->Status > ctsTrackerQuery)
                            {
                                messager(msgINFO, "channel %d recv LIVECDN_REPLY_CHANNEL_QUERY when status is %d",
                                         cIter->channelID,
                                         cIter->Status);
                            }
                        }
                        break;
                    }
                case HOW_ARE_YOU:
                    {
                        pHow = *(pduHowAreYou *)(recvBuf + sizeof(hdr));
                        messager(msgINFO, "recv HOW_ARE_YOU from %s:%d, pHow->UID = %d, pHeader->UID = %d, seq = %d",
                                 inet_ntoa(addrClient.sin_addr),
                                 ntohs(addrClient.sin_port),
                                 pHow.myUID,
                                 hdr.myUID,
                                 hdr.seqNumber);
                        updateNeighborList(&pHow, &addrClient);
                        replyTo(&hdr, &pHow, getNeighborPublicationAddr(hdr.myUID, &addrClient), Info->sendSocket);
                        //send reply to the addrClient
                        break;
                    }
                case I_AM_OK:
                    {
                        pOk = *(pduIAmOk *)(recvBuf + sizeof(hdr));
                        messager(msgDEBUG, "recv I_AM_OK from %s:%d, SupplyPort = %d, ZoneNumber = %d, Capability = %d, Load = %d, seq = %d",
                                 inet_ntoa(addrClient.sin_addr),
                                 ntohs(addrClient.sin_port),
                                 pOk.mySupplyPort,
                                 pOk.myZoneNumber,
                                 pOk.myCapability,
                                 pOk.myLoad,
                                 hdr.seqNumber);
                        //updateNeighborStatus(&hdr, &addrClient);
                        //checkConsumer();
                        break;
                    }
                case DO_YOU_HAVE:
                    {
                        pYouHave = *(pduDoYouHave *)(recvBuf + sizeof(hdr));
                        messager(msgINFO, "recv DO_YOU_HAVE from %s:%d, consumerUID = %d, supplierUID = %d, channelID = %d, s = %ld, e = %ld",
                                 inet_ntoa(addrClient.sin_addr),
                                 ntohs(addrClient.sin_port),
                                 pYouHave.consumerUID,
                                 pYouHave.supplierUID,
                                 pYouHave.channelID,
                                 pYouHave.s,
                                 pYouHave.e);
                        replyTo(&hdr, &pYouHave, getNeighborPublicationAddr(hdr.myUID, &addrClient), Info->sendSocket);
                        break;
                    }
                case I_HAVE:
                    {
                        pHave = *(pduIHave *)(recvBuf + sizeof(hdr));
                        if(pHave.YerOrNo)
                        {
                            messager(msgINFO, "recv I_HAVE from %s:%d, supplierUID = %d, channelID = %d, tsUID = %d, mostRecentSeqNum = %lld",
                                     inet_ntoa(addrClient.sin_addr),
                                     ntohs(addrClient.sin_port),
                                     pHave.UID,
                                     pHave.channelID,
                                     pHave.tsUID,
                                     pHave.mostRecentSeqNum);
                            if(updateConsumerTask(&hdr, &pHave, &addrClient) != 0)
                            {
                                //checkConsumer();
                            }
                        }
                        break;
                    }
                case PLEASE_START_TRANS:
                    {
                        pPleaseStart = *(pduPleaseStartTrans *)(recvBuf + sizeof(hdr));
                        messager(msgINFO, "recv PLEASE_START_TRANS from %s:%d, channelID = %d, start_time = %ld, port=%d",
                                 inet_ntoa(addrClient.sin_addr),
                                 ntohs(addrClient.sin_port),
                                 pPleaseStart.channelID,
                                 pPleaseStart.s,
                                 pPleaseStart.consumerPort);
                        if(updateSupplierTask(&hdr, &pPleaseStart, &addrClient) > 0)
                        {
                            replyTo(&hdr, &pPleaseStart,getNeighborPublicationAddr(hdr.myUID, &addrClient), Info->sendSocket);
                        }
                        break;
                    }
                case START_TRANS:
                    {
                        pStart = *(pduStartTrans *)(recvBuf + sizeof(hdr));
                        //pStart.synStartTransTime = buf2Time_t(recvBuf + 13);
                        messager(msgINFO, "recv START_TRANS from %s:%d, start_time = %ld",
                                 inet_ntoa(addrClient.sin_addr),
                                 ntohs(addrClient.sin_port),
                                 pStart.synStartTransTime);
                        updateConsumerTaskA(&hdr, &pStart, &addrClient);
                        //updateRelayTaskA(&hdr, &pStart, &addrClient);
                        break;
                    }
                case NEXT_TRANS:
                    {
                        //thread
                        //pNextTrans.channelID = buf2Uint(recvBuf + 13);
                        //pNextTrans.lastIndex = recvBuf[17];
                        pNextTrans = *(pduNextTrans *)(recvBuf + sizeof(hdr));
                        nextTransWithPair nextTrans;
                        pduResendPair pair;
                        messager(msgINFO, "recv NEXT_TRANS from %s:%d, channelID = %d, lastIndex = %d, pairNumber = %d",
                                 inet_ntoa(addrClient.sin_addr),
                                 ntohs(addrClient.sin_port),
                                 pNextTrans.channelID,
                                 pNextTrans.lastIndex,
                                 pNextTrans.pairNumber);
                        lockTsSupplierTaskList.lock();
                        for(sIter = globalTsSupplierTaskList.begin(); sIter != globalTsSupplierTaskList.end(); sIter ++)
                        {
                            if((*sIter).channelID == pNextTrans.channelID)
                            {
                                if(pNextTrans.tsUID != sIter->tsUID)
                                {
                                    break;
                                }

                                sIter->lockDeleteQuitConsumer.lock();
                                for(UINT k = 0; k<sIter->consumerDeque.size(); k++)
                                {
                                    if(sIter->consumerDeque[k].UID == hdr.myUID)
                                    {
                                        sIter->consumerDeque[k].lockConsumerNextTransDeque.lock();
                                        nextTrans.waitNextTrans = pNextTrans;
                                        for(UINT j = 0; j<pNextTrans.pairNumber; j++)
                                        {
                                            pair = *(pduResendPair *)(recvBuf + sizeof(hdr) + sizeof(pNextTrans) + sizeof(pduResendPair) * j);
                                            nextTrans.resendDeque.push_back(pair);
                                        }
                                        sIter->consumerDeque[k].nextTrans.push_back(nextTrans);
                                        sIter->consumerDeque[k].lockConsumerNextTransDeque.unlock();
                                        break;
                                    }
                                }
                                sIter->lockDeleteQuitConsumer.unlock();
                            }
                        }

                        lockTsSupplierTaskList.unlock();

                        //relayNextTrans(&hdr, &pNextTrans);2013-0705 by xieshengluo

                        break;
                    }
                default:
                    break;
                }
            }
        }
    }
#if defined(MOHO_X86)
    close(*Info->sockSrv);
#elif defined(MOHO_WIN32)
    closesocket(*Info->sockSrv);
#endif

#if defined(MOHO_X86)
    return NULL;
#elif defined(MOHO_WIN32)
    return 0;
#endif
}

void sayHello(SOCKADDR_IN *dest)
{
    if(dest == NULL)
    {
        messager(msgERROR, "say hello, dest = NULL");
        return;
    }
    char msg[1500];
    int len;
    int tolen = sizeof(*dest);

    std::list<neighborNode>::iterator nIter;
    pduHeader pHeader;
    pduHowAreYou pHowAreYou;

    pHeader.Type = HOW_ARE_YOU;
    pHeader.myUID = globalConfig.UID;
#if defined(MOHO_X86)
    pHeader.seqNumber = __sync_add_and_fetch(&globalConfig.seqNumber,1);
#elif defined(MOHO_WIN32)
    pHeader.seqNumber = InterlockedIncrement(&globalConfig.seqNumber);
#endif
    pHeader.Length = sizeof(pHeader) + sizeof(pHowAreYou);
    pHowAreYou.myUID = globalConfig.UID;


    memcpy(msg, &pHeader, sizeof(pHeader));
    memcpy(msg + sizeof(pHeader), &pHowAreYou, sizeof(pHowAreYou));//???big or litte???
    len = sendto(sockPublicationSrv, msg, pHeader.Length, 0, (SOCKADDR *)(dest), tolen) ;
    if(len == SOCKET_ERROR)
    {
        messager(msgERROR, "send to failed, dest ip = %s, msgtype = %d, error number = %d", inet_ntoa(dest->sin_addr), pHeader.Type, myGetLastError());
    }
    else if(len != pHeader.Length)
    {
        messager(msgERROR, "send to error, dest ip = %s, msgtype = %d, msglen = %d, sendlen = %d", inet_ntoa(dest->sin_addr), pHeader.Type, pHeader.Length, len);
    }
    else
    {
        {
            messager(msgDEBUG, "send HOW_ARE_YOU to dest ip = %s:%d, seq = %d",
                     inet_ntoa(dest->sin_addr),
                     ntohs(dest->sin_port),
                     globalConfig.seqNumber);
        }
    }

}

void sayHello2EveryOne()
{
    return;
    SOCKADDR_IN destIP;
    destIP.sin_family = AF_INET;
    destIP.sin_port = htons(globalConfig.publicationPort);
    char msg[1500];
    int len;
    int tolen = sizeof(destIP);

    std::list<neighborNode>::iterator nIter;
    pduHeader pHeader;
    pduHowAreYou pHowAreYou;

    lockNeighborNodeList.lock();
    for(nIter = globalNeighborNodeList.begin(); nIter != globalNeighborNodeList.end(); nIter ++)
    {
        pHeader.Type = HOW_ARE_YOU;
        pHeader.myUID = globalConfig.UID;
#if defined(MOHO_X86)
        pHeader.seqNumber = __sync_add_and_fetch(&globalConfig.seqNumber,1);
#elif defined(MOHO_WIN32)
        pHeader.seqNumber = InterlockedIncrement(&globalConfig.seqNumber);
#endif
        pHeader.Length = sizeof(pHeader) + sizeof(pHowAreYou);
        pHowAreYou.myUID = globalConfig.UID;
#if defined(MOHO_X86)
        destIP.sin_addr.s_addr = htonl((*nIter).ipAddress);
#elif defined(MOHO_WIN32)
        destIP.sin_addr.S_un.S_addr = htonl((*nIter).ipAddress);
#endif

        memcpy(msg, &pHeader, sizeof(pHeader));
        memcpy(msg + sizeof(pHeader), &pHowAreYou, sizeof(pHowAreYou));//???big or litte???
        if(nIter->ipAddress == 0)
        {
            len = sendto(sockPublicationSrv, msg, pHeader.Length, 0, (SOCKADDR *)(&(nIter->publicationAddr)), tolen) ;
        }
        else
        {
            len = sendto(sockPublicationSrv, msg, pHeader.Length, 0, (SOCKADDR *)(&(destIP)), tolen) ;
        }
        if(len == SOCKET_ERROR)
        {
            messager(msgERROR, "send to failed, dest ip = %s, msgtype = %d, error number = %d", inet_ntoa(destIP.sin_addr), pHeader.Type, myGetLastError());
        }
        else if(len != pHeader.Length)
        {
            messager(msgERROR, "send to error, dest ip = %s, msgtype = %d, msglen = %d, sendlen = %d", inet_ntoa(destIP.sin_addr), pHeader.Type, pHeader.Length, len);
        }
        else
        {
            if(nIter->ipAddress == 0)
            {
                messager(msgINFO, "send HOW_ARE_YOU to dest ip = %s", inet_ntoa(nIter->publicationAddr.sin_addr));
            }
            else
            {
                messager(msgINFO, "send HOW_ARE_YOU to dest ip = %s", inet_ntoa(destIP.sin_addr));
            }
        }
    }
    lockNeighborNodeList.unlock();

}

#if defined(MOHO_X86)
void taskNeighborNodeProcesser(union sigval v)
#elif defined(MOHO_WIN32)
        VOID CALLBACK taskNeighborNodeProcesser(
                HWND hwnd,        // handle to window for timer messages
                UINT message,     // WM_TIMER message
                UINT idTimer,     // timer identifier
                DWORD dwTime)     // current system time
#endif
{
#if defined(MOHO_X86)
    timer_t* timeid = (timer_t*)v.sival_ptr;
    if(*timeid != g_neighbor_process_timeid)
    {
        messager(msgINFO, "Not the taskNeighborNodeProcesser timer ID, timeID = %p", *timeid);
        return;
    }
#elif defined(MOHO_WIN32)
    if(idTimer != IDT_NEIGHBOR_PROCESS_TASK)
    {
        messager(msgINFO, "Not the taskNeighborNodeProcesser timer ID, timeID = %d", idTimer);
        return;
    }
#endif
    SOCKADDR_IN destIP;
    //    destIP.sin_family = AF_INET;
    //    destIP.sin_port = htons(globalConfig.publicationPort);
    //    char msg[1500];
    //    int len;
    //    int tolen = sizeof(destIP);
    //
    //    std::list<neighborNode>::iterator nIter;
    //    pduHeader pHeader;
    //    pduHowAreYou pHowAreYou;
    //    lockNeighborNodeList.lock();
    //    for(nIter = globalNeighborNodeList.begin(); nIter != globalNeighborNodeList.end(); nIter ++)
    //    {
    //        pHeader.Type = HOW_ARE_YOU;
    //        pHeader.myUID = globalConfig.UID;
    //#if defined(MOHO_X86)
    //        pHeader.seqNumber = __sync_add_and_fetch(&globalConfig.seqNumber,1);
    //#elif defined(MOHO_WIN32)
    //        pHeader.seqNumber = InterlockedIncrement(&globalConfig.seqNumber);
    //#endif
    //        pHeader.Length = sizeof(pHeader) + sizeof(pHowAreYou);
    //        pHowAreYou.myUID = globalConfig.UID;
    //#if defined(MOHO_X86)
    //        destIP.sin_addr.s_addr = htonl((*nIter).ipAddress);
    //#elif defined(MOHO_WIN32)
    //        destIP.sin_addr.S_un.S_addr = htonl((*nIter).ipAddress);
    //#endif
    //
    //
    //        memcpy(msg, &pHeader, sizeof(pHeader));
    //        memcpy(msg + sizeof(pHeader), &pHowAreYou, sizeof(pHowAreYou));//???big or litte???
    //        if(nIter->ipAddress == 0)
    //        {
    //            len = sendto(sockPublicationSrv, msg, pHeader.Length, 0, (SOCKADDR *)(&(nIter->publicationAddr)), tolen) ;
    //        }
    //        else
    //        {
    //            len = sendto(sockPublicationSrv, msg, pHeader.Length, 0, (SOCKADDR *)(&(destIP)), tolen) ;
    //        }
    //        if(len == SOCKET_ERROR)
    //        {
    //            messager(msgERROR, "send to failed, dest ip = %s, msgtype = %d, error number = %d", inet_ntoa(destIP.sin_addr), pHeader.Type, myGetLastError());
    //        }
    //        else if(len != pHeader.Length)
    //        {
    //            messager(msgERROR, "send to error, dest ip = %s, msgtype = %d, msglen = %d, sendlen = %d", inet_ntoa(destIP.sin_addr), pHeader.Type, pHeader.Length, len);
    //        }
    //        else
    //        {
    //            if(nIter->ipAddress == 0)
    //            {
    //                messager(msgINFO, "send HOW_ARE_YOU to dest ip = %s", inet_ntoa(nIter->publicationAddr.sin_addr));
    //            }
    //            else
    //            {
    //                messager(msgINFO, "send HOW_ARE_YOU to dest ip = %s", inet_ntoa(destIP.sin_addr));
    //            }
    //        }
    //        messager(msgDEBUG, "(*nIter).Status__%d", (*nIter).Status);
    std::list<neighborNode>::iterator nIter;
    lockNeighborNodeList.lock();
    for(nIter = globalNeighborNodeList.begin(); nIter != globalNeighborNodeList.end(); nIter ++)
    {
        if((*nIter).Status > 0 && nIter->Status != nnsUnknown)
        {
            (*nIter).Status --;
        }
        if(nIter->Status == nnsDead)
        {
            messager(msgINFO, "erase UID=%d ip=%s from neighborlist", nIter->UID, inet_ntoa((nIter->publicationAddr).sin_addr));
            nIter = globalNeighborNodeList.erase(nIter);
            if(nIter != globalNeighborNodeList.begin())
            {
                --nIter;
            }
            if(nIter == globalNeighborNodeList.end())
            {
                break;
            }
        }
    }
    lockNeighborNodeList.unlock();


    destIP.sin_family = AF_INET;
    char msg[1500];
    int len;
    int tolen = sizeof(destIP);
    pduHeader pHeader;
    pduUpdate pU;
    pU.nodeInfo.UID = globalConfig.UID;
    pU.nodeInfo.type = globalConfig.nodeType;
    pU.nodeInfo.capability = globalConfig.Capability;
    pU.nodeInfo.intranetAddr = globalConfig.addrSrv;

#if defined(MOHO_X86)
    pU.nodeInfo.intranetAddr.sin_addr.s_addr =  globalConfig.ipAddress;
#elif defined(MOHO_WIN32)
    pU.nodeInfo.intranetAddr.sin_addr.S_un.S_addr = globalConfig.ipAddress ;
#endif

    strcpy(pU.nodeInfo.carrier, globalConfig.carrier.c_str());
    strcpy(pU.nodeInfo.province, globalConfig.province.c_str());
    strcpy(pU.nodeInfo.city, globalConfig.city.c_str());
    pU.nodeInfo.currLoad = 0;
#if defined(MOHO_X86)
    pU.nodeInfo.platform = OS_LINUX;
#elif defined(MOHO_WIN32)
    pU.nodeInfo.platform = OS_WINDOWS;
#endif
    std::deque<pduUpdateSingleChannel> updateEventDeque;
    updateEventDeque.clear();
    std::list<tsSupplierTask>::iterator sIter;
    lockTsSupplierTaskList.lock();
    pU.channelNumber = globalTsSupplierTaskList.size();
    for(sIter = globalTsSupplierTaskList.begin(); sIter != globalTsSupplierTaskList.end(); sIter ++)
    {
        pduUpdateSingleChannel sc;
        sc.channelID = sIter->channelID;
        sc.updateEvent = sIter->update2TrackerEvent;
        updateEventDeque.push_back(sc);
        pU.nodeInfo.currLoad += sIter->consumerDeque.size();
    }
    lockTsSupplierTaskList.unlock();
    if(updateEventDeque.size() > 0)
    {
        memset(msg, 0, 1500);
        pHeader.Type = LIVECDN_CHANNEL_UPDATE;
#if defined(MOHO_X86)
        pHeader.seqNumber = __sync_add_and_fetch(&globalConfig.seqNumber,1);
#elif defined(MOHO_WIN32)
        pHeader.seqNumber = InterlockedIncrement(&globalConfig.seqNumber);
#endif
        pHeader.myUID = globalConfig.UID;
        pHeader.Length = sizeof(pHeader) + sizeof(pduUpdate) + pU.channelNumber * sizeof(pduUpdateSingleChannel);
        memcpy(msg, &pHeader, sizeof(pHeader));
        memcpy(msg + sizeof(pHeader), &pU, sizeof(pU));
        for(UINT i = 0; i<updateEventDeque.size(); i++)
        {
            memcpy(msg + sizeof(pHeader) + sizeof(pU) + i*sizeof(pduUpdateSingleChannel), &updateEventDeque[i], sizeof(pduUpdateSingleChannel));
        }

        for(UINT i = 0; i<globalTrackerDeque.size(); i++)
        {
            destIP.sin_port = htons(globalTrackerDeque[i].servicePort);
#if defined(MOHO_X86)
            pHeader.seqNumber = __sync_add_and_fetch(&globalConfig.seqNumber,1);
            destIP.sin_addr.s_addr = htonl(globalTrackerDeque[i].ipAddress);
#elif defined(MOHO_WIN32)
            pHeader.seqNumber = InterlockedIncrement(&globalConfig.seqNumber);
            destIP.sin_addr.S_un.S_addr = htonl(globalTrackerDeque[i].ipAddress);
#endif
            #if defined(MOHO_WIN32)
            len = sendto(sockPublicationSrv, msg, pHeader.Length, 0, (SOCKADDR *)(&(destIP)), tolen) ;
            if(len == SOCKET_ERROR)
            {
                messager(msgERROR, "send to failed, dest ip = %s, msgtype = %d, error number = %d", inet_ntoa(destIP.sin_addr), pHeader.Type, myGetLastError());
            }
            else if(len != pHeader.Length)
            {
                messager(msgERROR, "send to error, dest ip = %s, msgtype = %d, msglen = %d, sendlen = %d", inet_ntoa(destIP.sin_addr), pHeader.Type, pHeader.Length, len);
            }
            else
            {
                messager(msgINFO, "send update to tracker (%s:%d)", inet_ntoa(destIP.sin_addr), ntohs(destIP.sin_port));
            }
#endif
        }
    }

    std::list<tsConsumerTask>::iterator cIter;
    for(cIter = globalTsConsumerTaskList.begin(); cIter != globalTsConsumerTaskList.end(); cIter ++)
    {
        cIter->sayHello2Supplier();
    }
}



//消费者时钟中断处理函数
#if defined(MOHO_X86)
void taskConsumerProcesser(union sigval v)
#elif defined(MOHO_WIN32)
        VOID CALLBACK taskConsumerProcesser(  //all consumer tasks use only one timer, maybe???
                HWND hwnd,        // handle to window for timer messages
                UINT message,     // WM_TIMER message
                UINT idTimer,     // timer identifier
                DWORD dwTime)     // current system time
#endif
{
#if defined(MOHO_X86)
    timer_t* timeid = (timer_t*)v.sival_ptr;
    if(*timeid != g_consumer_process_timeid)
    {
        messager(msgINFO, "Not the taskConsumerProcesser timer ID, timeID = %p", *timeid);
        return;
    }
#elif defined(MOHO_WIN32)
    if(idTimer != IDT_CONSUMER_PROCESS_TASK)
    {
        messager(msgINFO, "Not the taskConsumerProcesser timer ID, timeID = %d", idTimer);
        return;
    }
#endif
    checkConsumer();
    return;
}

void sendChannelQuery(SOCKET *sock, tsConsumerTask *cIter)
{
    SOCKADDR_IN destIP;
    destIP.sin_family = AF_INET;
    char msg[1500];
    int len;
    int tolen = sizeof(SOCKADDR_IN);
    pduHeader pHeader;
    pduQuery pQ;
    pQ.nodeInfo.UID = globalConfig.UID;
    pQ.nodeInfo.type = globalConfig.nodeType;
    pQ.nodeInfo.capability = globalConfig.Capability;
    pQ.nodeInfo.intranetAddr = globalConfig.addrSrv;
 
#if defined(MOHO_X86)
    pQ.nodeInfo.intranetAddr.sin_addr.s_addr =  globalConfig.ipAddress;
#elif defined(MOHO_WIN32)
    pQ.nodeInfo.intranetAddr.sin_addr.S_un.S_addr = globalConfig.ipAddress ;
#endif

    strcpy(pQ.nodeInfo.carrier, globalConfig.carrier.c_str());
    strcpy(pQ.nodeInfo.province, globalConfig.province.c_str());
    strcpy(pQ.nodeInfo.city, globalConfig.city.c_str());
    pQ.nodeInfo.currLoad = 0;
#if defined(MOHO_X86)
    pQ.nodeInfo.platform = OS_LINUX;
#elif defined(MOHO_WIN32)
    pQ.nodeInfo.platform = OS_WINDOWS;
#endif
    pQ.channelID = cIter->channelID;
    memset(msg, 0, 1500);
    pHeader.Type = LIVECDN_CHANNEL_QUERY;
#if defined(MOHO_X86)
    pHeader.seqNumber = __sync_add_and_fetch(&globalConfig.seqNumber,1);
#elif defined(MOHO_WIN32)
    pHeader.seqNumber = InterlockedIncrement(&globalConfig.seqNumber);
#endif
    pHeader.myUID = globalConfig.UID;
    pHeader.Length = sizeof(pHeader) + sizeof(pQ);
    memcpy(msg, &pHeader, sizeof(pHeader));
    memcpy(msg + sizeof(pHeader), &pQ, sizeof(pQ));
    for(UINT i = 0; i<globalTrackerDeque.size(); i++)
    {
        destIP.sin_port = htons(globalTrackerDeque[i].servicePort);
#if defined(MOHO_X86)
        destIP.sin_addr.s_addr = htonl(globalTrackerDeque[i].ipAddress);
#elif defined(MOHO_WIN32)
        destIP.sin_addr.S_un.S_addr = htonl(globalTrackerDeque[i].ipAddress);
#endif
        //len = sendto(sockPublicationSrv, msg, pHeader.Length, 0, (SOCKADDR *)(&(destIP)), tolen) ;
        len = sendto(*sock, msg, pHeader.Length, 0, (SOCKADDR *)(&destIP), tolen) ;
        if(len == SOCKET_ERROR)
        {
            messager(msgERROR, "send to failed, dest ip = %s, msgtype = %d, error number = %d", inet_ntoa(destIP.sin_addr), pHeader.Type, myGetLastError());
        }
        else if(len != pHeader.Length)
        {
            messager(msgERROR, "send to error, dest ip = %s, msgtype = %d, msglen = %d, sendlen = %d", inet_ntoa(destIP.sin_addr), pHeader.Type, pHeader.Length, len);
        }
        else
        {
            messager(msgINFO, "send query of channel %d to tracker (%s:%d)",
                     pQ.channelID,
                     inet_ntoa(destIP.sin_addr),
                     ntohs(destIP.sin_port));
        }
    }

}

void checkConsumer()
{
    //PostMessage(pMainWnd,WM_TIMER, IDT_CONSUMER_PROCESS_TASK, (LPARAM)taskConsumerProcesser);
    SOCKADDR_IN destIP;
    destIP.sin_family = AF_INET;
    destIP.sin_port = htons(globalConfig.publicationPort);
    char msg[1500];
    int len;
    int tolen = sizeof(destIP);

    std::list<tsConsumerTask>::iterator cIter;
    //start2checkconsumer:
    for(cIter = globalTsConsumerTaskList.begin(); cIter != globalTsConsumerTaskList.end(); cIter ++)
    {
        if(cIter->Status == ctsTrackerQuery)
        {
            sendChannelQuery(&sockPublicationSrv, &(*cIter));
        }
        else if(cIter->Status == ctsNotStarted)
        {
            if(cIter->checkCtsNotStartedStatus() == 0)
            {
                messager(msgINFO, "consumer channel %d in status ctsNotStarted too many times, re-query-tracker!", cIter->channelID);
                cIter->Status = ctsTrackerQuery;
                //goto start2checkconsumer;
            }
            else
            {
                std::list<neighborNode>::iterator nIter;
                pduHeader pHeader;
                pduDoYouHave pYouHave;
                time_t s;
                time(&s);
                s -= (TS_CONSUMER_START_TIME_OFFSET * 60);
                cIter->fileIndex = 0;
                for(UINT i = 0; i<cIter->supplierDeque.size(); i++)
                {

                    //sayHello(cIter->getPossibleSupplierAddr(i));

                    pHeader.myUID = globalConfig.UID;
#if defined(MOHO_X86)
                    pHeader.seqNumber = __sync_add_and_fetch(&globalConfig.seqNumber,1);
#elif defined(MOHO_W32)
                    pHeader.seqNumber =  InterlockedIncrement(&globalConfig.seqNumber);
#endif
                    pHeader.Type = DO_YOU_HAVE;
                    pHeader.Length = sizeof(pHeader) + sizeof(pYouHave);

                    pYouHave.channelID = (*cIter).channelID;
                    pYouHave.consumerUID = globalConfig.UID;
                    pYouHave.supplierUID = cIter->supplierDeque[i].UID;

                    pYouHave.s = s;
                    if(cIter->Status == ctsLinkError)
                    {
                        pYouHave.s = 0;
                    }
                    pYouHave.e = 0;

                    memcpy(msg, &pHeader, sizeof(pHeader));
                    memcpy(msg + sizeof(pHeader), &pYouHave, sizeof(pYouHave));

                    len = sendto(sockPublicationSrv, msg, pHeader.Length, 0, (SOCKADDR *)(cIter->getPossibleSupplierAddr(i)), tolen);
                    if(len == SOCKET_ERROR)
                    {
                        messager(msgERROR, "send to failed, dest ip = %s, msgtype = %d, error number = %d", inet_ntoa((cIter->supplierDeque[i].publicationAddr).sin_addr), pHeader.Type, myGetLastError());
                    }
                    else if(len != pHeader.Length)
                    {
                        messager(msgERROR, "send to error, dest ip = %s, msgtype = %d, msglen = %d, sendlen = %d", inet_ntoa((cIter->supplierDeque[i].publicationAddr).sin_addr), pHeader.Type, pHeader.Length, len);
                    }
                    else
                    {
                        messager(msgINFO, "channel %d send DO_YOU_HAVE to %s:%d, supplierUID=%d",
                                 cIter->channelID,
                                 inet_ntoa(cIter->getPossibleSupplierAddr(i)->sin_addr),
                                 ntohs(cIter->getPossibleSupplierAddr(i)->sin_port),
                                 cIter->supplierDeque[i].UID);
						mySleep(100);
                    }
                }
            }
        }
        else if(cIter->Status == ctsQueryFinished)
        {
            {
                pduHeader pHeader;
                pduPleaseStartTrans pStart;
                pHeader.myUID = globalConfig.UID;
#if defined(MOHO_X86)
                pHeader.seqNumber = __sync_add_and_fetch(&globalConfig.seqNumber,1);
#elif defined(MOHO_W32)              
                pHeader.seqNumber = InterlockedIncrement(&globalConfig.seqNumber);
#endif
                pHeader.Type = PLEASE_START_TRANS;
                pHeader.Length = sizeof(pHeader) + sizeof(pStart);

                cIter->lockConsumerArrivedDeque.lock();
                if(cIter->nextPduSeq > 0)
                {
                    if(cIter->arrivedBufDeque.size() == 0)
                    {
                        pStart.expectedSeqNum = cIter->nextPduSeq;
                    }
                    else
                    {
                        pStart.expectedSeqNum = cIter->arrivedBufDeque.back()->seqNum + 1;
                    }
                    messager(msgINFO, "channel %d expect seq number = %lld,cIter->nextPduSeq__%lld,cIter->arrivedBufDeque.size()__%d", cIter->channelID , pStart.expectedSeqNum,cIter->nextPduSeq,cIter->arrivedBufDeque.size());
                }
                else
                {
                    pStart.expectedSeqNum = 0;
                }
                cIter->lockConsumerArrivedDeque.unlock();

                pStart.channelID = (*cIter).channelID;
                pStart.s = (*cIter).startTime;
                pStart.consumerPort = cIter->consumerPort;
                messager(msgINFO, "channel %d expect seq number = %lld, cIter->getSupplierDataTransferAddr()__%p", cIter->channelID , pStart.expectedSeqNum,cIter->getSupplierDataTransferAddr());

                destIP = *(cIter->getSupplierDataTransferAddr());
                memcpy(msg, &pHeader, sizeof(pHeader));
                memcpy(msg + sizeof(pHeader), &pStart, sizeof(pStart));
                sendChannelQuery(&(cIter->consumerSrv), &(*cIter));

                len = sendto(cIter->consumerSrv, msg, pHeader.Length, 0, (SOCKADDR *)(&destIP), tolen);
                if(len == SOCKET_ERROR)
                {
                    messager(msgERROR, "send to failed, dest ip = %s, msgtype = %d, error number = %d", inet_ntoa(destIP.sin_addr), pHeader.Type, myGetLastError());
                }
                else if(len != pHeader.Length)
                {
                    messager(msgERROR, "send to error, dest ip = %s, msgtype = %d, msglen = %d, sendlen = %d", inet_ntoa(destIP.sin_addr), pHeader.Type, pHeader.Length, len);
                }
                else
                {
                    messager(msgINFO, "send PLEASE_START_TRANS to %s:%d, supplierUID=%d",
                             inet_ntoa(destIP.sin_addr),
                             ntohs(destIP.sin_port),
                             cIter->supplierUID);
                }
            }
        }
        else if(cIter->supplierUID != 0 && cIter->Status == ctsStarted)
        {
            UINT RESTART_CHANNEL_CONDITION_2 = globalConfig.maxSupplyBuffer;
            if(RESTART_CHANNEL_CONDITION_2 < MAX_ARRIVED_BUF_LENGTH)
            {
                RESTART_CHANNEL_CONDITION_2 = MAX_ARRIVED_BUF_LENGTH;
            }

            int size = cIter->arrivedBufDeque.size();
            int64_t diff = _GetTickCount64();
            diff -= cIter->lastPDUTimeTick;
            int a = 0;
            if(cIter->lastPDUTimeTick != 0 && diff > (RESTART_CHANNEL_CONDITION_1 * 1000))
            {
                messager(msgERROR, "channel %d NEED TO RESTART1__%d, NO_PACKET time = %lld,diff__%lld",
                                        cIter->channelID,
                                        RESTART_CHANNEL_CONDITION_1 * 1000,
                                         _GetTickCount64() - cIter->lastPDUTimeTick,
                                        diff);
                a = 1;
            }
            else if(cIter->arrivedBufDeque.size() > (RESTART_CHANNEL_CONDITION_2 * NEED_TO_RESTART_PERCENT))
            {
                messager(msgERROR, "channel %d NEED TO RESTART2__%d, bufferSize = %d,size__%d",
					cIter->channelID,
                                        RESTART_CHANNEL_CONDITION_2 * NEED_TO_RESTART_PERCENT,
                                        cIter->arrivedBufDeque.size(),
                                        size);
                a = 1;
            }
            if(a)
            {
                cIter->lockConsumerArrivedDeque.lock();
                if(cIter->arrivedBufDeque.size() >= RESTART_CHANNEL_CONDITION_2 * NEED_TO_RESTART_PERCENT)
                {
                    std::deque<pduBuffer *>::iterator pIter;
                    for(pIter = cIter->arrivedBufDeque.begin(); pIter != cIter->arrivedBufDeque.end(); pIter ++)
                    {
                        if(*pIter != NULL)
                        {
                            delete (*pIter);
                        }
                    }
                    cIter->arrivedBufDeque.clear();
                    cIter->recvdFilesNumber = 0;
                    cIter->nextPduSeq = 0;
                }
                cIter->lockConsumerArrivedDeque.unlock();

                cIter->lastPDUTimeTick = _GetTickCount64();
                cIter->timesInCtsNotStartedStatus = 0;
               cIter->supplierDeque.clear();

                cIter->Status = ctsTrackerQuery;

                //checkConsumer();
                //goto start2checkconsumer;
            }
        }
        else
        {
            messager(msgDEBUG, "taskConsumerProcesser: do nothing now");
        }
    }
    return ;
}


//初始化时钟中断
int initTimer()
{
#if defined(MOHO_X86)
    int ret = 0;
    struct sigevent time_evp;

    memset(&time_evp, 0, sizeof(struct sigevent));
    time_evp.sigev_value.sival_ptr = &g_neighbor_process_timeid;
    time_evp.sigev_notify = SIGEV_THREAD;
    time_evp.sigev_notify_function = taskNeighborNodeProcesser;
    ret = timer_create(CLOCK_REALTIME, &time_evp, &g_neighbor_process_timeid);
    if(ret < 0)
    {
        messager(msgFATAL, "timer_create failed at taskNeighborNodeProcesser");
    }
    else
    {
        struct itimerspec ts;
        ts.it_interval.tv_sec = TIME_PERIOD_OF_NEIGHBOR_TASK/1000;
        ts.it_interval.tv_nsec = (TIME_PERIOD_OF_NEIGHBOR_TASK%1000)*1000000;
        ts.it_value.tv_sec = TIME_PERIOD_OF_NEIGHBOR_TASK/1000;
        ts.it_value.tv_nsec = (TIME_PERIOD_OF_NEIGHBOR_TASK%1000)*1000000;
        ret = timer_settime(g_neighbor_process_timeid, TIMER_ABSTIME, &ts, NULL);
        if(ret < 0)
        {
            timer_delete(g_neighbor_process_timeid);
            messager(msgFATAL, "timer_settime failed at taskNeighborNodeProcesser");
        }
    }

    memset(&time_evp, 0, sizeof(struct sigevent));
    time_evp.sigev_value.sival_ptr = &g_consumer_process_timeid;
    time_evp.sigev_notify = SIGEV_THREAD;
    time_evp.sigev_notify_function = taskConsumerProcesser;
    ret = timer_create(CLOCK_REALTIME, &time_evp, &g_consumer_process_timeid);
    if(ret < 0)
    {
        messager(msgFATAL, "timer_create failed at taskConsumerProcesser");
    }
    else
    {
        struct itimerspec ts;
        ts.it_interval.tv_sec = TIME_PERIOD_OF_CONSUMER_TASK/1000;
        ts.it_interval.tv_nsec = (TIME_PERIOD_OF_CONSUMER_TASK%1000)*1000000;
        ts.it_value.tv_sec = TIME_PERIOD_OF_CONSUMER_TASK/1000;
        ts.it_value.tv_nsec = (TIME_PERIOD_OF_CONSUMER_TASK%1000)*1000000;
        ret = timer_settime(g_consumer_process_timeid, TIMER_ABSTIME, &ts, NULL);
        if(ret < 0)
        {
            timer_delete(g_consumer_process_timeid);
            messager(msgFATAL, "timer_settime failed at taskConsumerProcesser");
        }
    }
    messager(msgINFO, "g_neighbor_process_timeid__%p,g_consumer_process_timeid__%p",g_neighbor_process_timeid,g_consumer_process_timeid);

#elif defined(MOHO_WIN32)
    //TIMECAPS tc;
    //UINT c;
    //c = sizeof(TIMECAPS);

    //timeGetDevCaps(&tc, c);

    UINT uResult = SetTimer(pMainWnd,IDT_NEIGHBOR_PROCESS_TASK,TIME_PERIOD_OF_NEIGHBOR_TASK,(TIMERPROC) taskNeighborNodeProcesser);
    if(uResult == 0)
    {
        messager(msgFATAL, "No timer is available at taskNeighborNodeProcesser");
    }

    uResult = SetTimer(pMainWnd,IDT_CONSUMER_PROCESS_TASK,TIME_PERIOD_OF_CONSUMER_TASK,(TIMERPROC) taskConsumerProcesser);
    if(uResult == 0)
    {
        messager(msgFATAL, "No timer is available at taskConsumerProcesser");
    }
    //consumerTimes = 0;
    //uResult = SetTimer(pMainWnd,IDT_FTDS_PROCESS_TASK, FTDS_SEND_PERIOD,(TIMERPROC) taskFtdsProcesser);
    //if(uResult == 0)
    //{
    //	messager(msgFATAL, "No timer is available at taskFtdsProcesser");
    //}
#endif

    return 0;
}

bool checkUpnp()
{
    //messager(msgINFO,"[ UPnP ] : check upnp ablity ..");

    ////初始化upnp
    //if(!upnp.init(10,10))
    //{
    //	messager(msgINFO,"[ UPnP ] : not have the ability of UPnP,Error=%s", upnp.get_last_error());
    //	return false;
    //}
    ////寻找upnp设备
    //if(!upnp.discovery())
    //{
    //	messager(msgINFO,"[ UPnP ] : not have the ability of UPnP!Error=%s",upnp.get_last_error());
    //	return false;
    //}
    ////得到描述文件
    //if(!upnp.get_description())
    //{
    //	messager(msgINFO,"[ UPnP ] : not have the ability of UPnP!Error=%d",upnp.get_last_error());
    //	return false;
    //}
    ////解析描述文件 获得controlURL
    //if(!upnp.parse_description())
    //{
    //	messager(msgINFO,"[ UPnP ] : not have the ability of UPnP!Error=%d",upnp.get_last_error());
    //	return false;
    //}
    //
    //messager(msgINFO,"[ UPnP ] : has upnp ability!");

    return true;
}


int uPnP(USHORT intraPort, USHORT interPort)
{
    return -1;

    UPNPNAT nat;
    nat.init(5,10);

    if(!nat.discovery()){
        messager(msgINFO, "discovery error is %s",nat.get_last_error());
        return -1;
    }

    if(!nat.del_port_mapping(interPort, "UDP"))
    {
        messager(msgINFO, "del_port_mapping error is %s",nat.get_last_error());
        //return -1;
    }
    else
    {
        messager(msgINFO, "del_port_mapping %d succ", interPort);
    }

    if(!nat.add_port_mapping("LiveCDN",
                             inet_ntoa(globalConfig.addrSrv.sin_addr),
                             intraPort,
                             interPort,
                             "UDP"))
    {
        messager(msgINFO, "add_port_mapping error is %s",nat.get_last_error());
        return -1;
    }

    messager(msgINFO, "add port mapping %d : %d succ.", intraPort, interPort);

    return 1;
}

//网络初始化，开启信令端口
int initNetwork()
{
#if defined(MOHO_WIN32)
    WORD wVersionRequested;
    WSADATA wsaData;
    int err;
    wVersionRequested = MAKEWORD( 1, 1 );
    err = WSAStartup( wVersionRequested, &wsaData );
    if ( err != 0 )
    {
        return -1;
    }
    if ( LOBYTE( wsaData.wVersion ) != 1 ||
         HIBYTE( wsaData.wVersion ) != 1 )
    {
        WSACleanup( );
        return -1;
    }
#endif

    SOCKADDR_IN localIP;

    if(getLocalIpByUdp(localIP)  == 0)
    {
        globalConfig.ipAddress = inet_addr(inet_ntoa(localIP.sin_addr));
    }


    if(globalConfig.carrier != MULTI_CARRIER_NODE)
    {
        IpQueryReplay replay;
        int ret = synchronGetLocation(0,replay);
        if(ret == 0)
        {
            globalConfig.carrier = replay.carrir;
            globalConfig.province = replay.provice;
            globalConfig.city = replay.city;
            messager(msgINFO, "carrier: %s, provice: %s, city: %s",
                     globalConfig.carrier.c_str(),
                     globalConfig.province.c_str(),
                     globalConfig.city.c_str());
        }
    }

    sockPublicationSrv = socket( AF_INET , SOCK_DGRAM , IPPROTO_UDP ) ;

#if defined(MOHO_X86)
    int optSO_REUSEADDR = 1;//设置soet可重用，否则重启程序后bind总是出错
    setsockopt(sockPublicationSrv,SOL_SOCKET,SO_REUSEADDR,&optSO_REUSEADDR,sizeof(optSO_REUSEADDR));
#elif defined(MOHO_WIN32)
    BOOL bReuseaddr=TRUE;
    setsockopt(sockPublicationSrv,SOL_SOCKET ,SO_REUSEADDR,(const char*)&bReuseaddr,sizeof(BOOL));
#endif

    //SOCKADDR_IN addrSrv ;
#if defined(MOHO_X86)
    // globalConfig.addrSrv.sin_addr.s_addr = globalConfig.ipAddress ;
    globalConfig.addrSrv.sin_addr.s_addr =  htonl(INADDR_ANY);
#elif defined(MOHO_WIN32)
    globalConfig.addrSrv.sin_addr.S_un.S_addr = globalConfig.ipAddress ;
#endif
    globalConfig.addrSrv.sin_family = AF_INET ;
    globalConfig.addrSrv.sin_port = htons(globalConfig.publicationPort) ;

    uPnP(ntohs(globalConfig.addrSrv.sin_port), ntohs(globalConfig.addrSrv.sin_port));


    //if(checkUpnp())
    //{
    //	UINT externPort;
    //	if(upnp.add_port_mapping(globalConfig.publicationPort, "UDP", &externPort))
    //	{
    //		messager(msgINFO, "map %d to %d success");
    //	}
    //	else
    //	{
    //		messager(msgERROR, "%s", upnp.get_last_error());
    //	}
    //}


    if(bind( sockPublicationSrv , (SOCKADDR*)&(globalConfig.addrSrv) , sizeof(SOCKADDR)) != 0)
    {
        messager(msgFATAL, "bind error, port = %d, error number = %d", globalConfig.publicationPort, myGetLastError());
        return -1;
    }
    else
    {
        messager(msgINFO, "UID %d bind %s:%d success", globalConfig.UID, inet_ntoa(globalConfig.addrSrv.sin_addr), globalConfig.publicationPort);
    }


    sendSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    Info.sockSrv = &sockPublicationSrv;
    //Info.sendSocket = &sendSocket;
    Info.sendSocket = &sockPublicationSrv;

    CMyThread thread;
    thread.creat(channelPublicationThreadProc,(void*)&Info);
    checkConsumer();
    //sayHello2EveryOne();
    //    DWORD dwThreadID = 0;
    //    HANDLE hThread = CreateThread(
    //            NULL,					            //安全属性使用缺省。
    //            0,					                //线程的堆栈大小。
    //            channelPublicationThreadProc,       //线程运行函数地址。
    //            &Info,								//传给线程函数的参数。
    //            0,									//创建标志。
    //            &dwThreadID);						//成功创建后的线程标识码。

    //std::list<tsConsumerTask>::iterator cIter;
    //for(cIter = globalTsConsumerTaskList.begin(); cIter != globalTsConsumerTaskList.end(); cIter ++)
    //{
    //	cIter->consumerThread = CreateThread(
    //						NULL,							//安全属性使用缺省。
    //						0,								//线程的堆栈大小。
    //						consumerTaskThreadProc,         //线程运行函数地址。
    //						&(*cIter),						//传给线程函数的参数。
    //						0,								//创建标志。
    //						&(cIter->consumerThreadID));	//成功创建后的线程标识码。
    //}


    return 0;

}

//添加一个消费者线程
int addNewConsumer(tsConsumerTask *cInfo)
{
    messager(msgINFO, "start consumer thread for channel %d", cInfo->channelID);
    cInfo->consumerThread.creat(consumerTaskThreadProc_CDN,(void*)cInfo);
    cInfo->consumerResendThread.creat(consumerResendTaskThreadProc_CDN,(void*)cInfo);
    //    cInfo->consumerThread = CreateThread(
    //            NULL,							//安全属性使用缺省。
    //            0,								//线程的堆栈大小。
    //            consumerTaskThreadProc,         //线程运行函数地址。
    //            cInfo,						//传给线程函数的参数。
    //            0,								//创建标志。
    //            &(cInfo->consumerThreadID));	//成功创建后的线程标识码。
    return 0;
}


